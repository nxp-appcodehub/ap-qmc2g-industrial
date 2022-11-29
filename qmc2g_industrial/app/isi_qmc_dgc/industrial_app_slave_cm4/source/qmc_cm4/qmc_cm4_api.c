/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/*!
 * @file qmc_cm4_api.c
 * @brief Implements the CM4 board API.
 *
 * The CM4 board API acts as glue logic between the HAL and the higher-level API functions.
 *
 * Main tasks of the CM4 board API are:
 *  - Configuring the CM4 software and hardware parts
 *  - Managing the functional watchdogs (configuration, ticking, kicking, reset notification, reset)
 *      - See "qmc_cm4_features_config.h" header file for configuration.
 *  - Managing the authenticated watchdog (configuration, ticking, kicking, reset notification, reset)
 *      - See "qmc_cm4_features_config.h" header file for configuration.
 *  - Setting the user outputs (GPIO13), notifying CM7 about user input (GPIO13) changes
 *      - See "qmc_cm4_features_config.h" for initial output value
 *  - Setting and getting the current time (SRTC)
 *  - Writing the firmware update status and the reset cause to the battery-backed SNVS LP register
 *  - Backing up the authenticated watchdog state to the battery-backed SNVS LP register
 *  - Performing system resets
 *
 * Since it is assumed that any access to the SNVS domain is too slow to run in ISRs, the
 * required SNVS content is mirrored within this module. There are two mirrors of all required SNVS fields.
 * In the main processing loop the mirrors are checked for differences.
 * If the mirrors are different, the changed items are synchronized with the SNVS hardware registers.
 * NOTE: The state backup of the secure watchdog timer is still written to hardware in an ISR out of security
 * reasons (attacker should not be able to extend timeout by blocking synchronization).
 *
 * The following code does not use interrupt nesting and correct functionality does depend on this
 * precondition! The maximum time spend in a interrupt (without invocations that lead to a reset) is 210us,
 * hence we can guarantee that the interrupt ticking the watchdogs works as expected (only the hardware watchdog
 * expiry interrupt has a higher priority, but that is excluded as it is executed only if the program can not be
 * trusted anymore).
 * Interrupt invocations which led to a reset were excluded, because even if the ISR processing would have been
 * faster just the reset would have happened faster, in both cases (future) pending ISRs are not executed.
 * For the timeout backup of the AWDG this is also not critical as it is anyhow halved at each reboot.
 * In the worst case ticking might happen around 210us later as expected, but that can be neglected
 * in comparison to the wanted timeout intervals.
 *
 * For reentrancy and concurrency assumptions, check the individual API descriptions.
 *
 * Limitations:
 *  - The backed-up RTC offset needs two SNVS LPGPR registers, hence a power-loss / reset inbetween
 *    the writing of these two registers may lead to a corrupted RTC offset. Furthermore, this may lead
 *    to unexpected time values.
 */

#include "qmc_cm4/qmc_cm4_api.h"
#include "qmc_cm4_features_config.h"
#include "hal/hal.h"
#include MBEDTLS_CONFIG_FILE
#include "mbedtls/memory_buffer_alloc.h"

#include "lwdg/lwdg_unit_api.h"
#include "awdg/awdg_api.h"
#include "rpc/rpc_api.h"

#include "utils/mem.h"
#include "utils/debug_log_levels.h"
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_SILENT
#endif
#include "utils/debug.h"
#include "utils/testing.h"
#include <string.h>

/*******************************************************************************
 * Variables
 *******************************************************************************/

/* SNVS mirror registers */
static volatile qmc_cm4_snvs_mirror_data_t gs_snvsStateModified = {
    0}; /*!< SNVS mirror register (for modifications!), should not be accessed from multiple
     contexts simultaneously, initialized with default state */
static volatile qmc_cm4_snvs_mirror_data_t gs_snvsStateHW = {
    0}; /*!< SNVS mirror register (HW), initialized with default state */

/* mbedtls static memory buffer */
STATIC_TEST_VISIBLE uint8_t
    gs_mbedTlsHeapStaticBuffer[AWDG_MBEDTLS_HEAP_MIN_STATIC_BUFFER_SIZE]; /*!< static memory buffer for mbedtls */

/* logical watchdog units + authenticated watchdog */
STATIC_TEST_VISIBLE volatile logical_watchdog_t
    gs_fwdgs[QMC_CM4_FWDGS_COUNT];                       /*!< functional watchdogs, configure in header file */
STATIC_TEST_VISIBLE logical_watchdog_unit_t gs_fwdgUnit; /*!< functional watchdog unit, configure in header file */

static volatile uint32_t gs_halGpioInputState; /*!< the current debounced GPIO input state from the HAL module */

static volatile qmc_reset_cause_id_t gs_previousResetCause; /*! the buffered previous reset cause */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Helper for initializing the functional watchdogs.
 *
 * Must be called before the communication and the tick interrupt are enabled.
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Err
 * An error at initializing the functional watchdogs occurred.
 * @retval kStatus_QMC_Ok
 * The functional watchdogs were initialized successfully.
 */
static qmc_status_t InitializeFunctionalWatchdogsHelper(void)
{
    qmc_status_t ret                             = kStatus_QMC_Err;
    uint32_t fwdgsTimeoutMs[QMC_CM4_FWDGS_COUNT] = QMC_CM4_FWDGS_TIMEOUT_MS;

    /* init functional watchdog unit */
    if (kStatus_LWDG_Ok != LWDGU_Init(&gs_fwdgUnit, QMC_CM4_FWDGS_GRACE_PERIOD_MS, QMC_CM4_WDG_TICK_FREQUENCY_HZ,
                                      gs_fwdgs, QMC_CM4_FWDGS_COUNT))
    {
        DEBUG_LOG_E(DEBUG_M4_TAG "LWDGU_Init failed!\r\n");
        ret = kStatus_QMC_Err;
    }
    /* group initialized OK */
    else
    {
        /* initialize single watchdogs */
        ret = kStatus_QMC_Ok;
        for (uint8_t id = 0U; id < QMC_CM4_FWDGS_COUNT; id++)
        {
            if (kStatus_LWDG_Ok != LWDGU_InitWatchdog(&gs_fwdgUnit, id, fwdgsTimeoutMs[id]))
            {
                DEBUG_LOG_E(DEBUG_M4_TAG "LWDGU_InitWatchdog %u failed!\r\n", id);
                ret = kStatus_QMC_Err;
                break;
            }
        }
    }

    return ret;
}

#if FEATURE_SECURE_WATCHDOG
/*!
 * @brief Helper for converting the given AWDG ticks to their backup format for
 * the battery-backed SNVS register.
 *
 * Rounds up to whole integer backup values.
 * Therefore, the returned value is only 0 if the given awdgTicks value was 0.
 *
 * @param[in] awdgTicks The AWDG ticks to convert.
 * @return uint16_t The backup value for the given AWDG ticks, 0 if the given ticks value was 0.
 */
static uint16_t ConvertAuthenticatedWatchdogTicksToBackupValue(uint32_t awdgTicks)
{
    /* round up to the next whole backup value */
    return (uint16_t)((awdgTicks + QMC_CM4_AWDG_TICKS_BACKUP_MAX) >> QMC_CM4_AWDG_TICKS_BACKUP_SHIFT);
}

/*!
 * @brief Helper for handling the expiration of the authenticated watchdog.
 *
 * Do only call from interrupts or before interrupts are enabled!
 */
static void HandleAuthenticatedWatchdogExpirationIntsDisabled(void)
{
    qmc_status_t notifyRet = kStatus_QMC_Err;
    (void)notifyRet;

    /* remember that the secure watchdog expired */
    gs_snvsStateModified.fwuStatus |= kFWU_AwdtExpired;
    gs_snvsStateModified.resetCause = kQMC_ResetSecureWd;
    /* reset backed-up watchdog status */
    gs_snvsStateModified.wdTimerBackup = 0U;
    gs_snvsStateModified.wdStatus      = 0U;
    /* if the notification does not work, the system is still reset
     * (but the CM7 might not have enough time for final tasks) */
    notifyRet = RPC_NotifyCM7AboutReset(kQMC_ResetSecureWd);
    assert(kStatus_QMC_Ok == notifyRet);
}

/*!
 * @brief Helper for initializing the authenticated watchdog.
 *
 * Must be called before the communication and the tick interrupt are enabled.
 * Zeros the RNG seed and PK after initialization is complete.
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Err
 * An error at initializing the authenticated watchdog occurred.
 * @retval kStatus_QMC_Ok
 * The authenticated watchdog was intialized successfully.
 */
static qmc_status_t InitializeAuthenticatedWatchdogHelper(void)
{
    qmc_status_t ret               = kStatus_QMC_Err;
    awdg_init_status_t awdgInitRet = kStatus_AWDG_InitError;
    lwdg_tick_status_t awdgTickRet = kStatus_LWDG_TickErr;
    uint32_t ticksToTimeout        = 0U;

    /* counter was running before before startup
     * restore timeout ticks value and reduce it
     * (a possible expiration is handled further below)
     */
    if (gs_snvsStateModified.wdStatus > 0U)
    {
        /* sync is performed at the end of QMC_CM4_Init() or if an reset is triggered
         * always reduce count (divide by 2, rounding down) here
         * so that we expire eventually if we are in a reboot loop */
        gs_snvsStateModified.wdTimerBackup >>= 1U;
        /* stored value was shifted, undo this */
        ticksToTimeout = gs_snvsStateModified.wdTimerBackup << QMC_CM4_AWDG_TICKS_BACKUP_SHIFT;
    }

    /* restore or initialize the AWDG depending on SNVS values */
    awdgInitRet =
        AWDG_Init(QMC_CM4_AWDG_INITIAL_TIMEOUT_MS, QMC_CM4_AWDG_GRACE_PERIOD_MS, QMC_CM4_WDG_TICK_FREQUENCY_HZ,
                  ticksToTimeout, gs_snvsStateModified.wdStatus, g_awdgInitDataSHM.rngSeed,
                  g_awdgInitDataSHM.rngSeedLen, g_awdgInitDataSHM.pk, g_awdgInitDataSHM.pkLen);
    /* zero RNG seed and key material after usage */
    memset(g_awdgInitDataSHM.rngSeed, 0U, sizeof(g_awdgInitDataSHM.rngSeed));
    memset(g_awdgInitDataSHM.pk, 0U, sizeof(g_awdgInitDataSHM.pk));

    /* evaluate init return value */
    switch (awdgInitRet)
    {
        case kStatus_AWDG_InitInitializedNew:
            /* tick once (AWDG_Init() uses LWDGU_KickOne() internally which increases the defined timeout by 1)
             * otherwise, the backup value would be increased as we round up! */
            awdgTickRet = AWDG_Tick();
            /* save authenticated watchdog state if freshly initialized
             * (otherwise it was anyhow already running)
             * a possible expiration is handled further below */
            ticksToTimeout                     = AWDG_GetRemainingTicks();
            gs_snvsStateModified.wdTimerBackup = ConvertAuthenticatedWatchdogTicksToBackupValue(ticksToTimeout);
            gs_snvsStateModified.wdStatus      = 1U;
            ret                                = kStatus_QMC_Ok;
            break;
        case kStatus_AWDG_InitInitializedResumed:
            /* tick once (AWDG_Init() uses LWDGU_KickOne() internally which increases the defined timeout by 1) */
            awdgTickRet = AWDG_Tick();
            ret         = kStatus_QMC_Ok;
            break;
        default:
            DEBUG_LOG_E(DEBUG_M4_TAG "AWDG_Init failed (%d)!\r\n", awdgInitRet);
            ret = kStatus_QMC_Err;
            break;
    }

    /* AWDG already expired and grace period started
     * immediately set appropriate reset cause, reset the watchdog backup and notify CM7
     * reset is performed after the grace period in the ticking interrupt */
    assert((kStatus_LWDG_TickRunning != awdgTickRet) && (kStatus_LWDG_TickPreviouslyExpired != awdgTickRet));
    if (kStatus_LWDG_TickJustStarted == awdgTickRet)
    {
        /* set appropriate reset cause, reset backed-up state, notify CM7 */
        HandleAuthenticatedWatchdogExpirationIntsDisabled();
    }
    /* no grace period -> reset backed-up state and system */
    else if (kStatus_LWDG_TickJustExpired == awdgTickRet)
    {
        QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
    }

    return ret;
}
#endif

/*!
 * @brief Helper function for translating the HAL input state to the QMC format.
 */
static uint8_t HalGpioState2QmcCm4GpioInput(uint32_t halGpioRegister)
{
    uint8_t qmcCm4Inputs = 0U;
    if (halGpioRegister & (1U << kHAL_SnvsUserInput0))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT4_DATA;
    }
    if (halGpioRegister & (1U << kHAL_SnvsUserInput1))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT5_DATA;
    }
    if (halGpioRegister & (1U << kHAL_SnvsUserInput2))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT6_DATA;
    }
    if (halGpioRegister & (1U << kHAL_SnvsUserInput3))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT7_DATA;
    }
    return qmcCm4Inputs;
}

/*!
 * @brief Loads the SNVS state into both software mirrors and performs sanity checks on the loaded values.
 *
 * All data which is stored in the battery-backed SNVS GPR registers should work
 * correctly with an initial value of zero (value after SNVS LP POR)!
 *
 * Not reentrant, must be called before interrupts are enabled!
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok The backed-up state from the SNVS LP domain was successfully restored.
 * @retval kStatus_QMC_Err The sanity check failed for some value(s), the SNVS state was reset.
 */
static qmc_status_t LoadAndCheckSnvsState(void)
{
    bool isSane = true;
    qmc_cm4_snvs_mirror_data_t loadedSnvsState;

    /* load data from the hardware registers */
    loadedSnvsState.wdTimerBackup    = HAL_GetWdTimerBackup();
    loadedSnvsState.wdStatus         = HAL_GetWdStatus();
    loadedSnvsState.fwuStatus        = HAL_GetFwuStatus();
    loadedSnvsState.srtcOffset       = HAL_GetSrtcOffset();
    loadedSnvsState.resetCause       = HAL_GetResetCause();
    loadedSnvsState.gpioOutputStatus = HAL_GetSnvsGpio13Outputs();

    /* check if entries have sane values (checks are not possible for all values) */

    /* if previous reset reason was kQMC_ResetSecureWd, then the watchdog should be freshly initialized */
    if ((kQMC_ResetSecureWd == loadedSnvsState.resetCause) && (loadedSnvsState.wdStatus > 0U))
    {
        isSane = false;
    }
    /* check FWU status */
    if (0U != (loadedSnvsState.fwuStatus & ~QMC_CM4_VALID_FWU_STATE_MASK))
    {
        isSane = false;
    }
    /* check reset cause */
    if((kQMC_ResetNone != loadedSnvsState.resetCause) && (kQMC_ResetRequest != loadedSnvsState.resetCause) &&
       (kQMC_ResetFunctionalWd != loadedSnvsState.resetCause) && (kQMC_ResetSecureWd != loadedSnvsState.resetCause))
    {
        isSane = false;
    }

    if (true == isSane)
    {
        /* everything OK, latch values */
        gs_snvsStateModified = gs_snvsStateHW = loadedSnvsState;

        /* latch previous reset cause */
        gs_previousResetCause = gs_snvsStateHW.resetCause;
        /* reinitialize reset cause to allow buffering of new cause */
        gs_snvsStateModified.resetCause = kQMC_ResetNone;

        return kStatus_QMC_Ok;
    }
    else
    {
        /* clear SNVS state */
        HAL_SetWdTimerBackup(0U);
        HAL_SetWdStatus(0U);
        HAL_SetFwuStatus(0U);
        HAL_SetSrtcOffset(0U);
        HAL_SetResetCause(0U);

        return kStatus_QMC_Err;
    }
}

/*!
 * @brief Synchronize the SNVS software mirrors with the SNVS hardware.
 *
 * Indented to be called from the main loop, do not call from an interrupt!
 * The WD backup is also written outside of this function in the ticking ISR
 * (after the ISR started). By moving the sync to the ISR we avoid the case where
 * an attacker blocks this function by continuously triggering interrupts and
 * hence no update of the WD backup would be possible anymore!
 *
 * Not reentrant!
 */
static void QMC_CM4_SyncSnvsStorageMain(void)
{
    qmc_cm4_snvs_mirror_data_diff_t diff = {0};

    /* check for differences between the software mirrors and latch them in one batch
     * (ensures that values that were changed together in an interrupt are latched together)
     */
    HAL_EnterCriticalSectionNonISR();
    if (gs_snvsStateModified.wdTimerBackup != gs_snvsStateHW.wdTimerBackup)
    {
        gs_snvsStateHW.wdTimerBackup = gs_snvsStateModified.wdTimerBackup;
        diff.wdTimerBackup           = true;
    }
    if (gs_snvsStateModified.wdStatus != gs_snvsStateHW.wdStatus)
    {
        gs_snvsStateHW.wdStatus = gs_snvsStateModified.wdStatus;
        diff.wdStatus           = true;
    }
    if (gs_snvsStateModified.fwuStatus != gs_snvsStateHW.fwuStatus)
    {
        gs_snvsStateHW.fwuStatus = gs_snvsStateModified.fwuStatus;
        diff.fwuStatus           = true;
    }
    if (gs_snvsStateModified.srtcOffset != gs_snvsStateHW.srtcOffset)
    {
        gs_snvsStateHW.srtcOffset = gs_snvsStateModified.srtcOffset;
        diff.srtcOffset           = true;
    }
    if (gs_snvsStateModified.resetCause != gs_snvsStateHW.resetCause)
    {
        gs_snvsStateHW.resetCause = gs_snvsStateModified.resetCause;
        diff.resetCause           = true;
    }
    HAL_ExitCriticalSectionNonISR();

    /* write to hardware (with enabled interrupts as writing is slow) */
    /* write FWU status and reset cause first as they modify boot behavior and should
     * be set before modifying the watchdog backup (helps avoiding conflicting
     * states of both) */
    if (diff.fwuStatus)
    {
        HAL_SetFwuStatus(gs_snvsStateHW.fwuStatus);
    }
    if (diff.resetCause)
    {
        HAL_SetResetCause(gs_snvsStateHW.resetCause);
    }
    if (diff.wdTimerBackup)
    {
        HAL_SetWdTimerBackup(gs_snvsStateHW.wdTimerBackup);
    }
    if (diff.wdStatus)
    {
        HAL_SetWdStatus(gs_snvsStateHW.wdStatus);
    }
    if (diff.srtcOffset)
    {
        HAL_SetSrtcOffset(gs_snvsStateHW.srtcOffset);
    }
}

qmc_status_t QMC_CM4_Init(void)
{
    /* early board initialization
     * without peripherals and interrupts */
    HAL_InitBoard();
    /* peripherals */
    /* also activates interrupt for hardware watchdog */
    HAL_InitHWWatchdog();
    HAL_InitSrtc();
    HAL_InitRtc();
    HAL_SnvsGpioInit(QMC_CM4_SNVS_USER_OUTPUTS_INIT_STATE);
    /* if it bounces already at the beginning we just read an intermediate state
     * (which is not wrong) */
    gs_halGpioInputState = HAL_GetSnvsGpio13();

    /* configure mbedtls to use a static buffer
     * if used for more things than just the AWDG increase buffer size */
    mbedtls_memory_buffer_alloc_init(gs_mbedTlsHeapStaticBuffer, AWDG_MBEDTLS_HEAP_MIN_STATIC_BUFFER_SIZE);

    /* load non-volatile SNVS state into its mirror registers
     * (must be after HAL_SnvsGpioInit() as it buffers the output state) */
    if (kStatus_QMC_Ok != LoadAndCheckSnvsState())
    {
        return kStatus_QMC_Err;
    }
    /* initialize the functional watchdogs */
    if (kStatus_QMC_Ok != InitializeFunctionalWatchdogsHelper())
    {
        return kStatus_QMC_Err;
    }
    /* initialize authenticated watchdog */
#if FEATURE_SECURE_WATCHDOG
    if (kStatus_QMC_Ok != InitializeAuthenticatedWatchdogHelper())
    {
        return kStatus_QMC_Err;
    }
#endif

    /* sync possibly changed SNVS values before communication with CM7 is allowed */
    QMC_CM4_SyncSnvsStorageMain();
    /* hardware testing showed that on the CM4 the last write to a SNVS LPGPR register
     * is not committed to hardware if a reset is performed directly afterwards (confirmed with MWE)
     * a dummy read from any SNVS LPGPR register seems to mitigate this issue
     * (ARM barriers did not help)
     */
    (void)HAL_GetWdStatus();

    /* activate interrupts - communication is now able to run */
    HAL_ConfigureEnableInterrupts();

    /* we are done initializing -> CM7 can start
     * this synchronization point should ensure that at least the initial updates are written to the
     * SNVS before the CM7 application can influence the behavior
     * this is important for the security claims
     * (attacker should not be able to perform endless reboot loops) */
    g_awdgInitDataSHM.ready = 1U;

    /* inform CM7 about initial GPIO state */
    HAL_EnterCriticalSectionNonISR();
    RPC_NotifyCM7AboutGpioChange(HalGpioState2QmcCm4GpioInput(gs_halGpioInputState));
    HAL_ExitCriticalSectionNonISR();

    /* start SysTick - debouncing handler starts being invoked immediately after this */
    HAL_InitSysTick();

    return kStatus_QMC_Ok;
}

qmc_status_t QMC_CM4_KickFunctionalWatchdogIntsDisabled(const uint8_t id)
{
    qmc_status_t ret              = kStatus_QMC_Err;
    lwdg_kick_status_t kickStatus = kStatus_LWDG_KickErr;

    kickStatus = LWDGU_KickOne(&gs_fwdgUnit, id);
    switch (kickStatus)
    {
        case kStatus_LWDG_KickStarted:
            ret = kStatus_QMC_Ok;
            break;
        case kStatus_LWDG_KickKicked:
            ret = kStatus_QMC_Ok;
            break;
        case kStatus_LWDG_KickInvArg:
            ret = kStatus_QMC_ErrArgInvalid;
            break;
        default:
            ret = kStatus_QMC_Err;
            break;
    }

    return ret;
}

#if FEATURE_SECURE_WATCHDOG
qmc_status_t QMC_CM4_GetNonceAuthenticatedWatchdog(uint8_t *const pData, uint32_t *const pDataLen)
{
    qmc_status_t ret   = kStatus_QMC_Err;
    size_t getNonceRet = 0U;

    if ((NULL == pData) || (NULL == pDataLen))
    {
        ret = kStatus_QMC_ErrArgInvalid;
    }
    else if (*pDataLen < AWDG_NONCE_LENGTH)
    {
        ret = kStatus_QMC_ErrNoBufs;
    }
    else
    {
        getNonceRet = AWDG_GetNonce(pData);
        if(AWDG_NONCE_LENGTH == getNonceRet)
        {
            *pDataLen = AWDG_NONCE_LENGTH;
            ret       = kStatus_QMC_Ok;
        }
        else
        {
            *pDataLen = 0U;
            ret       = kStatus_QMC_ErrInternal;
        }
    }

    return ret;
}

qmc_status_t QMC_CM4_ProcessTicketAuthenticatedWatchdog(const uint8_t *const pData, const uint32_t dataLen)
{
    qmc_status_t ret                                = kStatus_QMC_Err;
    awdg_validate_ticket_status_t validateTicketRet = kStatus_AWDG_ValidateTicketError;
    awdg_defer_watchdog_status_t deferWatchdogRet   = kStatus_AWDG_DeferWatchdogError;

    /* check arguments */
    if (NULL == pData)
    {
        return kStatus_QMC_ErrArgInvalid;
    }

    /* validate ticket */
    validateTicketRet = AWDG_ValidateTicket(pData, dataLen);
    /* validate ticket failed */
    if (kStatus_AWDG_ValidateTicketValidated != validateTicketRet)
    {
        /* translate to general return code */
        switch (validateTicketRet)
        {
            case kStatus_AWDG_ValidateTicketInvalidTicketLength:
                ret = kStatus_QMC_ErrArgInvalid;
                break;
            case kStatus_AWDG_ValidateTicketInvalidPointer:
                ret = kStatus_QMC_ErrInternal;
                break;
            case kStatus_AWDG_ValidateTicketHashingFailed:
                ret = kStatus_QMC_ErrInternal;
                break;
            case kStatus_AWDG_ValidateTicketRNGDisabled:
                ret = kStatus_QMC_ErrInternal;
                break;
            case kStatus_AWDG_ValidateTicketDidNotValidate:
                ret = kStatus_QMC_ErrSignatureInvalid;
                break;
            default:
                ret = kStatus_QMC_Err;
                break;
        }
        return ret;
    }

    /* deferring shares data with the tick ISR - protect it using a critical section */
    HAL_EnterCriticalSectionNonISR();
    deferWatchdogRet = AWDG_DeferWatchdog();
    HAL_ExitCriticalSectionNonISR();
    /* translate to general return code */
    switch (deferWatchdogRet)
    {
        case kStatus_AWDG_DeferWatchdogOk:
            ret = kStatus_QMC_Ok;
            break;
        case kStatus_AWDG_DeferWatchdogInvalidTimeout:
            ret = kStatus_QMC_ErrRange;
            break;
        case kStatus_AWDG_DeferWatchdogNotAllowed:
            ret = kStatus_QMC_ErrSignatureInvalid;
            break;
        default:
            ret = kStatus_QMC_Err;
            break;
    }

    return ret;
}
#endif

qmc_status_t QMC_CM4_SetSnvsGpioPinIntsDisabled(const hal_snvs_gpio_pin_t pin, const uint8_t value)
{
    if((kHAL_SnvsUserOutput0 != pin) && (kHAL_SnvsUserOutput1 != pin) && (kHAL_SnvsUserOutput2 != pin) &&
       (kHAL_SnvsUserOutput3 != pin) && (kHAL_SnvsSpiCs0 != pin) && (kHAL_SnvsSpiCs1 != pin))
    {
        return kStatus_QMC_ErrArgInvalid;
    }
    if (0U != value)
    {
        gs_snvsStateModified.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus | (1U << pin);
    }
    else
    {
        gs_snvsStateModified.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus & ~(1U << pin);
    }

    return kStatus_QMC_Ok;
}

/*!
 * @brief Transforms a SNVS RTC counter value to a timestamp in milliseconds.
 *
 * Transformation equation: time in ms = \f$ \lfloor rtcValue / 32768Hz * 1000 \rfloor \f$
 *
 * This function rounds down, i.e., the milliseconds are only advanced if the current
 * millisecond completely passed.
 *
 * The maximum supported input value without an overflow is
 * \f$ \lfloor \mathrm{INT64\_MAX}/1000\rfloor = \lfloor (2^{63} - 1)/1000\rfloor = 9223372036854775 \f$
 * which results in a timestamp in milliseconds which is capable of representing
 * approximately \f$ 9223372036854775 * 1000/32768/1000/3600/24/366 \approx 8901 \f$
 * years.
 *
 * This function is reentrant and can be used concurrently as long as pTimeMs points to
 * unique locations.
 *
 * @startuml
 * start
 * if (pTimeMs == NULL) then (true)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * if ((rtcValue > (INT64_MAX / 1000)) || (rtcValue < 0)) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :~*pTimeMs = rtcValue * 1000
 * *pTimeMs = *pTimeMs >> 15
 * return kStatus_QMC_Ok;
 * stop
 * @enduml
 *
 * @param[in] rtcValue A SNVS RTC counter value which should be converted to a timestamp
 * in milliseconds.
 * @param[out] pTimeMs Pointer to a variable which gets the resulting timestamp in milliseconds
 * assigned. The result has only been written if this function returns kStatus_QMC_Ok.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrRange
 * The given RTC counter value was below zero or too large so that
 * an overflow would have occurred. The result data at *pTimeMs was not modified.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully, the result has been written to *pTimeMs.
 */
static qmc_status_t ConvertRtcCounterValToMs(int64_t rtcValue, int64_t *pTimeMs)
{
    assert(NULL != pTimeMs);

    if ((rtcValue > (INT64_MAX / 1000)) || (rtcValue < 0))
    {
        return kStatus_QMC_ErrRange;
    }

    /* *pTimeMs = rtcValue * 1000 */
    *pTimeMs = rtcValue * 1000; /* do this first to prevent rounding errors */
    /* *pTimeMs = (rtcValue * 1000) / 32768 */
    *pTimeMs = *pTimeMs >> 15U;

    return kStatus_QMC_Ok;
}

/*!
 * @brief Transforms a timestamp in milliseconds to a value compatible with a SNVS
 * RTC counter.
 *
 * Transformation equation: \f$ rtcValue = \lceil timeMs / 1000 * 32768 Hz \rceil \f$
 *
 * This function rounds up to stay consistent with the QMC_CM4_ConvertRtcCounterValToMs function, i.e.,
 * QMC_CM4_ConvertRtcCounterValToMs(QMC_CM4_ConvertMsToRtcCounterVal(ms)) = ms should hold.
 *
 * The maximum supported input value without an overflow is
 * \f$ \lfloor (\mathrm{INT64\_MAX} - 999) / 32768 \rfloor = 281474976710655 \f$
 * hence the max. accepted timestamp in milliseconds is capable of representing
 * approximately \f$ 281474976710655/1000/3600/24/366 \approx 8901 \f$ years.
 *
 * This function is reentrant and can be used concurrently as long as pRtcValue points to
 * unique locations.
 *
 * @startuml
 * start
 * if (pRtcValue == NULL) then (true)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * if ((timeMs > ((INT64_MAX - 999) >> 15)) || (timeMs < 0)) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :~*pRtcValue = timeMs << 15
 * *pRtcValue = (*pRtcValue + 999) / 1000
 * return kStatus_QMC_Ok;
 * stop
 * @enduml
 *
 * @param timeMs The timestamp in milliseconds that should be converted to a value
 * compatible with a SNVS RTC counter.
 * @param pRtcValue Pointer to a variable which gets the resulting RTC counter value
 * assigned. The result has only been written if this function returns kStatus_QMC_Ok.
 * @return A qmc_status_t status code
 * @retval kStatus_QMC_ErrRange
 * The given timestamp value was below zero or too large so that an
 * overflow would have occurred. The result data at *pRtcValue
 * was not modified.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully, the result has been written to *pRtcValue.
 */
static qmc_status_t ConvertMsToRtcCounterVal(int64_t timeMs, int64_t *pRtcValue)
{
    assert(NULL != pRtcValue);

    if ((timeMs > ((INT64_MAX - 999) >> 15U)) || (timeMs < 0))
    {
        return kStatus_QMC_ErrRange;
    }

    /* *pRtcValue = timeMs * rtcFreq */
    *pRtcValue = timeMs << 15U;
    /* *pRtcValue = (timeMs * rtcFreq + 999) / 1000 */
    *pRtcValue = (*pRtcValue + 999) / 1000;

    return kStatus_QMC_Ok;
}

qmc_status_t QMC_CM4_GetRtcTimeIntsDisabled(qmc_timestamp_t *const pTime)
{
    qmc_status_t ret = kStatus_QMC_Err;

    if (NULL == pTime)
    {
        return kStatus_QMC_ErrArgInvalid;
    }

    /* get offset */
    int64_t offset = gs_snvsStateModified.srtcOffset;

    /* get the current RTC value
     * rtcValueIs is always positive by definition */
    int64_t rtcValueIs = 0;
    ret                = HAL_GetSrtcCount(&rtcValueIs);
    if (kStatus_QMC_Ok != ret)
    {
        return ret;
    }
    assert(rtcValueIs > 0);

    /* check for overflow
     * can only overflow in positive direction as rtcValueIs is always positive */
    if (offset > INT64_MAX - rtcValueIs)
    {
        return kStatus_QMC_ErrRange;
    }

    /* get the current time in ms */
    int64_t rtcValueReal = rtcValueIs + offset;
    int64_t timeMs       = 0;
    ret                  = ConvertRtcCounterValToMs(rtcValueReal, &timeMs);
    if (kStatus_QMC_Ok != ret)
    {
        return ret;
    }

    pTime->milliseconds = timeMs % 1000;
    pTime->seconds      = timeMs / 1000;

    return kStatus_QMC_Ok;
}

qmc_status_t QMC_CM4_SetRtcTimeIntsDisabled(qmc_timestamp_t *const pTime)
{
    qmc_status_t ret = kStatus_QMC_Err;

    /* check arguments */
    if (NULL == pTime)
    {
        return kStatus_QMC_ErrArgInvalid;
    }
    /* check for overflow */
    if (pTime->seconds > ((uint64_t)INT64_MAX - pTime->milliseconds) / 1000U)
    {
        return kStatus_QMC_ErrRange;
    }

    /* get the current RTC value as soon as possible
     * rtcValueIs is always positive by definition */
    int64_t rtcValueIs = 0;
    /* might time out in exceptional cases */
    ret = HAL_GetSrtcCount(&rtcValueIs);
    if (kStatus_QMC_Ok != ret)
    {
        return ret;
    }
    assert(rtcValueIs > 0);

    /* get the value the RTC should be after setting */
    int64_t timeMs         = pTime->seconds * 1000U + pTime->milliseconds;
    int64_t rtcValueShould = 0;
    ret                    = ConvertMsToRtcCounterVal(timeMs, &rtcValueShould);
    if (kStatus_QMC_Ok != ret)
    {
        return ret;
    }

    /* calculate offset (different signs -> can not overflow) */
    int64_t offset = rtcValueShould - rtcValueIs;
    gs_snvsStateModified.srtcOffset = offset;

    return kStatus_QMC_Ok;
}

/*!
 * @brief Helper for dealing with invalid / unknown reset causes.
 *
 * Invalid / unknown reset causes are translated to kQMC_ResetSecureWd.
 *
 * @param[in] resetCause The qmc_reset_cause_id_t value to constrain.
 * @return The constrained qmc_reset_cause_id_t value (kQMC_ResetSecureWd for unknown input causes).
 *
 */
static qmc_reset_cause_id_t constrainResetCause(qmc_reset_cause_id_t resetCause)
{
    if((kQMC_ResetNone != resetCause) && (kQMC_ResetRequest != resetCause) &&
       (kQMC_ResetFunctionalWd != resetCause) && (kQMC_ResetSecureWd != resetCause))
    {
        return kQMC_ResetSecureWd;
    }
    else
    {
        return resetCause;
    }
}

/*!
 * @brief Helper for returning the higher priority reset cause out of two options.
 *
 * The priority precedence is kQMC_ResetSecureWd > kQMC_ResetFunctionalWd >
 * kQMC_ResetRequest > kQMC_ResetNone.
 *
 * Unknown / unsupported reset causes are treated as kQMC_ResetSecureWd.
 *
 * @param[in] a The first qmc_reset_cause_id_t value.
 * @param[in] b The second qmc_reset_cause_id_t value.
 * @return The qmc_reset_cause_id_t value with the higher priority.
 *
 */
static qmc_reset_cause_id_t getHighestPriorityResetCause(qmc_reset_cause_id_t a, qmc_reset_cause_id_t b)
{
    /* in case we would not get a result (programming error) just return kQMC_ResetSecureWd
     * this would trigger a reboot into recovery mode */
    qmc_reset_cause_id_t result = kQMC_ResetSecureWd;

    /* constrain reset causes (unknown ones are translated to kQMC_ResetSecureWd) */
    a = constrainResetCause(a);
    b = constrainResetCause(b);

    /* start with highest go to lowest */
    if ((kQMC_ResetSecureWd == a) || (kQMC_ResetSecureWd == b))
    {
        result = kQMC_ResetSecureWd;
    }
    else if ((kQMC_ResetFunctionalWd == a) || (kQMC_ResetFunctionalWd == b))
    {
        result = kQMC_ResetFunctionalWd;
    }
    else if ((kQMC_ResetRequest == a) || (kQMC_ResetRequest == b))
    {
        result = kQMC_ResetRequest;
    }
    else if ((kQMC_ResetNone == a) || (kQMC_ResetNone == b))
    {
        result = kQMC_ResetNone;
    }

    return result;
}

void QMC_CM4_ResetSystemIntsDisabled(qmc_reset_cause_id_t resetCause)
{
    /* only overwrite resetCause if a reset cause  with a higher priority is given */
    gs_snvsStateModified.resetCause = getHighestPriorityResetCause(resetCause, gs_snvsStateModified.resetCause);

    /* if the expiration of the secure watchdog is the reason for the reset, we should make sure its
     * backed-up state is reset */
    if (kQMC_ResetSecureWd == gs_snvsStateModified.resetCause)
    {
        gs_snvsStateModified.fwuStatus |= kFWU_AwdtExpired;
        gs_snvsStateModified.wdTimerBackup = 0U;
        gs_snvsStateModified.wdStatus      = 0U;
    }
    /* if this is a regular reset request, clear a possible set kFWU_AwdtExpired flag
     * (this is done to leave restricted mode after maintenance completed) */
    else if (kQMC_ResetRequest == gs_snvsStateModified.resetCause)
    {
        gs_snvsStateModified.fwuStatus &= ~kFWU_AwdtExpired;
    }

    /* before a reset we always sync the current SNVS state to avoid
     * synchronization problems with QMC_CM4_SyncSnvsRpcStateMain()
     * write fwu status and reset cause first as they modify boot behavior and should
     * be set before modifying the watchdog backup (helps avoiding conflicting
     * states of both) */
    HAL_SetFwuStatus(gs_snvsStateModified.fwuStatus);
    HAL_SetResetCause(gs_snvsStateModified.resetCause);
    HAL_SetWdTimerBackup(gs_snvsStateModified.wdTimerBackup);
    HAL_SetWdStatus(gs_snvsStateModified.wdStatus);
    HAL_SetSrtcOffset(gs_snvsStateModified.srtcOffset);
    /* as we anyhow reset afterwards, writing outputs does make no sense */
    /* hardware testing showed that on the CM4 the last write to a SNVS LPGPR register
     * is not committed to hardware if a reset is performed directly afterwards (confirmed with MWE)
     * a dummy read from any SNVS LPGPR register seems to mitigate this issue
     * (ARM barriers did not help)
     */
    (void)HAL_GetWdStatus();
    HAL_ResetSystem();
    /* halt if system reset would fail (should not happen) */
    DEBUG_LOG_E(DEBUG_M4_TAG "QMC_CM4_ResetSystemIntsDisabled: Reset failed!\r\n");
    while(FOREVER());
}

qmc_fw_update_state_t QMC_CM4_GetFwUpdateState(void)
{
    return gs_snvsStateModified.fwuStatus;
}

qmc_reset_cause_id_t QMC_CM4_GetResetCause(void)
{
    return gs_previousResetCause;
}

void QMC_CM4_CommitFwUpdateIntsDisabled(void)
{
    gs_snvsStateModified.fwuStatus |= kFWU_Commit;
}

void QMC_CM4_RevertFwUpdateIntsDisabled(void)
{
    gs_snvsStateModified.fwuStatus |= kFWU_Revert;
}

void QMC_CM4_SyncSnvsRpcStateMain(void)
{
    qmc_status_t callReturn = kStatus_QMC_Err;
    (void) callReturn;

    QMC_CM4_SyncSnvsStorageMain();

#if FEATURE_SECURE_WATCHDOG
    /* process delayed AWDG commands */
    if (RPC_ProcessPendingSecureWatchdogCall(&callReturn))
    {
        HAL_EnterCriticalSectionNonISR();
        RPC_FinishSecureWatchdogCall(callReturn);
        HAL_ExitCriticalSectionNonISR();
    }
#endif
}

void QMC_CM4_SyncSnvsGpioIntsDisabled(void)
{
    if (gs_snvsStateModified.gpioOutputStatus != gs_snvsStateHW.gpioOutputStatus)
    {
        HAL_SetSnvsGpio13Outputs(gs_snvsStateModified.gpioOutputStatus);
        gs_snvsStateHW.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus;
    }
}

/*
 * Interrupt Handlers
 */
void QMC_CM4_HandleSystickISR(void)
{
    HAL_GpioTimerHandler();
    /* check if debounced input value is different then the previous input state */
    uint32_t newHalGpioInputs = HAL_GetSnvsGpio13();
    if (gs_halGpioInputState != newHalGpioInputs)
    {
        RPC_NotifyCM7AboutGpioChange(HalGpioState2QmcCm4GpioInput(newHalGpioInputs));
        gs_halGpioInputState = newHalGpioInputs;
    }
}

void QMC_CM4_HandleUserInputISR(void)
{
    HAL_GpioInterruptHandler();
}

void QMC_CM4_HandleWatchdogTickISR(void)
{
    bool resetSystem             = false;
    lwdg_tick_status_t tickState = kStatus_LWDG_TickErr;
#if FEATURE_SECURE_WATCHDOG
    uint16_t awdgBackupTicksToTimeout = 0U;
#endif
    qmc_status_t notifyRet = kStatus_QMC_Err;
    (void)notifyRet;

    /* tick functional watchdogs */
    tickState = LWDGU_Tick(&gs_fwdgUnit);
    /* can not happen */
    assert(kStatus_LWDG_TickPreviouslyExpired != tickState);
    /* grace period just started */
    if (kStatus_LWDG_TickJustStarted == tickState)
    {
        /* only update reset cause if kQMC_ResetFunctionalWd has a higher priority
         * than a possible current set reset cause */
        gs_snvsStateModified.resetCause =
            getHighestPriorityResetCause(kQMC_ResetFunctionalWd, gs_snvsStateModified.resetCause);
        /* if the notification does not work, the system is still reset
         * (but the CM7 might not have enough time for final tasks) */
        notifyRet = RPC_NotifyCM7AboutReset(kQMC_ResetFunctionalWd);
        assert(kStatus_QMC_Ok == notifyRet);
    }
    /* grace period expired -> reset system */
    else if (kStatus_LWDG_TickJustExpired == tickState)
    {
        resetSystem = true;
    }

#if FEATURE_SECURE_WATCHDOG
    /* tick authenticated watchdog */
    tickState = AWDG_Tick();
    /* can not happen */
    assert(kStatus_LWDG_TickPreviouslyExpired != tickState);
    /* AWDG is always running after initialization! */
    awdgBackupTicksToTimeout = ConvertAuthenticatedWatchdogTicksToBackupValue(AWDG_GetRemainingTicks());
    /* backup AWDG state
     * authenticated watchdog has not expired yet */
    if (awdgBackupTicksToTimeout > 0U)
    {
        /* backup value is rounded up
         * this guarantees we never store zero without setting the appropriate reset cause
         * (at restoring the backup value is always decreased to ensure we always eventually expire)
         */
        gs_snvsStateModified.wdTimerBackup = awdgBackupTicksToTimeout;
        /* sync AWDG backup directly to hardware, else an attacker could starve the sync in the main loop by
         * retriggering the communication interrupt
         * by triggering a sync here the most an attacker can gain is another reboot where the secure watchdog
         * will immediately expire and a reboot to recovery mode is triggered */
        if (gs_snvsStateModified.wdTimerBackup != gs_snvsStateHW.wdTimerBackup)
        {
            /* can not interfere with the main sync code as we immediately mark the value up-to-date
             * wdTimerBackup is only modified in this function while interrupts are enabled and the program keeps running
             * hence, in the main loop, HAL_SetWdTimerBackup will never be called */
            HAL_SetWdTimerBackup(gs_snvsStateModified.wdTimerBackup);
            gs_snvsStateHW.wdTimerBackup = gs_snvsStateModified.wdTimerBackup;
        }
    }
    /* authenticated watchdog expired
     * grace period just started */
    else if (kStatus_LWDG_TickJustStarted == tickState)
    {
        /* set appropriate reset cause, reset backed-up state, notify CM7 */
        HandleAuthenticatedWatchdogExpirationIntsDisabled();
    }
    /* grace period expired -> reset backed-up state and system */
    else if (kStatus_LWDG_TickJustExpired == tickState)
    {
        resetSystem = true;
    }
#endif

    /* reset system if needed */
    if (true == resetSystem)
    {
        QMC_CM4_ResetSystemIntsDisabled(gs_snvsStateModified.resetCause);
    }
    /* kick hardware watchdog */
    HAL_KickHWWatchdog();
}

void QMC_CM4_HandleHardwareWatchdogISR(void)
{
    /* we try to perform the reset ourselves
     * so that we can set the battery-backed SNVS registers to well-defined values */

    /* do not trust the SNVS software mirrors, just set the state in the hardware SNVS directly
     * (might happen before everything is initialized correctly)
     * use kFWU_AwdtExpired as FWU status and kQMC_ResetSecureWd as reset cause to boot into recovery mode
     * + reset authenticated watchdog state backup */
    HAL_SetFwuStatus(HAL_GetFwuStatus() | kFWU_AwdtExpired);
    HAL_SetResetCause(kQMC_ResetSecureWd);
    HAL_SetWdTimerBackup(0U);
    HAL_SetWdStatus(0U);
    /* hardware testing showed that on the CM4 the last write to a SNVS LPGPR register
     * is not committed to hardware if a reset is performed directly afterwards (confirmed with MWE)
     * a dummy read from any SNVS LPGPR register seems to mitigate this issue
     * (ARM barriers did not help)
     */
    (void)HAL_GetWdStatus();
    /* we do not reset the interrupt status flag as we anyhow reboot */
    HAL_ResetSystem();
    /* halt if system reset would fail (should not happen) */
    DEBUG_LOG_E(DEBUG_M4_TAG "QMC_CM4_HandleHardwareWatchdogISR: Reset failed!\r\n");
    while(FOREVER());
}

/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
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
 *      - See @ref qmc_cm4_features_config.h header file for configuration.
 *  - Managing the authenticated watchdog (configuration, ticking, kicking, reset notification, reset)
 *      - See @ref qmc_cm4_features_config.h header file for configuration.
 *  - Setting the user outputs (GPIO13), notifying CM7 about user input (GPIO13) changes
 *      - See @ref qmc_cm4_features_config.h for initial output value
 *  - Setting and getting the current time (SRTC)
 *  - Writing and reading the firmware update status and the reset cause to and from the battery-backed SNVS LP register
 *  - Performing system resets
 *  - Storing the system state in the SNVS
 *      - State of AWDG
 *      - Firmware update state
 *      - Real-time clock offset (SRTC)
 *      - Reset cause
 *
 * Since it is assumed that any access to the SNVS domain is slow, the required SNVS content is mirrored within this
 * module. There are two mirrors of all required SNVS fields.
 * In the main processing loop the mirrors are checked for differences.
 * If the mirrors are different, the changed items are synchronized with the SNVS hardware registers.
 * NOTE: The state backup of the secure watchdog timer is still written to hardware in an ISR out of security
 * reasons (attacker should not be able to extend timeout by blocking synchronization).
 *
 * The following code does not use interrupt nesting and correct functionality does depend on this
 * precondition! The maximum time spend in a interrupt (without invocations that lead to a reset) is 210us.
 * Hence, we can guarantee that the interrupt ticking the watchdogs works as expected (only the hardware watchdog
 * expiry interrupt has a higher priority, but that is excluded as it is executed only if the program can not be
 * trusted anymore).
 * Interrupt invocations which led to a reset were excluded, because even if the ISR processing would have been
 * faster, just the reset would have happened faster. In any case (future) pending ISRs are not executed.
 * For the timeout backup of the AWDG this is also not critical as it is anyhow halved at each reboot.
 * In the worst case ticking might happen around 210us later as expected, but that can be neglected
 * in comparison to the wanted timeout intervals (multiple hours).
 *
 * For reentrancy and concurrency assumptions, check the individual API descriptions.
 *
 * Note that the system should have a charged coin cell inserted, otherwise the secure watchdog will always indicate
 * an expiry so that the cyber resiliency feature is not bypassed.
 *
 * Limitations:
 *  - The RTC offset needs two SNVS LPGPR registers, hence a power-loss / reset between
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
 * Definitions
 ******************************************************************************/
#define RTC_FREQ (32768U) /*!< Frequency of the real time clock in Hz */

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
    gs_fwdgs[QMC_CM4_FWDGS_COUNT];                       /*!< functional watchdogs, see qmc_cm4_features_config.h */
STATIC_TEST_VISIBLE logical_watchdog_unit_t gs_fwdgUnit; /*!< functional watchdog unit, see qmc_cm4_features_config.h */

static volatile uint32_t gs_halGpioInputState; /*!< the current debounced GPIO input state from the HAL module */

static volatile qmc_reset_cause_id_t gs_previousResetCause; /*!< the buffered previous reset cause */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Helper function for initializing the functional watchdogs.
 *
 * Must be called before the communication and the tick interrupt are enabled.
 *
 * @startuml
 * start
 * :uint32_t fwdgsTimeoutMs[QMC_CM4_FWDGS_COUNT] = QMC_CM4_FWDGS_TIMEOUT_MS;
 * if (LWDGU_Init(&gs_fwdgUnit, QMC_CM4_FWDGS_GRACE_PERIOD_MS, QMC_CM4_WDG_TICK_FREQUENCY_HZ, gs_fwdgs, QMC_CM4_FWDGS_COUNT)) then (fail)
 *   :return kStatus_QMC_Err;
 *   stop
 * endif
 * repeat :For id in 0 ... QMC_CM4_FWDGS_COUNT - 1;
 *   if (LWDGU_InitWatchdog(&gs_fwdgUnit, id, fwdgsTimeoutMs[id])) then (fail)
 *     :return kStatus_QMC_Err;
 *     stop
 *   endif
 * repeat while (next id)
 * :return kStatus_QMC_Ok;
 * stop
 * @enduml
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
 * @brief Helper function for converting the given AWDG ticks to their backup format for
 * the battery-backed SNVS register.
 *
 * \f$awdgTicksBackup = \min\left(UINT16_{MAX}, \lceil\frac{awdgTicks}{2^{16}}\rceil\right)\f$
 * 
 * Rounds up to whole integer backup values.
 * Therefore, the returned value is only 0 if the given awdgTicks value was 0.
 *
 * @param[in] awdgTicks The AWDG ticks to convert.
 * @return uint16_t The backup value for the given AWDG ticks, 0 if the given ticks value was 0.
 */
static uint16_t ConvertAuthenticatedWatchdogTicksToBackupValue(uint32_t awdgTicks)
{
    /* overflow check */
    if (awdgTicks > (UINT32_MAX - QMC_CM4_AWDG_TICKS_BACKUP_MAX))
    {
        /* return max backup value */
        return UINT16_MAX;
    }
    else
    {
        /* round up to the next whole backup value */
        return (uint16_t)((awdgTicks + QMC_CM4_AWDG_TICKS_BACKUP_MAX) >> QMC_CM4_AWDG_TICKS_BACKUP_SHIFT);
    }
}

/*!
 * @brief Helper function for handling the expiration of the authenticated watchdog.
 *
 * Sets correct SNVS state (fwuStatus, resetCause, WD state) and notifies M7 about upcoming reset.
 * 
 * Do only call from interrupts or before interrupts are enabled!
 * 
 * @startuml
 * start
 * :gs_snvsStateModified.fwuStatus |= kFWU_AwdtExpired
 * gs_snvsStateModified.resetCause = kQMC_ResetSecureWd
 * gs_snvsStateModified.wdTimerBackup = 0
 * gs_snvsStateModified.wdStatus      = 0;
 * :RPC_NotifyCM7AboutReset(kQMC_ResetSecureWd);
 * stop
 * @enduml
 */
static void HandleAuthenticatedWatchdogExpirationIntsDisabled(void)
{
    qmc_status_t notifyRet = kStatus_QMC_Err;
    (void)notifyRet;

    /* remember that the secure watchdog expired */
    gs_snvsStateModified.fwuStatus |= (uint8_t)kFWU_AwdtExpired;
    gs_snvsStateModified.resetCause = kQMC_ResetSecureWd;
    /* reset watchdog status in SNVS */
    gs_snvsStateModified.wdTimerBackup = 0U;
    gs_snvsStateModified.wdStatus      = 0U;
    /* if the notification does not work, the system is still reset
     * (but the CM7 might not have enough time for final tasks) */
    notifyRet = RPC_NotifyCM7AboutReset(kQMC_ResetSecureWd);
    assert(kStatus_QMC_Ok == notifyRet);
    (void) notifyRet;
}

/*!
 * @brief Helper function for initializing the authenticated watchdog.
 *
 * Must be called before the communication and the tick interrupt are enabled.
 * Zeros the RNG seed and PK after initialization is complete.
 * 
 * If the watchdog was running before this boot, its state is restored.
 * The remaining AWDG ticks in the SNVS are divided by two (rounding down) before
 * restoring, so that an attacker can not trigger endless reboot loops 
 * without starting recovery mode.
 *
 * @startuml
 * start
 * :ticksToTimeout = 0;
 * if (gs_snvsStateModified.wdStatus) then (!= 0)
 *  :gs_snvsStateModified.wdTimerBackup = gs_snvsStateModified.wdTimerBackup / 2
 *  ticksToTimeout = gs_snvsStateModified.wdTimerBackup << QMC_CM4_AWDG_TICKS_BACKUP_SHIFT;
 * endif
 * :awdgInitRet = AWDG_Init(QMC_CM4_AWDG_INITIAL_TIMEOUT_MS, QMC_CM4_AWDG_GRACE_PERIOD_MS, QMC_CM4_WDG_TICK_FREQUENCY_HZ, ticksToTimeout, gs_snvsStateModified.wdStatus, g_awdgInitDataSHM.rngSeed, g_awdgInitDataSHM.rngSeedLen, g_awdgInitDataSHM.pk, g_awdgInitDataSHM.pkLen)
 * ~*g_awdgInitDataSHM.rngSeed = 0
 * ~*g_awdgInitDataSHM.pk = 0;
 * if (awdgInitRet) then (kStatus_AWDG_InitInitializedNew)
 *   :awdgTickRet = AWDG_Tick()
 *   ticksToTimeout = AWDG_GetRemainingTicks();
 *   :gs_snvsStateModified.wdTimerBackup = ConvertAuthenticatedWatchdogTicksToBackupValue(ticksToTimeout)
 *   gs_snvsStateModified.wdStatus = 1
 *   gs_snvsStateModified.fwuStatus |= kFWU_AwdtExpired;
 *   :ret = kStatus_QMC_Ok;
 * else if (awdgInitRet) then (kStatus_AWDG_InitInitializedResumed)
 *   :awdgTickRet = AWDG_Tick();
 *   :ret = kStatus_QMC_Ok;
 * else
 *   :awdgTickRet = kStatus_LWDG_TickErr;
 *   :ret = kStatus_QMC_Err;
 * endif
 * if (awdgTickRet) then (kStatus_LWDG_TickJustStarted)
 *   :HandleAuthenticatedWatchdogExpirationIntsDisabled();
 * else if (awdgTickRet) then (kStatus_LWDG_TickJustExpired)
 *   :QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
 * endif
 * :return ret;
 * stop
 * @enduml
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Err
 * An error at initializing the authenticated watchdog occurred.
 * @retval kStatus_QMC_Ok
 * The authenticated watchdog was initialized successfully.
 */
static qmc_status_t InitializeAuthenticatedWatchdogHelper(void)
{
    qmc_status_t ret               = kStatus_QMC_Err;
    awdg_init_status_t awdgInitRet = kStatus_AWDG_InitError;
    lwdg_tick_status_t awdgTickRet = kStatus_LWDG_TickErr;
    uint32_t ticksToTimeout        = 0U;

    /* watchdog was running before reset
     * restore timeout ticks value and reduce it
     * (a possible expiration is handled further below)
     */
    if (gs_snvsStateModified.wdStatus > 0U)
    {
        /* sync is performed at the end of QMC_CM4_Init() or if a reset is triggered
         * always reduce count (divide by 2, rounding down) here
         * so that we expire eventually if we are in a reboot loop */
        gs_snvsStateModified.wdTimerBackup >>= 1U;
        /* stored value was shifted, undo this */
        ticksToTimeout = (uint32_t)gs_snvsStateModified.wdTimerBackup << QMC_CM4_AWDG_TICKS_BACKUP_SHIFT;
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
            /* save authenticated watchdog state if freshly initialized */
            ticksToTimeout                     = AWDG_GetRemainingTicks();
            gs_snvsStateModified.wdTimerBackup = ConvertAuthenticatedWatchdogTicksToBackupValue(ticksToTimeout);
            gs_snvsStateModified.wdStatus      = 1U;
            /* set expiry flag if secure watchdog is freshly initialized which happens in the following cases:
             * 	- first boot after replacing the coin cell or after power cycles if coin cell is empty
             * 		- ensures the cyber resiliency feature can not be bypassed if coin cell is empty
             * 	- after the secure watchdog expired (in this case the flag is anyhow already set) */
            gs_snvsStateModified.fwuStatus |= (uint8_t)kFWU_AwdtExpired;
            ret                                = kStatus_QMC_Ok;
            break;
        case kStatus_AWDG_InitInitializedResumed:
            /* tick once (AWDG_Init() uses LWDGU_KickOne() internally which increases the defined timeout by 1) */
            awdgTickRet = AWDG_Tick();
            ret         = kStatus_QMC_Ok;
            break;
        default:
            DEBUG_LOG_E(DEBUG_M4_TAG "AWDG_Init failed (%d)!\r\n", awdgInitRet);
            awdgTickRet = kStatus_LWDG_TickErr;
            ret = kStatus_QMC_Err;
            break;
    }

    assert((kStatus_LWDG_TickRunning != awdgTickRet) && (kStatus_LWDG_TickPreviouslyExpired != awdgTickRet));
    /* AWDG already expired and grace period started
     * set appropriate reset cause, reset the watchdog SNVS state and notify CM7
     * reset is performed after the grace period in the ticking interrupt */
    if (kStatus_LWDG_TickJustStarted == awdgTickRet)
    {
        /* set appropriate reset cause, reset SNVS state, notify CM7 */
        HandleAuthenticatedWatchdogExpirationIntsDisabled();
    }
    /* no grace period -> reset SNVS state and system */
    else if (kStatus_LWDG_TickJustExpired == awdgTickRet)
    {
        QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
    }
    else
    {
        /* no action required, "else" added for compliance */
        ;
    }

    return ret;
}
#endif

/*!
 * @brief Helper function for translating the HAL input state to the QMC format.
 *
 * @param[in] halGpioRegister GPIO input states in HAL format (hal_snvs_gpio_pin_t).
 * @return The GPIO input states in QMC format.
 */
static uint8_t HalGpioState2QmcCm4GpioInput(uint32_t halGpioRegister)
{
    uint8_t qmcCm4Inputs = 0U;
    if (halGpioRegister & ((uint32_t)1U << kHAL_SnvsUserInput0))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT4_DATA;
    }
    if (halGpioRegister & ((uint32_t)1U << kHAL_SnvsUserInput1))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT5_DATA;
    }
    if (halGpioRegister & ((uint32_t)1U << kHAL_SnvsUserInput2))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT6_DATA;
    }
    if (halGpioRegister & ((uint32_t)1U << kHAL_SnvsUserInput3))
    {
        qmcCm4Inputs = qmcCm4Inputs | RPC_DIGITAL_INPUT7_DATA;
    }
    return qmcCm4Inputs;
}

/*!
 * @brief Loads the system state from SNVS into both software mirrors and performs sanity checks on the loaded values.
 *
 * If any sanity check fails, SNVS is reset to zero (user should reboot system into recovery mode)!
 * All data which is stored in the battery-backed SNVS GPR registers should work
 * correctly with an initial value of zero (value after SNVS LP POR)!
 *
 * Not reentrant, must be called before interrupts are enabled!
 *
 * @startuml
 * start
 * :isSane = true;
 * :loadedSnvsState.wdTimerBackup = HAL_GetWdTimerBackup()
 * loadedSnvsState.wdStatus = HAL_GetWdStatus()
 * loadedSnvsState.fwuStatus = HAL_GetFwuStatus()
 * loadedSnvsState.srtcOffset = HAL_GetSrtcOffset()
 * loadedSnvsState.resetCause = HAL_GetResetCause()
 * loadedSnvsState.gpioOutputStatus = HAL_GetSnvsGpio13Outputs();
 * if () then ((loadedSnvsState.resetCause == kQMC_ResetSecureWd) && (loadedSnvsState.wdStatus > 0U))
 *   :isSane = false;
 * else if () then (loadedSnvsState.fwuStatus & ~QMC_CM4_VALID_FWU_STATE_MASK != 0)
 *   :isSane = false;
 * else if () then (loadedSnvsState.resetCause not from qmc_reset_cause_id_t)
 *   :isSane = false;
 * endif
 * if (isSane) then (true)
 *   :gs_snvsStateHW = loadedSnvsState
 *   gs_snvsStateModified = gs_snvsStateHW;
 *   :gs_previousResetCause = gs_snvsStateHW.resetCause
 *   gs_snvsStateModified.resetCause = kQMC_ResetNone;
 *   :return kStatus_QMC_Ok;
 *   stop
 * else
 *   :HAL_SetWdTimerBackup(0U)
 *   HAL_SetWdStatus(0U)
 *   HAL_SetFwuStatus(0U)
 *   HAL_SetSrtcOffset(0U)
 *   HAL_SetResetCause(0U);
 *   :return kStatus_QMC_Err;
 *   stop
 * endif
 * @enduml
 * 
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok The system state was successfully restored from SNVS.
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
    else if (0U != (loadedSnvsState.fwuStatus & ~QMC_CM4_VALID_FWU_STATE_MASK))
    {
        isSane = false;
    }
    /* check reset cause */
    else if((kQMC_ResetNone != loadedSnvsState.resetCause) && (kQMC_ResetRequest != loadedSnvsState.resetCause) &&
       (kQMC_ResetFunctionalWd != loadedSnvsState.resetCause) && (kQMC_ResetSecureWd != loadedSnvsState.resetCause))
    {
        isSane = false;
    }

    if (true == isSane)
    {
        /* everything OK, latch values */
        gs_snvsStateHW = loadedSnvsState;
        gs_snvsStateModified = gs_snvsStateHW;

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
 * Without GPIO13!
 * 
 * Intended to be called from the main loop, do not call from an interrupt!
 * The WD state is also written outside of this function in the watchdog ticking ISR
 * (after the ISR started). By moving the sync to the ISR we avoid the case where
 * an attacker blocks this function by continuously triggering interrupts and
 * hence no update of the WD state would be possible anymore!
 *
 * Not reentrant, only use once!
 */
static void QMC_CM4_SyncSnvsStorageMain(void)
{
    qmc_cm4_snvs_mirror_data_diff_t diff = {0};

    /* check for differences between the software mirrors and latch them in one batch
     * (ensures that values that were changed together in an interrupt are latched together)
     */
    HAL_EnterCriticalSectionNonISR();
    uint16_t wdTimerBackupModified = gs_snvsStateModified.wdTimerBackup;
    uint16_t wdTimerBackupHW = gs_snvsStateHW.wdTimerBackup;
    if (wdTimerBackupModified != wdTimerBackupHW)
    {
        gs_snvsStateHW.wdTimerBackup = wdTimerBackupModified;
        diff.wdTimerBackup           = true;
    }
    uint8_t wdStatusModified = gs_snvsStateModified.wdStatus;
    uint8_t wdStatusHW = gs_snvsStateHW.wdStatus;
    if (wdStatusModified != wdStatusHW)
    {
        gs_snvsStateHW.wdStatus = wdStatusModified;
        diff.wdStatus           = true;
    }
    uint8_t fwuStatusModified = gs_snvsStateModified.fwuStatus;
    uint8_t fwuStatusHW = gs_snvsStateHW.fwuStatus;
    if (fwuStatusModified != fwuStatusHW)
    {
        gs_snvsStateHW.fwuStatus = fwuStatusModified;
        diff.fwuStatus           = true;
    }
    int64_t srtcOffsetModified = gs_snvsStateModified.srtcOffset;
    int64_t srtcOffsetHW = gs_snvsStateHW.srtcOffset;
    if (srtcOffsetModified != srtcOffsetHW)
    {
        gs_snvsStateHW.srtcOffset = srtcOffsetModified;
        diff.srtcOffset           = true;
    }
    uint8_t resetCauseModified = gs_snvsStateModified.resetCause;
    uint8_t resetCauseHW = gs_snvsStateHW.resetCause;
    if (resetCauseModified != resetCauseHW)
    {
        gs_snvsStateHW.resetCause = resetCauseModified;
        diff.resetCause           = true;
    }
    HAL_ExitCriticalSectionNonISR();

    /* write to hardware (with enabled interrupts as writing is slow) */
    /* write FWU status and reset cause first as they modify boot behavior and should
     * be set before modifying the watchdog state (helps avoiding conflicting
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
    /* without interrupts, except for hardware watchdog interrupt */
    HAL_InitHWWatchdog();
    HAL_InitSrtc();
    HAL_InitRtc();
    HAL_SnvsGpioInit(QMC_CM4_SNVS_USER_OUTPUTS_INIT_STATE);
    HAL_InitMcuTemperatureSensor();
      
    /* if they bounce already at the beginning we just read an intermediate state
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

    /* sync possibly changed system state to SNVS before communication with CM7 is allowed */
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
    if(kStatus_QMC_Ok != HAL_InitSysTick())
    {
        DEBUG_LOG_E(DEBUG_M4_TAG "QMC_CM4_Init: SysTick initialization failed!\r\n");
        return kStatus_QMC_Err;
    }

    return kStatus_QMC_Ok;
}

qmc_status_t QMC_CM4_KickFunctionalWatchdogIntsDisabled(const uint8_t id)
{
    qmc_status_t ret              = kStatus_QMC_Err;

    lwdg_kick_status_t kickStatus = LWDGU_KickOne(&gs_fwdgUnit, id);
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

    /* AWDT kicked successfully -> reset AWDT previous expiry flag */
    if (kStatus_QMC_Ok == ret)
    {
        HAL_EnterCriticalSectionNonISR();
        gs_snvsStateModified.fwuStatus &= (uint8_t)(~kFWU_AwdtExpired);
        HAL_ExitCriticalSectionNonISR();
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
        gs_snvsStateModified.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus | ((uint32_t)1U << (uint8_t)pin);
    }
    else
    {
        gs_snvsStateModified.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus & ~((uint32_t)1U << (uint8_t)pin);
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
 * approximately \f$ 9223372036854775/32768/3600/24/366 \approx 8901 \f$
 * years.
 *
 * This function is reentrant and can be used concurrently as long as pTimeMs points to
 * unique locations.
 *
 * @startuml
 * start
 * if ((rtcValue > (INT64_MAX / 1000)) || (rtcValue < 0)) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :~*pTimeMs = rtcValue * 1000
 * *pTimeMs = *pTimeMs / RTC_FREQ
 * return kStatus_QMC_Ok;
 * stop
 * @enduml
 *
 * @param[in] rtcValue A SNVS RTC counter value which should be converted to a timestamp
 * in milliseconds.
 * @param[out] pTimeMs Pointer to a variable which gets the resulting timestamp in milliseconds
 * assigned. The result has only been written if this function returns kStatus_QMC_Ok.
 * The resulting timestamp value is always positive.
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
    *pTimeMs = *pTimeMs / RTC_FREQ;

    return kStatus_QMC_Ok;
}

/*!
 * @brief Transforms a timestamp in milliseconds to a value compatible with a SNVS
 * RTC counter.
 *
 * Transformation equation: \f$ rtcValue = \lceil timeMs / 1000 * 32768 Hz \rceil \f$
 *
 * This function rounds up to stay consistent with the ConvertRtcCounterValToMs() function, i.e.,
 * ConvertRtcCounterValToMs(ConvertMsToRtcCounterVal(ms)) = ms should hold.
 *
 * The maximum supported input value without an overflow is
 * \f$ \lfloor (\mathrm{INT64\_MAX} - 999) / 32768 \rfloor = 281474976710655 \f$.
 * Hence, the max. accepted timestamp in milliseconds is capable of representing
 * approximately \f$ 281474976710655/1000/3600/24/366 \approx 8901 \f$ years.
 *
 * This function is reentrant and can be used concurrently as long as pRtcValue points to
 * unique locations.
 *
 * @startuml
 * start
 * if ((timeMs > ((INT64_MAX - 999) / RTC_FREQ)) || (timeMs < 0)) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :~*pRtcValue = timeMs * RTC_FREQ
 * *pRtcValue = (*pRtcValue + 999) / 1000
 * return kStatus_QMC_Ok;
 * stop
 * @enduml
 *
 * @param[in] timeMs The timestamp in milliseconds that should be converted to a value
 * compatible with a SNVS RTC counter.
 * @param[out] pRtcValue Pointer to a variable which gets the resulting RTC counter value
 * assigned. The result has only been written if this function returns kStatus_QMC_Ok.
 * The resulting RTC counter value is always positive.
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

    if ((timeMs > ((INT64_MAX - 999) / RTC_FREQ)) || (timeMs < 0))
    {
        return kStatus_QMC_ErrRange;
    }

    /* *pRtcValue = timeMs * rtcFreq */
    *pRtcValue = timeMs * RTC_FREQ;
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
    if (offset > (INT64_MAX - rtcValueIs))
    {
        return kStatus_QMC_ErrRange;
    }

    /* get the current time in ms */
    int64_t rtcValueReal = rtcValueIs + offset;
    int64_t timeMs       = 0;
    /* only returns positive timestamps, else fails */
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
    if (pTime->seconds > (((uint64_t)INT64_MAX - pTime->milliseconds) / 1000U))
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

    /* get the value the RTC should be after setting
     * rtcValueShould is always positive by definition */
    int64_t timeMs         = (pTime->seconds * 1000U) + pTime->milliseconds;
    int64_t rtcValueShould = 0;
    /* only returns positive RTC counter values, else fails */
    ret                    = ConvertMsToRtcCounterVal(timeMs, &rtcValueShould);
    if (kStatus_QMC_Ok != ret)
    {
        return ret;
    }

    /* calculate offset (operands positive, same bit widths -> can not overflow) */
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
static qmc_reset_cause_id_t ConstrainResetCause(qmc_reset_cause_id_t resetCause)
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
 * @brief Helper function for returning the higher priority reset cause out of two options.
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
static qmc_reset_cause_id_t GetHighestPriorityResetCause(qmc_reset_cause_id_t a, qmc_reset_cause_id_t b)
{
    /* in case we would not get a result (programming error) just return kQMC_ResetSecureWd
     * this would trigger a reboot into recovery mode */
    qmc_reset_cause_id_t result = kQMC_ResetSecureWd;
    /* constrain reset causes (unknown ones are translated to kQMC_ResetSecureWd) */
    qmc_reset_cause_id_t aConstrained = ConstrainResetCause(a);
    qmc_reset_cause_id_t bConstrained = ConstrainResetCause(b);

    /* start with highest go to lowest */
    if ((kQMC_ResetSecureWd == aConstrained) || (kQMC_ResetSecureWd == bConstrained))
    {
        result = kQMC_ResetSecureWd;
    }
    else if ((kQMC_ResetFunctionalWd == aConstrained) || (kQMC_ResetFunctionalWd == bConstrained))
    {
        result = kQMC_ResetFunctionalWd;
    }
    else if ((kQMC_ResetRequest == aConstrained) || (kQMC_ResetRequest == bConstrained))
    {
        result = kQMC_ResetRequest;
    }
    else if ((kQMC_ResetNone == aConstrained) || (kQMC_ResetNone == bConstrained))
    {
        result = kQMC_ResetNone;
    }
    else
    {
        /* only happens in case of code error, set reason to secure watchdog reset */
        result = kQMC_ResetSecureWd;
    }

    return result;
}

void QMC_CM4_ResetSystemIntsDisabled(qmc_reset_cause_id_t resetCause)
{
    /* only overwrite resetCause if a reset cause with a higher priority is given */
    gs_snvsStateModified.resetCause = GetHighestPriorityResetCause(resetCause, (qmc_reset_cause_id_t)gs_snvsStateModified.resetCause);

    /* if the expiration of the secure watchdog is the reason for the reset, 
     * we should set the correct state in the SNVS */
    if (kQMC_ResetSecureWd == gs_snvsStateModified.resetCause)
    {
        gs_snvsStateModified.fwuStatus |= (uint8_t)kFWU_AwdtExpired;
        gs_snvsStateModified.wdTimerBackup = 0U;
        gs_snvsStateModified.wdStatus      = 0U;
    }
    else
    {
        /* no action required, "else" added for compliance */
        ;
    }

    /* before a reset we always sync the current state to the SNVS to avoid
     * synchronization problems with QMC_CM4_SyncSnvsRpcStateMain()
     * write fwu status and reset cause first as they modify boot behavior and should
     * be set before modifying the watchdog backup (helps avoiding conflicting
     * states of both) */
    HAL_SetFwuStatus(gs_snvsStateModified.fwuStatus);
    HAL_SetResetCause(gs_snvsStateModified.resetCause);
    HAL_SetWdTimerBackup(gs_snvsStateModified.wdTimerBackup);
    HAL_SetWdStatus(gs_snvsStateModified.wdStatus);
    HAL_SetSrtcOffset(gs_snvsStateModified.srtcOffset);
    /* as we anyhow reset afterwards, setting GPIO outputs does make no sense */

    /* hardware testing showed that on the CM4 the last write to a SNVS LPGPR register
     * is not committed to hardware if a reset is performed directly afterwards (confirmed with MWE)
     * a dummy read from any SNVS LPGPR register seems to mitigate this issue
     * (ARM barriers did not help)
     */
    (void)HAL_GetWdStatus();
    HAL_ResetSystem();
    /* halt if system reset would fail (should not happen) */
    DEBUG_LOG_E(DEBUG_M4_TAG "QMC_CM4_ResetSystemIntsDisabled: Reset failed!\r\n");
    while(FOREVER())
    {
       /* no action required, added for compliance */
       ;
    }
}

qmc_fw_update_state_t QMC_CM4_GetFwUpdateStateIntsDisabled(void)
{
    return gs_snvsStateModified.fwuStatus;
}

qmc_reset_cause_id_t QMC_CM4_GetResetCause(void)
{
    return gs_previousResetCause;
}

void QMC_CM4_CommitFwUpdateIntsDisabled(void)
{
    gs_snvsStateModified.fwuStatus |= (uint8_t)kFWU_Commit;
}

void QMC_CM4_RevertFwUpdateIntsDisabled(void)
{
    gs_snvsStateModified.fwuStatus |= (uint8_t)kFWU_Revert;
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
    uint32_t gpioOutputStatusModified = gs_snvsStateModified.gpioOutputStatus;
    uint32_t gpioOutputStatusHW = gs_snvsStateHW.gpioOutputStatus;
    if (gpioOutputStatusModified != gpioOutputStatusHW)
    {
        HAL_SetSnvsGpio13Outputs(gpioOutputStatusModified);
        gs_snvsStateHW.gpioOutputStatus = gpioOutputStatusModified;
    }
}

/*
 * Interrupt Handlers
 */
void QMC_CM4_HandleSystickISR(void)
{
    HAL_GpioTimerHandler();
    /* check if debounced input state is different than the previous input state */
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
    static uint32_t s_ticksUntilHwWatchdogKick = 0U;
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
            GetHighestPriorityResetCause(kQMC_ResetFunctionalWd, (qmc_reset_cause_id_t)gs_snvsStateModified.resetCause);
        /* if the notification does not work, the system is still reset
         * (but the CM7 might not have enough time for final tasks) */
        notifyRet = RPC_NotifyCM7AboutReset(kQMC_ResetFunctionalWd);
        (void)notifyRet;
        assert(kStatus_QMC_Ok == notifyRet);
    }
    /* grace period expired -> store state and reset system */
    else if (kStatus_LWDG_TickJustExpired == tickState)
    {
        resetSystem = true;
    }
    else
    {
        /* no action required, "else" added for compliance */
        ;
    }

#if FEATURE_SECURE_WATCHDOG
    /* tick authenticated watchdog */
    tickState = AWDG_Tick();
    /* can not happen */
    assert(kStatus_LWDG_TickPreviouslyExpired != tickState);
    /* backup value is rounded up
     * this guarantees we never store zero without setting the appropriate reset cause
     * (at restoring the backup value is always decreased to ensure we always eventually expire)
     */
    awdgBackupTicksToTimeout = ConvertAuthenticatedWatchdogTicksToBackupValue(AWDG_GetRemainingTicks());
    /* backup AWDG state
     * authenticated watchdog has not expired yet */
    if (awdgBackupTicksToTimeout > 0U)
    {
        gs_snvsStateModified.wdTimerBackup = awdgBackupTicksToTimeout;
        /* sync AWDG state directly to hardware, else an attacker could starve the sync in the main loop by
         * triggering the communication interrupt constantly
         * by triggering a sync here the most an attacker can gain is another reboot where the secure watchdog
         * will immediately expire and a reboot to recovery mode is triggered */
        if (awdgBackupTicksToTimeout != gs_snvsStateHW.wdTimerBackup)
        {
            /* can not interfere with the main sync code as we immediately mark the value up-to-date 
             * we are allowed to do this as gs_snvsStateModified.wdTimerBackup is only modified in this interrupt */
            HAL_SetWdTimerBackup(awdgBackupTicksToTimeout);
            gs_snvsStateHW.wdTimerBackup = awdgBackupTicksToTimeout;
        }
    }
    /* authenticated watchdog expired
     * grace period just started */
    else if (kStatus_LWDG_TickJustStarted == tickState)
    {
        /* set appropriate reset cause, store state, notify CM7 */
        HandleAuthenticatedWatchdogExpirationIntsDisabled();
    }
    /* grace period expired -> store state and reset system */
    else if (kStatus_LWDG_TickJustExpired == tickState)
    {
        resetSystem = true;
    }
    else
    {
        /* no action required, "else" added for compliance */
        ;
    }
#endif

    /* reset system if needed */
    if (true == resetSystem)
    {
        QMC_CM4_ResetSystemIntsDisabled((qmc_reset_cause_id_t)gs_snvsStateModified.resetCause);
    }

    /* kick hardware watchdog */
    if(0U == s_ticksUntilHwWatchdogKick)
    {
        HAL_KickHWWatchdog();
        /* can never be set to zero, checked with compile-time macro */
        s_ticksUntilHwWatchdogKick = QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD;
        assert(s_ticksUntilHwWatchdogKick > 0U);
    }
    s_ticksUntilHwWatchdogKick--;
}

void QMC_CM4_HandleHardwareWatchdogISR(void)
{
    /* we try to perform the reset ourselves
     * so that we can set the battery-backed SNVS registers to well-defined values */

    /* do not trust the SNVS software mirrors, just set the state in the hardware SNVS directly
     * (might happen before everything is initialized correctly)
     * use kFWU_AwdtExpired as FWU status and kQMC_ResetSecureWd as reset cause to boot into recovery mode
     * + reset authenticated watchdog state backup */
    HAL_SetFwuStatus(HAL_GetFwuStatus() | (uint8_t)kFWU_AwdtExpired);
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
    while(FOREVER())
    {
       /* no action required, added for compliance */
       ;
    }
}

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
 * @file qmc_cm4_api.h
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

#ifndef _QMC_CM4_API_H_
#define _QMC_CM4_API_H_

#include <stdint.h>
#include <stddef.h>

#include "qmc_features_config.h"
#include "qmc_cm4_features_config.h"
#include "api_qmc_common.h"
#include "api_rpc.h"
#include "awdg/awdg_api.h"
#include "hal/hal.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*!
 * @brief Struct to mirror the data in SNVS
 *
 */
typedef struct
{
    uint16_t wdTimerBackup;    /*!< Backup of the ticks which the AWDT has left till expiry. */
    uint8_t wdStatus;          /*!< Status whether AWDT was running before reset. */
    uint8_t fwuStatus;         /*!< Status whether firmware update shall be committed or reverted.
                                    Takes qmc_fw_update_state_t as values. */
    int64_t srtcOffset;        /*!< Offset value for SRTC to calculate actual time. */
    uint8_t resetCause;        /*!< Reset reason to be stored in SNVS.
                                    Takes qmc_reset_cause_id_t as values. */
    uint32_t gpioOutputStatus; /*!< Mirror for the GPIO output pins status. */
} qmc_cm4_snvs_mirror_data_t;

/*!
 * @brief Struct to diff the qmc_cm4_snvs_mirror_data_t structure
 *
 */
typedef struct
{
    bool wdTimerBackup;    /*!< Is wdTimerBackup different? */
    bool wdStatus;         /*!< Is wdStatus different? */
    bool fwuStatus;        /*!< Is fwuStatus different? */
    bool srtcOffset;       /*!< Is srtcOffset different? */
    bool resetCause;       /*!< Is resetCause different? */
    bool gpioOutputStatus; /*!< Is gpioOutputStatus different? */
} qmc_cm4_snvs_mirror_data_diff_t;

#define QMC_CM4_WDG_TICK_FREQUENCY_HZ                                         \
    (1U << HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP) /*!< tick frequency of the watchdogs (functional + authenticated) \
                 (defined by HAL drivers) */
#define QMC_CM4_AWDG_TICKS_BACKUP_SHIFT                                             \
    (16U) /*!< shift value which is used to reduce the bit width of the timer value \
               (result must fit into 16bit!) for backup */

#define QMC_CM4_AWDG_TICKS_BACKUP_MAX ((1U << QMC_CM4_AWDG_TICKS_BACKUP_SHIFT) - 1U) /*!< max. value of timer backup */
#define QMC_CM4_VALID_FWU_STATE_MASK (kFWU_Revert | kFWU_Commit | kFWU_BackupCfgData | \
    kFWU_AwdtExpired | kFWU_VerifyFw | kFWU_TimestampIssue) /*!< mask for checking validity of FWU state */

/*******************************************************************************
 * Compile time checks
 ******************************************************************************/
/* we want to have timeouts of at least 7 days (604800 seconds) */
#if QMC_CM4_WDG_TICK_FREQUENCY_HZ > ((UINT32_MAX - 1U) / 604800U)
#error "An expiry time of at least 7 days should be setable for the watchdogs, reduce tick frequency!"
#endif

/* max. 255 functional watchdogs are supported */
#if QMC_CM4_FWDGS_COUNT > 255U
#error "Max. amount (255) of supported functional watchdogs was exceeded!"
#endif

/*******************************************************************************
 * API
 *******************************************************************************/

/*!
 * @brief Initializes and configures all components needed by the M4 part of the project.
 *
 * Initialize the board, its peripherals (user outputs are set to their initial value)
 * and buffers the needed SNVS register values into their software mirrors.
 * Initializes the functional, secure watchdog(s) and if necessary the shared memory
 * needed for inter-core communication.
 * Starts the hardware watchdog.
 * Configures and enables interrupts.
 * Notifies the CM7 about the initial state of the user inputs (this will be repeated
 * until acknowledged by CM7, so it does not matter if CM7 is not ready yet).
 *
 * This function should be the first function that is called after control reached the
 * main function. Not reentrant, must be called before interrupts are enabled!
 *
 * If this function fails the board should be rebooted into recovery mode as something
 * serious is wrong!
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The initialization was successful.
 * @retval KStatus_QMC_Err
 * The initialization failed.
 * 
 * @startuml
 * start
 * :ret = kStatus_QMC_Err;
 * :HAL_InitBoard()
 * HAL_InitSrtc()
 * HAL_InitRtc()
 * HAL_InitHWWatchdog();
 * :HAL_SnvsGpioInit(QMC_CM4_SNVS_USER_OUTPUTS_INIT_STATE)
 * gs_halGpioInputState = HAL_GetSnvsGpio13();
 * :mbedtls_memory_buffer_alloc_init(gs_MbedTlsHeapStaticBuffer, AWDG_MBEDTLS_HEAP_MIN_STATIC_BUFFER_SIZE);
 * if (LoadAndCheckSnvsState()) then (failed)
 *      :ret = kStatus_QMC_Err;
 * elseif (InitializeFunctionalWatchdogsHelper()) then (failed)
 *      :ret = kStatus_QMC_Err;
 * elseif (InitializeAuthenticatedWatchdogHelper()) then (failed)
 *      :ret = kStatus_QMC_Err;
 * else (else)
 *      :QMC_CM4_SyncSnvsStateMain()
 *      HAL_ConfigureEnableInterrupts()
 *      g_awdgInitDataSHM.ready = 1U;
 *      :HAL_EnterCriticalSectionNonISR()
 *      QMC_CM4_NotifyCM7AboutGpioChange(HalGpioState2QmcCm4GpioInput(gs_halGpioInputState))
 *      HAL_ExitCriticalSectionNonISR();
 *      :HAL_InitSysTick();
 *      :ret = kStatus_QMC_Ok;
 * endif
 * :return ret;
 * stop
 * @enduml
 */
qmc_status_t QMC_CM4_Init(void);

/*!
 * @brief Helper for kicking a functional watchdog.
 *
 * Not reentrant, must not be used in parallel with the
 * QMC_CM4_HandleWatchdogTickISR() function!
 *
 * @startuml
 * start
 * :kickStatus = LWDGU_KickOne(&gs_FwdgUnit, id)
 * ret = toQmcStatus(kickStatus)
 * return ret;
 * stop
 * @enduml
 *
 * @param[in] id The functional watchdog's id.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * The functional watchdog with the given ID does not exist.
 * @retval kStatus_QMC_Ok
 * The operation was successful.
 */
qmc_status_t QMC_CM4_KickFunctionalWatchdogIntsDisabled(const uint8_t id);

#if FEATURE_SECURE_WATCHDOG
/*!
 * @brief Helper function for getting a nonce from the authenticated watchdog.
 *
 * Not reentrant, must not be used in parallel with the
 * QMC_CM4_ProcessTicketAuthenticatedWatchdog() function!
 *
 * @startuml
 * start
 * :ret = kStatus_QMC_Err;
 * if () then (pData == NULL || pDataLen == NULL)
 *   :ret = kStatus_QMC_ErrArgInvalid;
 * else if () then (*pDataLen < AWDG_NONCE_LENGTH)
 *   :ret = kStatus_QMC_ErrNoBufs;
 * else (else)
 *   :getNonceRet = AWDG_GetNonce(pData)
 *   ret = toQmcStatus(getNonceRet);
 *   if () then (ret == kStatus_QMC_Ok)
 *     : *pDataLen = AWDG_NONCE_LENGTH;
 *   else (else)
 *     : *pDataLen = 0;
 *   endif
 * endif
 * :return ret;
 * stop
 * @enduml
 *
 * @param[out] pData Pointer to the buffer receiving the nonce.
 * @param[in,out] pDataLen Pointer to an uint32_t containing the size of the nonce buffer in bytes.
 * If the operation was successful, the size of the written nonce is stored at this address.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * Returned in case a supplied pointer was NULL.
 * @retval kStatus_QMC_ErrNoBufs
 * The supplied buffer for receiving the nonce was too small.
 * @retval kStatus_QMC_ErrInternal
 * Returned in case an error occurred at the internal RNG calls.
 * @retval kStatus_QMC_Ok
 * The operation was successful.
 */
qmc_status_t QMC_CM4_GetNonceAuthenticatedWatchdog(uint8_t *const pData, uint32_t *const pDataLen);

/*!
 * @brief Helper for processing a ticket for the authenticated watchdog.
 *
 * Verifies the given ticket and defers the watchdog if the verification was successful.
 *
 * Not reentrant, must not be used in parallel with the
 * QMC_CM4_GetNonceAuthenticatedWatchdog() function!
 *
 * Worst case execution time on IMX RT1176 CM4 (n = 100):
 *      w.c. 15016ms
 *
 * @startuml
 * start
 * :ret = kStatus_QMC_Err
 * validateTicketRet = kStatus_AWDG_ValidateTicketError
 * deferWatchdogRet = kStatus_AWDG_DeferWatchdogError;
 * if () then (pData == NULL)
 *    :return kStatus_QMC_ErrArgInvalid;
 *    stop
 * endif
 * :validateTicketRet = AWDG_ValidateTicket(pData, dataLen);
 * if () then (validateTicketRet != kStatus_AWDG_ValidateTicketValidated)
 *    :ret = toQmcStatus(validateTicketRet)
 *    return ret;
 *    stop
 * endif
 * :HAL_EnterCriticalSectionNonISR()
 * deferWatchdogRet = AWDG_DeferWatchdog()
 * HAL_ExitCriticalSectionNonISR();
 * :ret = toQmcStatus(deferWatchdogRet)
 * return ret;
 * stop
 * @enduml
 *
 * @param[in] pData Pointer to the buffer containing the ticket.
 * @param[in] dataLen The tickets size in bytes.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * Returned in case a supplied pointer was NULL or due to an invalid ticket format.
 * @retval kStatus_QMC_ErrInternal
 * Returned in case an error occurred at the internal RNG calls.
 * @retval kStatus_QMC_ErrRange
 * Returned in case the ticket timeout value was too large and would have caused an overflow.
 * @retval kStatus_QMC_ErrSignatureInvalid
 * Returned in case the ticket's signature could not be verified.
 * @retval kStatus_QMC_Ok
 * The operation was successful.
 */
qmc_status_t QMC_CM4_ProcessTicketAuthenticatedWatchdog(const uint8_t *const pData, const uint32_t dataLen);
#endif

/*!
 * @brief Sets the logic-level value for the specified SNVS user output pins to the given value.
 *
 * Checks if the pin value is valid and performs the changes on a SNVS software mirror.
 * The changes must be committed later in the main loop using QMC_CM4_SyncSnvsRpcStateMain().
 *
 * Not reentrant! If called from a non-interrupt context, access to the SNVS mirror
 * must be protected (disable interrupts during access).
 *
 * @startuml
 * start
 * if(pin is valid) then (false)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * if(value != 0) then (true)
 *   :gs_snvsStateModified.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus | (1 << pin);
 * else (false)
 *   :gs_snvsStateModified.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus & ~(1 << pin);
 * endif
 * stop
 * @enduml
 *
 * @param[in] pin The pin of which the output level should be set, one of qmc_cm4_snvs_pin_t.
 * @param[in] value The logic level the pin should be set to (0 ... low, > 0 ... high).
 * @return qmc_status_t A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully.
 * @retval kStatus_QMC_ErrArgInvalid
 * The given pin or value is not supported.
 */
qmc_status_t QMC_CM4_SetSnvsGpioPinIntsDisabled(const hal_snvs_gpio_pin_t pin, const uint8_t value);

/*!
 * @brief Gets the current timestamp from the real time clock.
 *
 * Accesses the SRTC SNVS peripheral, do not use in interrupt!
 *
 * Not reentrant, must not be used in parallel with QMC_CM4_SetRtcTime()!
 *
 * @startuml
 * start
 * :qmc_status_t ret = kStatus_QMC_Err;
 * if (pTime == NULL) then (true)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * :int64_t offset = gs_snvsStateModified.srtcOffset;
 * :int64_t rtcValueIs
 * ret = HAL_GetSrtcCount(&rtcValueIs);
 * if (ret != kStatus_QMC_Ok) then (true)
 *   :return ret;
 *   stop
 * endif
 * if (offset > INT64_MAX - rtcValueIs) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :int64_t rtcValueReal = rtcValueIs + offset
 * int64_t timeMs = 0
 * ret = QMC_CM4_ConvertRtcCounterValToMs(rtcValueReal, &timeMs);
 * if (ret != kStatus_QMC_Ok) then (true)
 *   :return ret;
 *   stop
 * endif
 * :pTime->milliseconds = timeMs % 1000
 * pTime->seconds = timeMs / 1000
 * return kStatus_QMC_Ok;
 * stop
 * @enduml
 *
 * @param pTime Pointer to the variable where the fetched time should be stored. The
 * result is written only when the function returns kStatus_QMC_Ok.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrRange
 * The internal tick value was too large and would have caused an
 * integer overflow at an internal calculation.
 * @retval kStatus_QMC_ErrArgInvalid
 * The pointer pTime was NULL.
 * @retval kStatus_QMC_Timeout
 * Accessing the SVNS SRTC peripheral timed out. Try again.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully. The result data was
 * written to *pTime.
 */
qmc_status_t QMC_CM4_GetRtcTimeIntsDisabled(qmc_timestamp_t *const pTime);

/*!
 * @brief Sets the real time clock from the given timestamp.
 *
 * The maximum setable time is limited by the QMC_CM4_ConvertMsToRtcCounterVal
 * function which accepts a timestamp in milliseconds capable of representing
 * approximately 8901 years.
 *
 * Accesses the SRTC SNVS peripheral, do not use in interrupt!
 *
 * Not reentrant, must not be used in parallel with QMC_CM4_GetRtcTime()!
 * 
 * This function does not write the new offset directly into the SVNS hardware
 * register, but rather in an software mirror. The changes must be commited later
 * using QMC_CM4_SyncSnvsRpcStateMain() or QMC_CM4_ResetSystemIntsDisabled().
 *
 * The expression rtcValueShould + (-rtcValueIs) can not overflow or
 * underflow as the two addition operands have different signs.
 *
 * @startuml
 * start
 * :qmc_status_t ret = kStatus_QMC_Err;
 * :int64_t rtcValueIs
 * ret = HAL_GetSrtcCount(&rtcValueIs);
 * if (ret != kStatus_QMC_Ok) then (true)
 *   :return ret;
 *   stop
 * endif
 * if (pTime == NULL) then (true)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * if (pTime->seconds > (INT64_MAX - pTime->milliseconds) / 1000) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :int64_t timeMs = pTime->seconds*1000 + pTime->milliseconds
 * int64_t rtcValueShould = 0
 * :ret = QMC_CM4_ConvertMsToRtcCounterVal(timeMs, &rtcValueShould);
 * if (ret != kStatus_QMC_Ok) then (true)
 *   :return ret;
 *   stop
 * endif
 * :int64_t offset = rtcValueShould - rtcValueIs;
 * :gs_snvsStateModified.srtcOffset = offset;
 * :return kStatus_QMC_Ok;
 * stop
 * @enduml
 *
 * @param[in] pTime Pointer to the time the real-time clock should be set to.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrRange
 * The input timestamp was too large and would have caused an
 * integer overflow. The real-time clock was not modified.
 * @retval kStatus_QMC_Timeout
 * Accessing the SVNS SRTC peripheral timed out. Try again.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully.
 */
qmc_status_t QMC_CM4_SetRtcTimeIntsDisabled(qmc_timestamp_t *const pTime);

/*!
 * @brief Resets the entire SoC.
 *
 * Writes reset cause into SNVS if it has an higher priority than the current 
 * reset cause, syncs the SNVS mirror to the hardware SNVS register and performs 
 * the reset.
 *
 * If the reset cause is kQMC_ResetSecureWd, the backed-up secure watchdog state
 * is also reset (not running).
 * 
 * The priority precedence is kQMC_ResetSecureWd > kQMC_ResetFunctionalWd > 
 * kQMC_ResetRequest > kQMC_ResetNone. 
 * If an unknown reset cause is given, kQMC_ResetSecureWd is used!
 * 
 * Not reentrant! Must be called while interrupts are disabled!
 * 
 * Worst case execution time (without HAL_ResetSystem() on IMX RT1176 CM4 (n = 100000):
 *      w.c. 395563 cycles 989us
 *      (comparably high execution time because of SNVS access)
 * 
 * @startuml
 * start
 * :gs_snvsStateModified.resetCause = getHighestPriorityResetCause(resetCause, gs_snvsStateModified.resetCause);
 * if (gs_snvsStateModified.resetCause == kQMC_ResetSecureWd) then (true)
 *   :gs_snvsStateModified.fwuStatus |= kFWU_AwdtExpire
 *   gs_snvsStateModified.wdTimerBackup = 0
 *   gs_snvsStateModified.wdStatus = 0;
 * else if (gs_snvsStateModified.resetCause == kQMC_ResetRequest) then (true)
 *   :gs_snvsStateModified.fwuStatus &= ~kFWU_AwdtExpired;
 * endif
 * :HAL_SetFwuStatus(gs_snvsStateModified.fwuStatus)
 * HAL_SetResetCause(gs_snvsStateModified.resetCause)
 * HAL_SetWdTimerBackup(gs_snvsStateModified.wdTimerBackup)
 * HAL_SetWdStatus(gs_snvsStateModified.wdStatus)
 * HAL_SetSrtcOffset(gs_snvsStateModified.srtcOffset)
 * HAL_ResetSystem();
 * stop
 * @enduml
 *
 * @param[in] resetCause The cause for the reset.
 */
void QMC_CM4_ResetSystemIntsDisabled(qmc_reset_cause_id_t resetCause);

/*!
 * @brief Gets the reset-proof firmware update status from the battery backed-up
 * SNVS GPR register.
 *
 * This function does not actually access the SNVS hardware, but rather a software mirror!
 * The software mirrors must have been initialized before (done by QMC_CM4_Init()).
 *
 * Not reentrant! If called from a non-interrupt context, access to the SNVS mirror
 * must be protected (disable interrupts during access).
 *
 * @startuml
 * start
 *   :return gs_snvsStateModified.fwuStatus;
 * stop
 * @enduml
 *
 * @return A qmc_fw_update_state_t representing the firmware update state.
 */
qmc_fw_update_state_t QMC_CM4_GetFwUpdateState(void);

/*!
 * @brief Gets the reset-proof reset cause from the battery backed-up
 * SNVS GPR register.
 *
 * This function returns the last reset cause as was loaded at startup by the QMC_CM4_Init() function.
 *
 * Not reentrant! If called from a non-interrupt context, access to the SNVS mirror
 * must be protected (disable interrupts during access).
 *
 * @startuml
 * start
 *   :return gs_previousResetCause;
 * stop
 * @enduml
 *
 * @return A qmc_reset_cause_id_t representing the reset cause.
 */
qmc_reset_cause_id_t QMC_CM4_GetResetCause(void);

/*!
 * @brief Saves a "commit" request into the battery backed-up SNVS GPR register.
 *
 * The commit will be executed by the bootloader after reset.
 *
 * This function does not actually access the SNVS hardware, but rather a software mirror!
 * The changes must be committed later using QMC_CM4_SyncSnvsRpcStateMain() or QMC_CM4_ResetSystemIntsDisabled()!
 *
 * Not reentrant! If called from a non-interrupt context, access to the SNVS mirror
 * must be protected (disable interrupts during access).
 *
 * @startuml
 * start
 *   :gs_snvsStateModified.fwuStatus |= kFWU_Commit;
 * stop
 * @enduml
 *
 */
void QMC_CM4_CommitFwUpdateIntsDisabled(void);

/*!
 * @brief Saves a "revert" request into the battery backed-up SNVS GPR register.
 *
 * The revert will be executed by the bootloader after reset.
 *
 * This function does not actually access the SNVS hardware, but rather a software mirror!
 * The changes must be committed later using QMC_CM4_SyncSnvsRpcStateMain() or QMC_CM4_ResetSystemIntsDisabled()!
 *
 * Not reentrant! If called from a non-interrupt context, access to the SNVS mirror
 * must be protected (disable interrupts during access).
 *
 * @startuml
 * start
 *   :gs_snvsStateModified.fwuStatus |= kFWU_Revert;
 * stop
 * @enduml
 *
 */
void QMC_CM4_RevertFwUpdateIntsDisabled(void);

/*!
 * @brief Synchronize the SNVS software mirrors with the SNVS hardware. Finishes pending RPC calls.
 *
 * Without GPIO13.
 *
 * Indented to be called from the main loop, do not call from an interrupt!
 * In the case an asynchronous remote procedure call completed, the CM7 is informed.
 *
 * Not reentrant!
 *
 * @startuml
 * start
 * :callReturn = kStatus_QMC_Err;
 * :QMC_CM4_SyncSnvsStorageMain();
 * if(RPC_ProcessPendingSecureWatchdogCall(&callReturn)) then (true)
 *   :HAL_EnterCriticalSectionNonISR()
 *   RPC_FinishSecureWatchdogCall(callReturn)
 *   HAL_ExitCriticalSectionNonISR();
 * endif
 * stop
 * @enduml
 */
void QMC_CM4_SyncSnvsRpcStateMain(void);

/*!
 * @brief Synchronize the SNVS GPIO register with the SNVS hardware.
 *
 * Not reentrant! Indented to be called when interrupts are disabled.
 * Do not call from multiple execution contexts simultaneously!
 *
 * @startuml
 * start
 * if(gs_snvsStateModified.gpioOutputStatus != gs_snvsStateHW.gpioOutputStatus) then (true)
 *   :HAL_SetSnvsGpio13Outputs(gs_snvsStateModified.gpioOutputStatus)
 *   gs_snvsStateHW.gpioOutputStatus = gs_snvsStateModified.gpioOutputStatus;
 * endif
 * stop
 * @enduml
 */
void QMC_CM4_SyncSnvsGpioIntsDisabled(void);

/*!
 * @brief Processes systick periodic calls
 *
 * This function should be called from the SysTick_Handler interrupt.
 *
 * Worst case execution time on IMX RT1176 CM4 (n = 100000):
 *      w.c. 47935 cycles 120us
 *      (comparably high execution time because of SNVS access)
 * 
 * @startuml
 * start
 * :HAL_GpioTimerHandler()
 * uint32_t newHalGpioInputs = HAL_GetSnvsGpio13();
 * if (gs_halGpioInputState != newHalGpioInputs) then (true)
 *   :RPC_NotifyCM7AboutGpioChange(HalGpio2QMC_CM4_GpioInput(newHalGpioInputs))
 *   gs_halGpioInputState = newHalGpioInputs;
 * endif
 * stop
 * @enduml
 */
void QMC_CM4_HandleSystickISR(void);

/*!
 * @brief Processes SNVS user input changes.
 *
 * This function should be called from the GPIO13_GPIO_COMB_0_15_IRQHANDLER or GPIO13_Combined_0_31_IRQHandler
 * interrupt. WARNING! The ISR names are different between cmake-based SDK and the mcuexpresso-based SDK, so check it in
 * the startup file to be sure.
 *
 * Worst case execution time on IMX RT1176 CM4 (n = 100000): 
 *      w.c. 83891 cycles 210us
 *      (comparably high execution time because of SNVS access)
 * 
 * @startuml
 * start
 *    :HAL_GpioInterruptHandler();
 * stop
 * @enduml
 */
void QMC_CM4_HandleUserInputISR(void);

/*!
 * @brief Processes the timer interrupt for ticking the watchdogs.
 *
 * This function should be called from the SNVS_HP_NON_TZ_IRQHandler interrupt.
 * Ticks the functional and authenticated watchdog, notifies the system about
 * an upcoming reset in case of a watchdog expiration, writes the reset cause
 * to the SNVS and finally resets the system.
 *
 * Worst case execution time on IMX RT1176 CM4 (n = 100000): 
 *      10 Functional Watchdogs:
 *      w.c. 71910 cycles 180us
 *      (all watchdogs were running at the same time)
 * 
 * @startuml
 * :resetSystem = false
 * tickState = kStatus_LWDG_TickErr
 * awdgBackupTicksToTimeout = 0;
 * :tickState = LWDGU_Tick(&gs_FwdgUnit);
 * if () then (tickState == kStatus_LWDG_TickJustStarted)
 *   :gs_snvsStateModified.resetCause = getHighestPriorityResetCause(kQMC_ResetFunctionalWd, gs_snvsStateModified.resetCause) 
 *   RPC_NotifyCM7AboutReset(kQMC_ResetFunctionalWd); 
 * else if () then (tickState == kStatus_LWDG_TickJustExpired) 
 *   :resetSystem = true;  
 * endif
 * :tickState = AWDG_Tick()
 * awdgBackupTicksToTimeout = ConvertAuthenticatedWatchdogTicksToBackupValue(AWDG_GetRemainingTicks());
 * if () then (awdgBackupTicksToTimeout > 0U)
 *   :gs_snvsStateModified.wdTimerBackup = awdgBackupTicksToTimeout;
 *   if () then (gs_snvsStateModified.wdTimerBackup != gs_snvsStateHW.wdTimerBackup)
 *      :HAL_SetWdTimerBackup(gs_snvsStateModified.wdTimerBackup)
 *      gs_snvsStateHW.wdTimerBackup = gs_snvsStateModified.wdTimerBackup;
 *   endif
 * else if () then (tickState == kStatus_LWDG_TickJustStarted)
 *   :HandleAuthenticatedWatchdogExpirationIntsDisabled();
 * else if () then (tickState == kStatus_LWDG_TickJustExpired)
 *   :resetSystem = true; 
 * endif
 * if () then (resetSystem == true)
 *  :QMC_CM4_ResetSystemIntsDisabled(gs_snvsStateModified.resetCause);
 *  stop
 * endif
 * :HAL_KickHWWatchdog();
 * stop
 * @enduml
 *
 */
void QMC_CM4_HandleWatchdogTickISR(void);

/*!
 * @brief Processes the hardware watchdog interrupt.
 *
 * This function should be called from the WDOG1_IRQHandler interrupt.
 * Tries to reset the system in a well-defined fashion before the 
 * hardware watchdog expires. Ensures the battery-backed SNVS registers
 * are in a defined state.
 *
 * Worst case execution time (without reset) on IMX RT1176 CM4 (n = 100000):
 *      w.c. 359593 cycles 899us
 *      (comparably high execution time because of SNVS access)
 * 
 * @startuml
 * :HAL_SetFwuStatus(HAL_GetFwuStatus() | kFWU_AwdtExpired)
 * HAL_SetResetCause(kQMC_ResetSecureWd)
 * HAL_SetWdTimerBackup(0)
 * HAL_SetWdStatus(0)
 * HAL_ResetSystem();
 * stop
 * @enduml
 *
 */
void QMC_CM4_HandleHardwareWatchdogISR(void);

#endif /* _QMC_CM4_API_H_ */

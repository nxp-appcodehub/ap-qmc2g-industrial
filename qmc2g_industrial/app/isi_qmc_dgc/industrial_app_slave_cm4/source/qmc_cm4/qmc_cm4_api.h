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
 * module. There are two mirrors of all required SNVS fields. There are two mirrors of all required SNVS fields.
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
 * Note that the system should have a charged coin cell inserted, otherwise the secure watchdog will always indicate
 * an expiry so that the cyber resiliency feature is not bypassed.
 *
 * For reentrancy and concurrency assumptions, check the individual API descriptions.
 *
 * Limitations:
 *  - The RTC offset needs two SNVS LPGPR registers, hence a power-loss / reset between
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
    uint16_t wdTimerBackup;    /*!< Backup of the ticks which the AWDG has left till expiry. */
    uint8_t wdStatus;          /*!< Status whether AWDG was running before reset. */
    uint8_t fwuStatus;         /*!< Status whether firmware update shall be committed or reverted.
                                    Takes qmc_fw_update_state_t as values. */
    int64_t srtcOffset;        /*!< Offset value for SRTC to calculate actual time. */
    uint8_t resetCause;        /*!< Reset reason.
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

#define QMC_CM4_WDG_TICK_FREQUENCY_HZ                                                                                 \
    (UINT32_C(1) << HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP) /*!< tick frequency of the watchdogs (functional +
                 authenticated) (defined by HAL drivers) */
/* chosen so that QMC_CM4_WDG_TICK_FREQUENCY_HZ is tightly enforced by the HW watchdog
 * the HW watchdog interrupt issued before the real timeout would also reset the system (just gracefully),
 * hence the time until this interrupt is fired must be used as timeout
 * further, 5ms are kept as buffer for other interrupts and processing time noise */
#define QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD                                                                         \
    ((500U * (HAL_WDG_TIMEOUT_VALUE + 1U - HAL_WDG_INT_BEFORE_TIMEOUT_VALUE) - 5U) * QMC_CM4_WDG_TICK_FREQUENCY_HZ / \
     1000U) /*!< Ticks until HW watchdog is kicked */

#define QMC_CM4_AWDG_TICKS_BACKUP_SHIFT                                             \
    (16U) /*!< shift value which is used to reduce the bit width of the timer value
               (result must fit into 16bit!) for backup */
#define QMC_CM4_AWDG_TICKS_BACKUP_MAX \
    ((UINT32_C(1) << QMC_CM4_AWDG_TICKS_BACKUP_SHIFT) - 1U) /*!< max. value of timer backup */
#define QMC_CM4_VALID_FWU_STATE_MASK                                                                         \
    ((uint8_t)kFWU_Revert | (uint8_t)kFWU_Commit | (uint8_t)kFWU_BackupCfgData | (uint8_t)kFWU_AwdtExpired | \
     (uint8_t)kFWU_VerifyFw | (uint8_t)kFWU_TimestampIssue) /*!< mask for checking validity of FWU state */

/*******************************************************************************
 * Compile time checks
 ******************************************************************************/
/* unsigned value checks */
#if (HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP < 0)
#error "HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP must be unsigned!"
#endif
#if (HAL_WDG_TIMEOUT_VALUE < 0)
#error "HAL_WDG_TIMEOUT_VALUE must be unsigned!"
#endif
#if (HAL_WDG_INT_BEFORE_TIMEOUT_VALUE < 0)
#error "HAL_WDG_INT_BEFORE_TIMEOUT_VALUE must be unsigned!"
#endif

/* tick frequency checks 
 * we want to have timeouts of at least 7 days (604800 seconds) */
#if (QMC_CM4_WDG_TICK_FREQUENCY_HZ == 0U)
#error "Tick frequency must not be 0!"
#endif 
#if (QMC_CM4_WDG_TICK_FREQUENCY_HZ > ((UINT32_MAX - 1U) / 604800U))
#error "An expiry time of at least 7 days should be setable for the watchdogs, reduce tick frequency!"
#endif

/* functional watchdog count checks 
 * max. 255 functional watchdogs are supported */
#if (QMC_CM4_FWDGS_COUNT > 255U)
#error "Max. amount (255) of supported functional watchdogs was exceeded!"
#endif

/* QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD calculation sanity checks */
#if (HAL_WDG_INT_BEFORE_TIMEOUT_VALUE > HAL_WDG_TIMEOUT_VALUE)
#error "HAL_WDG_INT_BEFORE_TIMEOUT_VALUE must not be larger than HAL_WDG_TIMEOUT_VALUE!"
#endif
#if (HAL_WDG_TIMEOUT_VALUE > (UINT32_MAX - 1U))
#error "QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD calculation overflowed, review configuration defines!"
#endif
#if ((HAL_WDG_TIMEOUT_VALUE + 1U - HAL_WDG_INT_BEFORE_TIMEOUT_VALUE) > (UINT32_MAX / 500U))
#error "QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD calculation overflowed, review configuration defines!"
#endif
/* first term can not be negative as HAL_WDG_INT_BEFORE_TIMEOUT_VALUE <= HAL_WDG_TIMEOUT_VALUE */
#if ((500U * (HAL_WDG_TIMEOUT_VALUE + 1U - HAL_WDG_INT_BEFORE_TIMEOUT_VALUE) - 5U) > (UINT32_MAX / QMC_CM4_WDG_TICK_FREQUENCY_HZ))
#error "QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD calculation overflowed, review configuration defines!"
#endif
/* check if ticks until kick of hardware watchdog are valid */
#if (QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD == 0U)
#error "Ticks until a kick of the hardware watchdog must be greater 0!"
#endif

/* backup shift sanity checks */
#if ((QMC_CM4_AWDG_TICKS_BACKUP_SHIFT < 16U) || (QMC_CM4_AWDG_TICKS_BACKUP_SHIFT > 31U))
#error "AWDG ticks backup shift must be in the range [16, 31]!"
#endif

/*******************************************************************************
 * API
 *******************************************************************************/

/*!
 * @brief Initializes and configures all components needed by the M4 part of the project.
 *
 * Initialize the board, its peripherals (user outputs are set to their initial value)
 * and buffers the needed SNVS register values (system state) into their software mirrors.
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
 * If this function fails, the board should be rebooted into recovery mode as something
 * serious is wrong!
 *
 * Note that the system should have a charged coin cell inserted, otherwise the secure
 * watchdog will always indicate an expiry so that the cyber resiliency feature is not
 * bypassed.
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
 * HAL_InitHWWatchdog()
 * HAL_InitSrtc()
 * HAL_InitRtc()
 * HAL_SnvsGpioInit(QMC_CM4_SNVS_USER_OUTPUTS_INIT_STATE)
 * HAL_InitMcuTemperatureSensor();
 * :gs_halGpioInputState = HAL_GetSnvsGpio13();
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
 *      if (HAL_InitSysTick()) then (failed)
 *          :return kStatus_QMC_Err;
 *          stop
 *      endif 
 *      :ret = kStatus_QMC_Ok;
 * endif
 * :return ret;
 * stop
 * @enduml
 */
qmc_status_t QMC_CM4_Init(void);

/*!
 * @brief Helper function for kicking a functional watchdog.
 *
 * Not reentrant (disable interrupts), must not be used in parallel with the
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
 * Not reentrant (disable interrupts), must not be used in parallel with the
 * QMC_CM4_ProcessTicketAuthenticatedWatchdog() function!
 *
 * @startuml
 * start
 * :ret = kStatus_QMC_Err;
 * if () then (pData == NULL || pDataLen == NULL)
 *   :ret = kStatus_QMC_ErrArgInvalid;
 * else if () then (~*pDataLen < AWDG_NONCE_LENGTH)
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
 * If the verification succeeded, then the AWDT previous expiry flag is cleared.
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
 * if (ret) then (kStatus_QMC_Ok)
 *    :HAL_EnterCriticalSectionNonISR()
 *    gs_snvsStateModified.fwuStatus &= ~kFWU_AwdtExpired
 *    HAL_ExitCriticalSectionNonISR();
 * endif
 * return ret;
 * stop
 * @enduml
 *
 * @param[in] pData Pointer to the buffer containing the ticket.
 * @param[in] dataLen The ticket's size in bytes.
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
 * @brief Sets the logic-level value for the specified SNVS user output pin to the given value.
 *
 * Checks if the pin value is valid and performs the changes on a SNVS software mirror.
 * The changes must be committed later using QMC_CM4_SyncSnvsGpioIntsDisabled().
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
 * @param[in] pin The pin whose output level should be set, one of hal_snvs_gpio_pin_t.
 * @param[in] value The logic level the pin should be set to (0 ... low, > 0 ... high).
 * @return qmc_status_t A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully.
 * @retval kStatus_QMC_ErrArgInvalid
 * The given pin or value is not supported.
 */
qmc_status_t QMC_CM4_SetSnvsGpioPinIntsDisabled(const hal_snvs_gpio_pin_t pin, const uint8_t value);

/*!
 * @brief Gets the current timestamp from the real-time clock.
 *
 * Not reentrant (disable interrupts), must not be used in parallel with QMC_CM4_SetRtcTimeIntsDisabled()!
 *
 * @startuml
 * start
 * :qmc_status_t ret = kStatus_QMC_Err;
 * if (pTime == NULL) then (true)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * :int64_t offset = gs_snvsStateModified.srtcOffset;
 * :int64_t rtcValueIs = 0
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
 * ret = ConvertRtcCounterValToMs(rtcValueReal, &timeMs);
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
 * @param[out] pTime Pointer to the variable where the fetched time should be stored. The
 * result was written only when the function returns kStatus_QMC_Ok.
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
 * @brief Sets the real-time clock from the given timestamp.
 *
 * The maximum setable time is limited by the ConvertMsToRtcCounterVal()
 * function which accepts a timestamp in milliseconds capable of representing
 * approximately 8901 years.
 *
 * Not reentrant (disable interrupts), must not be used in parallel with QMC_CM4_GetRtcTimeIntsDisabled()!
 * 
 * This function does not write the new offset directly into the SVNS hardware
 * register, but rather in a software mirror. The changes must be committed later
 * using QMC_CM4_SyncSnvsRpcStateMain() or QMC_CM4_ResetSystemIntsDisabled().
 *
 * The expression rtcValueShould + (-rtcValueIs) can not overflow or
 * underflow as the two addition operands have different signs.
 *
 * @startuml
 * start
 * :qmc_status_t ret = kStatus_QMC_Err;
 * if (pTime == NULL) then (true)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * if (pTime->seconds > (INT64_MAX - pTime->milliseconds) / 1000) then (true)
 *   :return kStatus_QMC_ErrRange;
 *   stop
 * endif
 * :int64_t rtcValueIs = 0
 * ret = HAL_GetSrtcCount(&rtcValueIs);
 * if (ret != kStatus_QMC_Ok) then (true)
 *   :return ret;
 *   stop
 * endif
 * :int64_t timeMs = pTime->seconds*1000 + pTime->milliseconds
 * int64_t rtcValueShould = 0
 * :ret = ConvertMsToRtcCounterVal(timeMs, &rtcValueShould);
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
 * If the reset cause is kQMC_ResetSecureWd, the secure watchdog state in the SNVS
 * is also reset (not running).
 * 
 * The priority precedence is kQMC_ResetSecureWd > kQMC_ResetFunctionalWd > 
 * kQMC_ResetRequest > kQMC_ResetNone. 
 * If an unknown reset cause is given, kQMC_ResetSecureWd is used!
 * 
 * Not reentrant! Must be called while interrupts are disabled!
 * 
 * Worst case execution time (without HAL_ResetSystem()) on IMX RT1176 CM4 (n = 100000):
 *      w.c. 395563 cycles 989us
 *      (comparably high execution time because of SNVS access)
 * 
 * @startuml
 * start
 * :gs_snvsStateModified.resetCause = GetHighestPriorityResetCause(resetCause, gs_snvsStateModified.resetCause);
 * if (gs_snvsStateModified.resetCause == kQMC_ResetSecureWd) then (true)
 *   :gs_snvsStateModified.fwuStatus |= kFWU_AwdtExpired
 *   gs_snvsStateModified.wdTimerBackup = 0
 *   gs_snvsStateModified.wdStatus = 0;
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
qmc_fw_update_state_t QMC_CM4_GetFwUpdateStateIntsDisabled(void);

/*!
 * @brief Gets the reset-proof reset cause from the battery backed-up
 * SNVS GPR register.
 *
 * This function returns the last reset cause as was loaded at startup by the QMC_CM4_Init() function.
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
 * Intended to be called from the main loop, do not call from an interrupt!
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
 * @brief Synchronize the SNVS GPIO shadow register with the SNVS hardware.
 *
 * Not reentrant! Intended to be called when interrupts are disabled.
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
 * @brief Processes periodic SysTicks used for input debouncing.
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
 *   :RPC_NotifyCM7AboutGpioChange(HalGpioState2QmcCm4GpioInput(newHalGpioInputs))
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
 * @brief Handler for the timer interrupt used to tick the watchdogs.
 *
 * This function should be called from the SNVS_HP_NON_TZ_IRQHandler interrupt.
 * Ticks the functional and authenticated watchdog, notifies the system about
 * an upcoming reset in case of a watchdog expiration, writes the system state
 * to the SNVS and finally resets the system.
 * 
 * For ensuring that the authenticated watchdog is ticked and that the tick 
 * frequency matches the expectation the HW watchdog WDOG1 is used. Note
 * that this enforcement still allows to prolong timeouts insignificantly 
 * as we have to keep a buffer for other interrupts that may happen exactly
 * before this one and processing time noise. The maximal increase in percent
 * of authenticated watchdog timeout times this enforcement allows can be 
 * calculated as:
 * 
 * \f$\Delta = \frac{\mathrm{QMC\_CM4\_WDG\_TICK\_FREQUENCY\_HZ} \cdot (\mathrm{HAL\_WDG\_TIMEOUT\_VALUE} + 1 - \mathrm{HAL\_WDG\_INT\_BEFORE\_TIMEOUT\_VALUE}) \cdot 0.5}{\mathrm{QMC\_CM4\_TICKS\_UNTIL\_HWDG\_KICK\_RELOAD}} - 1\f$
 * 
 * In the current implementation this formula results in +1.19%.
 * 
 * Worst case execution time on IMX RT1176 CM4 (n = 100000): 
 *      10 Functional Watchdogs:
 *      w.c. 71910 cycles 180us
 *      (all watchdogs were running at the same time)
 * 
 * @startuml
 * :resetSystem = false
 * tickState = kStatus_LWDG_TickErr
 * static s_ticksUntilHwWatchdogKick = 0
 * awdgBackupTicksToTimeout = 0;
 * :tickState = LWDGU_Tick(&gs_FwdgUnit);
 * if () then (tickState == kStatus_LWDG_TickJustStarted)
 *   :gs_snvsStateModified.resetCause = GetHighestPriorityResetCause(kQMC_ResetFunctionalWd, gs_snvsStateModified.resetCause);
 *   :RPC_NotifyCM7AboutReset(kQMC_ResetFunctionalWd); 
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
 * if () then (s_ticksUntilHwWatchdogKick == 0)
 *  :HAL_KickHWWatchdog()
 *  s_ticksUntilHwWatchdogKick = QMC_CM4_TICKS_UNTIL_HWDG_KICK_RELOAD;
 * endif
 * :s_ticksUntilHwWatchdogKick--;
 * stop
 * @enduml
 *
 */
void QMC_CM4_HandleWatchdogTickISR(void);

/*!
 * @brief Handler for the hardware watchdog interrupt.
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

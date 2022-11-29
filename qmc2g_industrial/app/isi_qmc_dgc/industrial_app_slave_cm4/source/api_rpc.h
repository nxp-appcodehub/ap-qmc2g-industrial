/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _API_RPC_H_
#define _API_RPC_H_

#include "qmc_features_config.h"
#include "api_qmc_common.h"
#include "stdbool.h"
#include "stdatomic.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define RPC_SHM_SECTION_NAME "SRAM_OC_ECC1"                /*!> must match linker configuration */
#define RPC_SECWD_INIT_DATA_SECTION_NAME "secwd_init_data" /*!> must match linker configuration */

#define RPC_SECWD_MAX_RNG_SEED_SIZE  (48U)  /*!< see secure watchdog implementation for details */
#define RPC_SECWD_MAX_PK_SIZE        (158U) /*!< see secure watchdog implementation for details */
#define RPC_SECWD_MAX_MSG_SIZE       (150U)

#define RPC_DIGITAL_INPUT4_DATA (1U << 0U) /*!< mask where state for digital input 4 is stored */
#define RPC_DIGITAL_INPUT5_DATA (1U << 1U) /*!< mask where state for digital input 5 is stored */
#define RPC_DIGITAL_INPUT6_DATA (1U << 2U) /*!< mask where state for digital input 6 is stored */
#define RPC_DIGITAL_INPUT7_DATA (1U << 3U) /*!< mask where state for digital input 7 is stored */

#define RPC_DIGITAL_OUTPUT4_DATA   (1U << 0U)  /*!< mask where data for digital output 4 is stored */
#define RPC_DIGITAL_OUTPUT5_DATA   (1U << 1U)  /*!< mask where data for digital output 5 is stored */
#define RPC_DIGITAL_OUTPUT6_DATA   (1U << 2U)  /*!< mask where data for digital output 6 is stored */
#define RPC_DIGITAL_OUTPUT7_DATA   (1U << 3U)  /*!< mask where data for digital output 7 is stored */
#define RPC_DIGITAL_OUTPUT4_MODIFY (1U << 4U)  /*!< mask if digital output 4 needs modification */
#define RPC_DIGITAL_OUTPUT5_MODIFY (1U << 5U)  /*!< mask if digital output 5 needs modification */
#define RPC_DIGITAL_OUTPUT6_MODIFY (1U << 6U)  /*!< mask if digital output 6 needs modification */
#define RPC_DIGITAL_OUTPUT7_MODIFY (1U << 7U)  /*!< mask if digital output 7 needs modification */
#define RPC_SPI_CS0_DATA           (1U << 8U)  /*!< mask where data for SPI CS 0 is stored */
#define RPC_SPI_CS1_DATA           (1U << 9U)  /*!< mask where data for SPI CS 1 is stored */
#define RPC_SPI_CS0_MODIFY         (1U << 10U) /*!< mask if SPI CS 0 needs modification */
#define RPC_SPI_CS1_MODIFY         (1U << 11U) /*!< mask if SPI CS 1 needs modification */

#define RPC_OUTPUT_CONTROL_MASK                                                                                  \
    (RPC_DIGITAL_OUTPUT4_DATA | RPC_DIGITAL_OUTPUT5_DATA | RPC_DIGITAL_OUTPUT6_DATA | RPC_DIGITAL_OUTPUT7_DATA | \
     RPC_DIGITAL_OUTPUT4_MODIFY | RPC_DIGITAL_OUTPUT5_MODIFY | RPC_DIGITAL_OUTPUT6_MODIFY |                      \
     RPC_DIGITAL_OUTPUT7_MODIFY | RPC_SPI_CS0_DATA | RPC_SPI_CS1_DATA | RPC_SPI_CS0_MODIFY |                     \
     RPC_SPI_CS1_MODIFY) /*!< mask of all RPC GPIO output control flags */

/*!
 * @brief Static initialization macro for rpc_event_t.
 */
#define RPC_EVENT_STATIC_INIT                             \
    {                                                     \
        .isResetProcessed = true, .isGpioProcessed = true \
    }

/*!
 * @brief Static initialization macro for rpc_status_t.
 */
#define RPC_STATUS_STATIC_INIT                                                  \
    {                                                                           \
        .isNew = false, .isProcessed = true, .waitForAsyncCompletionCM4 = false \
    }

/*!
 * @brief Static initialization macro for RPC call data structures.
 */
#define RPC_SHM_CALL_DATA_STATIC_INIT(CALL) .CALL = {.status = RPC_STATUS_STATIC_INIT}

/*!
 * @brief Static initialization macro for rpc_shm_t.
 *
 * NOTE: This must be updated if rpc_shm_t changes.
 */
#define RPC_SHM_STATIC_INIT                                                                                           \
    {                                                                                                                 \
        .events = RPC_EVENT_STATIC_INIT, RPC_SHM_CALL_DATA_STATIC_INIT(funcWd), RPC_SHM_CALL_DATA_STATIC_INIT(secWd), \
        RPC_SHM_CALL_DATA_STATIC_INIT(gpioOut), RPC_SHM_CALL_DATA_STATIC_INIT(rtc),                                   \
        RPC_SHM_CALL_DATA_STATIC_INIT(fwUpdate), RPC_SHM_CALL_DATA_STATIC_INIT(reset)                                 \
    }

/*!
 * @brief Holds the RNG seed and PK needed for secure watchdog initialization.
 *
 */
typedef struct
{
    uint8_t rngSeed[RPC_SECWD_MAX_RNG_SEED_SIZE]; /*!< The RNG seed for secure watchdog initialization. */
    size_t rngSeedLen;                            /*!< The RNG seed length. */
    uint8_t pk[RPC_SECWD_MAX_PK_SIZE];            /*!< The PK for secure watchdog initialization. */
    size_t pkLen;                                 /*!< The PK length. */
    _Atomic uint8_t ready;                        /*!< The CM4 is ready. */
} rpc_secwd_init_data_t;

/*!
 * @brief Lists available watchdogs in the system.
 */
typedef enum _rpc_watchdog_id
{
    kRPC_SecureWatchdog      = 0U,
    kRPC_FunctionalWatchdog1 = 1U,
    kRPC_FunctionalWatchdogLast = kRPC_FunctionalWatchdog1
    /* TODO: add other wacthdogs here */
} rpc_watchdog_id_t;

/*!
 * @brief Holds data about SNVS GPIO input events and reset events triggered by the watchdogs.
 */
typedef struct _rpc_event
{
	_Atomic bool                 isResetProcessed;
	_Atomic bool                 isGpioProcessed;
	_Atomic qmc_reset_cause_id_t resetCause;
	_Atomic uint8_t              gpioState;  /*!< Bit 0 - 3 represents DIG.INPUT4 - 7 (1=high, 0=low). */
} rpc_event_t;

/*!
 * @brief Contains return value and status information of remote procedure calls.
 */
typedef struct _rpc_status
{
	_Atomic bool         isNew;
	_Atomic bool         isProcessed;
	_Atomic bool         waitForAsyncCompletionCM4;
    qmc_status_t retval;
} rpc_status_t;

/*!
 * @brief Holds parameters of the RPC_KickFunctionalWatchdog function.
 */
typedef struct _rpc_func_wd
{
    rpc_status_t      status;
    rpc_watchdog_id_t watchdog;
} rpc_func_wd_t;

/*!
 * @brief Holds parameters of the RPC_KickSecureWatchdog(data : uint8_t*, dataLen : int) : qmc_status_t and RPC_RequestNonceFromSecureWatchdogSync(nonce : uint8_t*, length : int*) : qmc_status_t functions.
 */
typedef struct _rpc_sec_wd
{
    rpc_status_t status;
    bool         isNonceNotKick;
    uint32_t     dataLen;
    uint8_t      data[RPC_SECWD_MAX_MSG_SIZE];
} rpc_sec_wd_t;

/*!
 * @brief Holds parameters of the RPC_SetSnvsOutput(gpioState : uint16_t) : qmc_status_t function.
 */
typedef struct _rpc_gpio
{
    rpc_status_t status;
    uint16_t     gpioState; /*!< Bit 0 - 3 represent DIG.INPUT4 - 7 states (1=high, 0=low); Bit 4 - 7 are a mask (1=apply state, 0=do not apply state). Bit 8 - 9 represent SPI_SEL0 - 1 states; Bit 10 - 11 are the corresponding mask */
} rpc_gpio_t;

/*!
 * @brief Holds parameters of the RPC_GetTimeFromRTCSync(timestamp : qmc_timestamp_t*) : qmc_status_t and RPC_SetTimeToRTC(timestamp : qmc_timestamp_t*) : qmc_status_t  functions.
 */
typedef struct _rpc_rtc
{
    rpc_status_t    status;
    qmc_timestamp_t timestamp;
    bool            isSetNotGet;
} rpc_rtc_t;

/*!
 * @brief Holds parameters of the RPC_CommitFwUpdate() : qmc_status_t, RPC_RevertFwUpdate() : qmc_status_t and RPC_GetFwUpdateState(state : qmc_fw_update_state_t*) : qmc_status_t functions.
 */
typedef struct _rpc_fwupdate
{
    rpc_status_t          status;
    qmc_fw_update_state_t fwStatus;
    qmc_reset_cause_id_t  resetCause;
    bool                  isReadNotWrite;
    bool                  isCommitNotRevert;
    bool                  isStatusBitsNotResetCause;
} rpc_fwupdate_t;

/*!
 * @brief Holds parameters of the RPC_Reset(cause : qmc_reset_cause_id_t) : qmc_status_t function.
 */
typedef struct _rpc_reset
{
    rpc_status_t         status;
    qmc_reset_cause_id_t cause;
} rpc_reset_t;

/*!
 * @brief This structure is meant to be instantiated once as a global variable. It acts as a the shared memory interface between Cortex M4 and Cortex M7 cores.
 */
typedef struct _rpc_shm
{
    rpc_event_t    events;   /*!< Holds data about SNVS GPIO input events and reset events triggered by the watchdogs. */
    rpc_func_wd_t  funcWd;   /*!< Holds parameters of the RPC_KickFunctionalWatchdog function. */
    rpc_sec_wd_t   secWd;    /*!< Holds parameters of the RPC_KickSecureWatchdog(data : uint8_t*, dataLen : int) : qmc_status_t and RPC_RequestNonceFromSecureWatchdogSync(nonce : uint8_t*, length : int*) : qmc_status_t functions. */
    rpc_gpio_t     gpioOut;  /*!< Holds parameters of the RPC_SetSnvsOutput(gpioState : uint16_t) : qmc_status_t function. */
    rpc_rtc_t      rtc;      /*!< Holds parameters of the RPC_GetTimeFromRTCSync(timestamp : qmc_timestamp_t*) : qmc_status_t and RPC_SetTimeToRTC(timestamp : qmc_timestamp_t*) : qmc_status_t  functions. */
    rpc_fwupdate_t fwUpdate; /*!< Holds parameters of the RPC_CommitFwUpdate() : qmc_status_t, RPC_RevertFwUpdate() : qmc_status_t and RPC_GetFwUpdateState(state : qmc_fw_update_state_t*) : qmc_status_t functions. */
    rpc_reset_t    reset;    /*!< Holds parameters of the RPC_Reset(cause : qmc_reset_cause_id_t) : qmc_status_t function. */
} rpc_shm_t;



/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Kick the functional watchdog referenced by id.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[in] id Watchdog to be kicked
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A functional watchdog with the given ID does not exist.
 * @retval kStatus_QMC_Ok
 * The watchdog was kicked successfully.
 */
qmc_status_t RPC_KickFunctionalWatchdog(rpc_watchdog_id_t id);

#if FEATURE_SECURE_WATCHDOG
/*!
 * @brief Kick the secure watchdog by giving it an encrypted update message (ticket) received from Watchdog Connection
 * Service.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[in] data Pointer to the ticket retrieved from the Secure Watchdog Service.
 * @param[in] dataLen Length of the ticket retrieved from the Secure Watchdog Service.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A given pointer was NULL, or the ticket format was wrong.
 * @retval kStatus_QMC_ErrInternal
 * Returned in case an internal error occurred.
 * @retval kStatus_QMC_ErrSignatureInvalid
 * Returned in case the ticket signature verification failed.
 * @retval kStatus_QMC_ErrRange
 * Returned in case the ticket's timeout was too large and would have lead to an
 * overflow.
 * @retval kStatus_QMC_Ok
 * The secure watchdog was kicked successfully.
 */
qmc_status_t RPC_KickSecureWatchdog(const uint8_t *data, const size_t dataLen);

/*!
 * @brief Request a nonce from the secure watchdog.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time. This function is meant to be
 * called by the Watchdog Connection Service.
 *
 * @param[out] nonce Pointer to a buffer for the nonce
 * @param[in,out] length Contains the available size in the buffer. Gets updated with the bytes written to the buffer.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A given pointer was NULL.
 * @retval kStatus_QMC_ErrNoBufs
 * The supplied buffer for receiving the nonce was too small.
 * @retval kStatus_QMC_ErrInternal
 * Returned in case an internal error occurred.
 * @retval kStatus_QMC_Ok
 * A nonce was successfully received from the secure watchdog.
 */
qmc_status_t RPC_RequestNonceFromSecureWatchdog(uint8_t *nonce, size_t *length);
#endif

/*!
 * @brief Sets the output state of the SPI selection pins in the SNVS domain such
 * that the SPI device referenced by mode is selected.
 *
 * @param[in] mode The SPI device to be selected
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrRange
 * Specified mode was not part of the supported enumeration.
 * @retval kStatus_QMC_Ok
 * The SPI device was selected successfully.
 */
qmc_status_t RPC_SelectPowerStageBoardSpiDevice(qmc_spi_id_t mode);

/*!
 * @brief Request a timestamp from the real time clock implemented in the CM4 core.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[out] timestamp Pointer to write the timestamp to
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A given pointer was NULL.
 * @retval kStatus_QMC_ErrRange
 * An overflow at the timestamp handling occurred.
 * @retval kStatus_QMC_Ok
 * The time was fetched successfully.
 */
qmc_status_t RPC_GetTimeFromRTC(qmc_timestamp_t *timestamp);

/*!
 * @brief Try to set the real time clock to the given timestamp.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[in] timestamp Pointer to get the timestamp from
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A given pointer was NULL.
 * @retval kStatus_QMC_ErrRange
 * An overflow at the timestamp handling occurred.
 * @retval kStatus_QMC_Ok
 * The time was set successfully.
 */
qmc_status_t RPC_SetTimeToRTC(const qmc_timestamp_t *timestamp);

/*!
 * @brief Trigger a reset.
 *
 * This Function shall also create a log entry and send it to the Logging Service.
 * 
 * If an unknown reset cause is given, the reset is still performed with 
 * kQMC_ResetSecureWd as cause to trigger a boot into recovery mode.
 *
 * @param[in] cause Reason a reset is triggered
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_Err
 * The reset could not have been performed (should never happen!).
 */
qmc_status_t RPC_Reset(qmc_reset_cause_id_t cause);

/*!
 * @brief Retrieve the reset-proof firmware update status from the battery backed-up SNVS_LP_GPR register.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[out] state Pointer to write the retrieved FW update state to
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A given pointer was NULL.
 * @retval kStatus_QMC_Ok
 * The firmware update state was retrieved successfully.
 */
qmc_status_t RPC_GetFwUpdateState(qmc_fw_update_state_t *state);

/*!
 * @brief Get the reason, why a reset was triggered by the watchdog implementation of the Cortex M4 core.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[out] cause Pointer to write the reset cause to
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_ErrArgInvalid
 * A given pointer was NULL.
 * @retval kStatus_QMC_Ok
 * The reset cause was retrieved successfully.
 */
qmc_status_t RPC_GetResetCause(qmc_reset_cause_id_t *cause);

/*!
 * @brief Communicate a "commit" request to the bootloader.
 *
 * During next boot the new firmware will be committed and a recovery image will be created. May return
 * kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_Ok
 * The firmware update commit request was stored successfully.
 */
qmc_status_t RPC_CommitFwUpdate(void);

/*!
 * @brief Communicate a "revert" request to the bootloader.
 *
 * During next boot the old firmware will be restored. May return kStatus_QMC_Timeout, if the CM4 core cannot handle the
 * request in time.
 *
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_Ok
 * The firmware update revert request was stored successfully.
 */
qmc_status_t RPC_RevertFwUpdate(void);

#endif /* _API_RPC_H_ */

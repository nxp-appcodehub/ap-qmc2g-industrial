/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _INIT_RPC_H_
#define _INIT_RPC_H_

#include "qmc2_types.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define RPC_SHM_SECTION_NAME "rpmsg_sh_mem"                /*!> must match linker configuration */
#define RPC_SECWD_INIT_DATA_SECTION_NAME "secwd_init_data" /*!> must match linker configuration */

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
    uint8_t ready;                       		  /*!< The CM4 is ready. */
} rpc_secwd_init_data_t;

/*!
 * @brief Lists available watchdos in the system.
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
	 bool                 isResetProcessed;
	 bool                 isGpioProcessed;
	 qmc_reset_cause_id_t resetCause;
	 uint8_t              gpioState;  /*!< Bit 0 - 3 represents DIG.INPUT4 - 7 (1=high, 0=low). */
} rpc_event_t;

/*!
 * @brief Contains return value and status information of remote procedure calls.
 */
typedef struct _rpc_status
{
	 bool         isNew;
	 bool         isProcessed;
	 bool         waitForAsyncCompletionCM4;
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
    fw_state_t			  fwStatus;
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

/*!
 * @brief Cross-core shared memory, referenced by section name.
 *
 * The shared memory is statically initialized here as the CM7 starts first.
 * Note that the shared memory must be initialized by one core before the
 * communication is activated on both cores. Therefore, this can also be done
 * by the CM4 if more appropriate.
 *
 */


#endif /* _INIT_RPC_H_ */

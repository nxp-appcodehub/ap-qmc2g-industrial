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
 * @file rpc_api.c
 * @brief Implements functions for communicating with the CM7.
 *
 * The RPC module allows the CM4 core to communicate with the CM7 core. This
 * module depends on certain preconditions:
 *  - The module is run on top of the IMX RT1176 SOC.
 *  - The module is run on top of a bare-metal environment.
 *  - The GPR_IRQ SW interrupt is required for operation and the RPC_HandleISR()
 *    function must be called from this interrupt.
 *  - The linker has to include a "rpc_shm_section" section with a suitable size
 *    (at least as big as the rpc_shm_t object) in a RAM region that is shared
 *    between the two cores.
 *    Further, this region has to be marked as device memory and non-cacheable (can be done with MPU:
 *    developer.arm.com/documentation/ddi0439/b/Memory-Protection-Unit/About-the-MPU).
 *  - The CM7 implementation must include a RPC client implementation matching to
 *    this module.
 *
 * If the above preconditions are met, then the module processes synchronous
 * remote command request from the CM7 and returns the processing results.
 * Further, the module allows to trigger asynchronous event notifications received by the CM7.
 *
 * The module enables communication using a shared memory region which contains
 * command data and synchronization flags. By atomically modifying the synchronization
 * flags and using an inter-core software interrupt to notify the other side about pending
 * commands / events the communication is established.
 *
 * The static initializer RPC_SHM_STATIC_INIT should be used once before either side starts the communication
 * as it initializes the shared memory.
 * Note that also the CM7 can initialize the shared memory if applicable.
 *
 * After initializing, all public RPC functions can be run. For reentrancy and concurrency
 * assumptions see the detailed API descriptions.
 *
 * Known side-effects / limitations (CM4 and CM7 implementation):
 *  - If a command was requested by the CM7, the CM4 will retrigger the inter-core interrupt until
 *    processing by the CM7 finished. As we do not want to miss incoming messages on the
 *    CM4, this also implies that the CM4 itself loops in the SW interrupt (similar to polling) until
 *    the communication has completed. As the processing by the CM7 should finish quickly
 *    this behaviour is accepted.
 *  - If events from the CM4 side are incoming at a fast pace and the scheduling of the
 *    code aligns in a special way, it may happen that an older event is missed and a newer event
 *    is reported twice!
 *  - Command timeout handling depends on the cooperation of the CM4, as the CM7
 *    side can not know in which state the CM4 is exactly when a timeout occurred. If the CM4
 *    does not cooperate, for example because it hung up, the CM7 has to decide
 *    how to solve this issue (kStatus_QMC_ErrSync is returned).
 *  - If the FreeRTOS timer service queue is full, no event group notification can be send
 *    from the ISR. For CM4 events that means that they will be lost. RPCs will time out,
 *    which is detected by the CM7.
 *  - The inter-core IRQ triggering is globally locked using a FreeRTOS mutex,
 *    so that it is not entered by multiple tasks at the same time (preemptive multi tasking).
 *    Without locking undesired effects (for example: RPC IRQ triggers continuously on CM7) could occur.
 *    While using a mutex allows for higher-important tasks to interrupt this section and perform more
 *    important work, it also has side effects on the availability of the communication:
 *    As the inter-core-IRQ-triggering sequence also disables the inter-core IRQ on the CM7, communication
 *    is globally paused until the preempted locked section is exited again. So if a high-important task
 *    itself uses communication, it must wait for the mutex to become available. The mutex only becomes available
 *    after all tasks with a higher priority than the task with the preempted inter-core-IRQ-triggering sequence
 *    are not ready-to-run anymore. The execution of the inter-core IRQ triggering sequence takes on average 475
 *    cycles (interrupts disabled, without mutex, n = 1000000). Note that also events can not be received during
 *    this period.
 *    However, if this behaviour is not wanted, alternatively a FreeRTOS critical section
 *    (or less invasive suspending the scheduler) might be considered as alternatives.
 *    These alternatives have then a different side-effect: Higher priority tasks can not preempt the task
 *    currently using the inter-core IRQ triggering sequence (max. 25us).
 *  - Accessing the synchronization flags and the event data must be atomic!
 */
#include "api_rpc.h"
#include "rpc/rpc_api.h"
#include "hal/hal.h"
#include "qmc_cm4/qmc_cm4_api.h"
#include "awdg/awdg_api.h"

#include "utils/mem.h"
#include "utils/debug_log_levels.h"
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_SILENT
#endif
#include "utils/debug.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*!
 * @brief Type for RPC handler functions.
 *
 * @param[in,out] pArg Pointer to the shared memory section related to the message.
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous function.
 *  Useful, if part of the operation is performed asynchronously outside the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 *
 */
typedef qmc_status_t (*rpc_handler_func_t)(volatile void *const pArg, bool *const pAsynchronous);

/*!
 * @brief Structure for registering RPC handler functions.
 *
 */
typedef struct
{
    volatile rpc_status_t *pStatus; /*< Pointer to the RPC's associated status struct in the rpc_shm_t struct. */
    volatile void *pData;           /*< Pointer to the RPC's associated shm portion in the rpc_shm_t struct. */
    bool triggerCM7;                /*< If true the CM7 is notified about the processing result. */
    rpc_handler_func_t pHandler;    /*< The handler function that should be called. */
} rpc_call_info_t;

/*******************************************************************************
 * Prototypes
 *******************************************************************************/

/* forward declarations of RPC handlers
 * NOTE: Add a new RPC handler here if needed
 */
#if FEATURE_SECURE_WATCHDOG
static qmc_status_t HandleSecureWatchdogCommandISR(volatile void *const pArg, bool *const pAsynchronous);
static qmc_status_t HandleSecureWatchdogCommandMain(volatile rpc_sec_wd_t *const pRpcSecWd);
#endif
static qmc_status_t HandleFunctionalWatchdogCommand(volatile void *const pArg, bool *const pAsynchronous);
static qmc_status_t HandleGpioOutCommand(volatile void *const pArg, bool *const pAsynchronous);
static qmc_status_t HandleRtcCommand(volatile void *const pArg, bool *const pAsynchronous);
static qmc_status_t HandleFirmwareUpdateCommand(volatile void *const pArg, bool *const pAsynchronous);
static qmc_status_t HandleResetCommand(volatile void *const pArg, bool *const pAsynchronous);

/*******************************************************************************
 * Variables
 *******************************************************************************/
/* Structs describing all available RPCs.
 * NOTE: Add a struct for a new RPC here if needed.
 */
#if FEATURE_SECURE_WATCHDOG
static const rpc_call_info_t gs_kSecWdCallInfo = {&g_rpcSHM.secWd.status, &g_rpcSHM.secWd, true,
                                                  HandleSecureWatchdogCommandISR};
#endif
static const rpc_call_info_t gs_kFuncWdCallInfo   = {&g_rpcSHM.funcWd.status, &g_rpcSHM.funcWd, true,
                                                   HandleFunctionalWatchdogCommand};
static const rpc_call_info_t gs_kGpioOutCallInfo  = {&g_rpcSHM.gpioOut.status, &g_rpcSHM.gpioOut, true,
                                                    HandleGpioOutCommand};
static const rpc_call_info_t gs_kRtcCallInfo      = {&g_rpcSHM.rtc.status, &g_rpcSHM.rtc, true, HandleRtcCommand};
static const rpc_call_info_t gs_kFwUpdateCallInfo = {&g_rpcSHM.fwUpdate.status, &g_rpcSHM.fwUpdate, true,
                                                     HandleFirmwareUpdateCommand};
static const rpc_call_info_t gs_kResetCallInfo    = {&g_rpcSHM.reset.status, &g_rpcSHM.reset, true, HandleResetCommand};

/*!
 * @brief Registers all available RPCs.
 * NOTE: Register a new RPC here if needed.
 */
static const rpc_call_info_t *const gs_kRpcInfoPointers[] = {
#if FEATURE_SECURE_WATCHDOG
    &gs_kSecWdCallInfo,
#endif
    &gs_kFuncWdCallInfo, &gs_kGpioOutCallInfo, &gs_kRtcCallInfo, &gs_kFwUpdateCallInfo, &gs_kResetCallInfo};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Helper function for handling CM7 -> CM4 messages.
 *
 * Must be called in the communication interrupt handler!
 *
 * @param pRpcCallInfo Pointer to a rpc_call_info_t which describes the RPC.
 * @return Indicates if the CM7 should be notified about a processing result.
 * @retval true if CM7 should be notified
 * @retval false if the CM7 should not be notified
 */
static bool ProcessRpc(const rpc_call_info_t *pRpcCallInfo)
{
    assert((NULL != pRpcCallInfo) && (NULL != pRpcCallInfo->pStatus) && (NULL != pRpcCallInfo->pData) &&
           (NULL != pRpcCallInfo->pHandler));

    bool sendTrigger  = false;
    bool asynchronous = false;

    /* check if this command waits for asynchronous completion on the CM4
     * then, the user has to return the final value manually by calling ReturnAsynchronous()
     * (only relevant for operations that requested asynchronous completion, otherwise this flag is false)
     * NOTE: We skip the retriggering of the inter-core interrupt here on purpose, otherwise
     * constantly the interrupt would fire and asynchronous completion would not be possible! */
    if (!pRpcCallInfo->pStatus->waitForAsyncCompletionCM4)
    {
        /* we have a fresh command */
        if (pRpcCallInfo->pStatus->isNew)
        {
            /* process command */
            pRpcCallInfo->pStatus->retval = pRpcCallInfo->pHandler(pRpcCallInfo->pData, &asynchronous);
            /* ensure retval writing retires before updating synchronization flags */
            HAL_DataMemoryBarrier();
            /* if the result should be transmitted synchronously, then directly return it
             * + if the initial handler function fails, we always return synchronously */
            if ((false == asynchronous) || (kStatus_QMC_Ok != pRpcCallInfo->pStatus->retval))
            {
                pRpcCallInfo->pStatus->isNew = false;
                sendTrigger                  = pRpcCallInfo->triggerCM7;
            }
            /* operation should be finalized asynchronously
             * NOTE: We processed the initial handler, but final processing is deferred
             *       Therefore, isNew is not cleared here
             */
            else
            {
                pRpcCallInfo->pStatus->waitForAsyncCompletionCM4 = true;
                sendTrigger                                      = false;
            }
#if defined(DEBUG) && (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_ERROR)
            if (kStatus_QMC_Ok != pRpcCallInfo->pStatus->retval)
            {
                DEBUG_LOG_E(DEBUG_M4_TAG "... failure (%d)!\r\n", pRpcCallInfo->pStatus->retval);
            }
#endif
        }
        /* reissue the interrupt if isProcessed is not cleared (interrupt might have been missed on CM7 side)
         * NOTE: This will lead to permanent reissuing of the interrupt until the CM7 finished processing (polling)
         *       This behaviour is wanted and tolerated (CM7 side should anyhow be finished quickly)
         */
        else if (!pRpcCallInfo->pStatus->isProcessed)
        {
            sendTrigger = pRpcCallInfo->triggerCM7;
        }
    }

    return sendTrigger;
}

/*!
 * @brief Helper for checking if a command should be processed asynchronously.
 *
 * Reentrant and concurrency safe.
 *
 * @param pRpcCallInfo Pointer to a rpc_call_info_t which describes the RPC.
 */
static bool ShouldBeProcessedAsynchronous(const rpc_call_info_t *pRpcCallInfo)
{
    assert(NULL != pRpcCallInfo);

    return pRpcCallInfo->pStatus->waitForAsyncCompletionCM4;
}

/*!
 * @brief Helper function for asynchronously sending return values to the CM7.
 *
 * Requires the command to sets its asynchronous parameter to true!
 *
 * This function is not reentrant and must not be used in parallel with the
 * other RPC functions which use the underlying inter-core communication.
 *
 * @param pRpcCallInfo Pointer to a rpc_call_info_t which describes the RPC.
 * @param retval The return value which should be send to the CM7.
 */
static void ReturnAsynchronous(const rpc_call_info_t *pRpcCallInfo, qmc_status_t retval)
{
    assert(NULL != pRpcCallInfo);

    /* ok, this is indeed an pending asynchronous command */
    if (ShouldBeProcessedAsynchronous(pRpcCallInfo))
    {
        pRpcCallInfo->pStatus->retval = retval;
        /* disable communication interrupt during synchronization flag updating, otherwise
         * an event interrupting after setting "waitForAsyncCompletionCM4 = false" might
         * lead to a double execution of the command handler
         * isProcessed = false -> event -> RPC_HandleISR -> detects asynchronous command
         * as not processed -> processed again
         */
        HAL_DisableInterCoreIRQ();
        /* critical section */

        pRpcCallInfo->pStatus->waitForAsyncCompletionCM4 = false;
        /* ensure clearing of waitForAsyncCompletionCM4 retires before clearing isNew */
        HAL_DataMemoryBarrier();
        pRpcCallInfo->pStatus->isNew = false;

        /* notify CM7 */
        HAL_TriggerInterCoreIRQWhileIRQDisabled();

        HAL_EnableInterCoreIRQ();
        /* critical section left */
    }
}

qmc_status_t RPC_NotifyCM7AboutReset(qmc_reset_cause_id_t resetCause)
{
    qmc_status_t ret = kStatus_QMC_Err;

    /* check if reset cause is supported */
    if((kQMC_ResetFunctionalWd != resetCause) && (kQMC_ResetSecureWd != resetCause))
    {
        ret = kStatus_QMC_ErrArgInvalid;
    }
    else
    {
        g_rpcSHM.events.resetCause = resetCause;
        HAL_DataMemoryBarrier();
        g_rpcSHM.events.isResetProcessed = false;
        /* no need to disable inter-core interrupt explicitly as API concurrency
         * requirements already cover this
         */
        HAL_TriggerInterCoreIRQWhileIRQDisabled();
        ret = kStatus_QMC_Ok;
    }

    return ret;
}

void RPC_NotifyCM7AboutGpioChange(uint8_t gpioInputStatus)
{
    g_rpcSHM.events.gpioState = gpioInputStatus;
    HAL_DataMemoryBarrier();
    g_rpcSHM.events.isGpioProcessed = false;
    /* no need to disable inter-core interrupt explicitly as API concurrency
     * requirements already cover this
     */
    HAL_TriggerInterCoreIRQWhileIRQDisabled();
}

void RPC_HandleISR(void)
{
    bool sendTrigger = false;

    /* process RPC calls */
    for (size_t i = 0U; i < sizeof(gs_kRpcInfoPointers) / sizeof(rpc_call_info_t *); i++)
    {
        sendTrigger |= ProcessRpc(gs_kRpcInfoPointers[i]);
    }

    /* event notification */
    sendTrigger |= !g_rpcSHM.events.isResetProcessed;
    sendTrigger |= !g_rpcSHM.events.isGpioProcessed;

    /* trigger CM7 if needed */
    if (sendTrigger)
    {
        /* we are anyhow in the communication interrupt, no need to disable it */
        HAL_TriggerInterCoreIRQWhileIRQDisabled();
    }
}

#if FEATURE_SECURE_WATCHDOG
bool RPC_ProcessPendingSecureWatchdogCall(qmc_status_t *pRet)
{
    bool processed = false;

    if (ShouldBeProcessedAsynchronous(&gs_kSecWdCallInfo))
    {
        qmc_status_t rtcRet = HandleSecureWatchdogCommandMain(gs_kSecWdCallInfo.pData);
        if (NULL != pRet)
        {
            *pRet = rtcRet;
        }
        processed = true;
    }

    return processed;
}

void RPC_FinishSecureWatchdogCall(qmc_status_t ret)
{
    ReturnAsynchronous(&gs_kSecWdCallInfo, ret);
}
#endif

/*
 * RPC handler functions
 */
#if FEATURE_SECURE_WATCHDOG
/*!
 * @brief Processes a pending secure watchdog command. ISR communication handler part.
 *
 * Just delays processing to the main loop.
 *
 * @param[in,out] pArg Pointer to a rpc_sec_wd_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleSecureWatchdogCommandISR(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    (void)pArg;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleSecureWatchdogCommandISR called!\r\n");

    *pAsynchronous = true;
    return kStatus_QMC_Ok;
}

/*!
 * @brief Processes a pending secure watchdog command. Main function part.
 *
 * @param[in,out] pRpcSecWd Pointer to a rpc_sec_wd_t struct (member of rpc_shm_t).
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * Returned in case of an error due to an invalid ticket format.
 * @retval kStatus_QMC_ErrInternal
 * Returned in case an error occurred at the internal RNG calls.
 * @retval kStatus_QMC_ErrSignatureInvalid
 * Returned in case the ticket signature verification failed.
 * @retval kStatus_QMC_ErrRange
 * Returned in case the ticket's timeout was too large and would have lead to an
 * overflow.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleSecureWatchdogCommandMain(volatile rpc_sec_wd_t *const pRpcSecWd)
{
    assert(NULL != pRpcSecWd);
    qmc_status_t ret = kStatus_QMC_Err;

    /* get nonce */
    if (pRpcSecWd->isNonceNotKick)
    {
        DEBUG_LOG_I(DEBUG_M4_TAG "Getting new nonce...\r\n");

        uint8_t nonce[RPC_SECWD_MAX_MSG_SIZE];
        uint32_t nonceLen = RPC_SECWD_MAX_MSG_SIZE;
        ret               = QMC_CM4_GetNonceAuthenticatedWatchdog(nonce, &nonceLen);
        /* this return values should not be possible in any circumstance */
        assert((kStatus_QMC_ErrArgInvalid != ret) && (kStatus_QMC_ErrNoBufs != ret));
        if (kStatus_QMC_Ok == ret)
        {
            /* copy to RPC SHM */
            (void)vmemcpy(pRpcSecWd->data, nonce, nonceLen);
            pRpcSecWd->dataLen = nonceLen;
        }
    }
    /* validate */
    else
    {
        DEBUG_LOG_I(DEBUG_M4_TAG "Validating new ticket...\r\n");

        uint8_t ticket[AWDG_MAX_TICKET_LENGTH];
        /* latch data length
         * otherwise we might have a race condition which could allow an attacker to mount a buffer overflow */
        uint32_t dataLen = pRpcSecWd->dataLen; 
        /* too big */
        if (dataLen > AWDG_MAX_TICKET_LENGTH)
        {
            ret = kStatus_QMC_ErrArgInvalid;
        }
        else
        {
            /* copy to remove volatile specifier - only compliant way to do this */
            (void)vmemcpy(ticket, pRpcSecWd->data, dataLen);

            ret = QMC_CM4_ProcessTicketAuthenticatedWatchdog(ticket, dataLen);
        }
    }

#if defined(DEBUG) && (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_ERROR)
    if (kStatus_QMC_Ok != ret)
    {
        DEBUG_LOG_E(DEBUG_M4_TAG "... failure (%d)!\r\n", ret);
    }
#endif

    return ret;
}
#endif

/*!
 * @brief Processes a pending functional watchdog command.
 *
 * @param[in,out] pArg Pointer to a rpc_func_wd_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * A functional watchdog with the given ID does not exist.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleFunctionalWatchdogCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    (void)pAsynchronous;

    qmc_status_t ret                           = kStatus_QMC_Err;
    const volatile rpc_func_wd_t *const pRpcWd = pArg;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleFunctionalWatchdogCommand called!\r\n");

    /* latch watchdog id */
    rpc_watchdog_id_t watchdog = pRpcWd->watchdog;

    if ((watchdog >= kRPC_FunctionalWatchdog1) && (watchdog <= kRPC_FunctionalWatchdogLast))
    {
        ret = QMC_CM4_KickFunctionalWatchdogIntsDisabled(watchdog - 1U);
    }
    else
    {
        /* unexpected watchdog id */
        DEBUG_LOG_E(DEBUG_M4_TAG "HandleFunctionalWatchdogCommand got unexpected id (%d)\r\n", watchdog);
        ret = kStatus_QMC_ErrArgInvalid;
    }

    return ret;
}

/*!
 * @brief Processes a pending GPIO command.
 *
 * @param[in,out] pArg Pointer to a rpc_gpio_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the QMC_CM4_RpcReturnAsynchronous function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * One or multiple specifed bit values were not recognized. Hardware state might not reflect expectation!
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleGpioOutCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));

    qmc_status_t ret = kStatus_QMC_Err;
    (void)ret;
    const volatile rpc_gpio_t *const pRpcGpio = pArg;
    (void)pRpcGpio;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleGpioOutCommand called!\r\n");

    /* latch GPIO state */
    uint16_t gpioState = pRpcGpio->gpioState;

    /* validate input */
    if (0U != (gpioState & ~(uint16_t)RPC_OUTPUT_CONTROL_MASK))
    {
        return kStatus_QMC_ErrArgInvalid;
    }

    /* set outputs in mirror */
    if (gpioState & RPC_DIGITAL_OUTPUT4_MODIFY)
    {
        if (gpioState & RPC_DIGITAL_OUTPUT4_DATA)
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput0, 1U);
        }
        else
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput0, 0U);
        }
        assert(kStatus_QMC_Ok == ret);
    }
    if (gpioState & RPC_DIGITAL_OUTPUT5_MODIFY)
    {
        if (gpioState & RPC_DIGITAL_OUTPUT5_DATA)
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput1, 1U);
        }
        else
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput1, 0U);
        }
        assert(kStatus_QMC_Ok == ret);
    }
    if (gpioState & RPC_DIGITAL_OUTPUT6_MODIFY)
    {
        if (gpioState & RPC_DIGITAL_OUTPUT6_DATA)
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput2, 1U);
        }
        else
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput2, 0U);
        }
        assert(kStatus_QMC_Ok == ret);
    }
    if (gpioState & RPC_DIGITAL_OUTPUT7_MODIFY)
    {
        if (gpioState & RPC_DIGITAL_OUTPUT7_DATA)
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput3, 1U);
        }
        else
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsUserOutput3, 0U);
        }
        assert(kStatus_QMC_Ok == ret);
    }
    if (gpioState & RPC_SPI_CS0_MODIFY)
    {
        if (gpioState & RPC_SPI_CS0_DATA)
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsSpiCs0, 1U);
        }
        else
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsSpiCs0, 0U);
        }
        assert(kStatus_QMC_Ok == ret);
    }
    if (gpioState & RPC_SPI_CS1_MODIFY)
    {
        if (gpioState & RPC_SPI_CS1_DATA)
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsSpiCs1, 1U);
        }
        else
        {
            ret = QMC_CM4_SetSnvsGpioPinIntsDisabled(kHAL_SnvsSpiCs1, 0U);
        }
        assert(kStatus_QMC_Ok == ret);
    }

    /* sync GPIO register directly */
    QMC_CM4_SyncSnvsGpioIntsDisabled();

    return kStatus_QMC_Ok;
}

/*!
 * @brief Processes a pending RTC command.
 *
 * @param[in,out] pRpcRtc Pointer to a rpc_rtc_t struct (member of rpc_shm_t).
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrRange
 * An overflow at the timestamp handling occurred.
 * @retval kStatus_QMC_Timeout
 * Accessing the SVNS SRTC peripheral timed out. Try again.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleRtcCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    (void)pAsynchronous;

    qmc_status_t ret             = kStatus_QMC_Err;
    volatile rpc_rtc_t *const pRpcRtc = pArg;
    qmc_timestamp_t newTimestamp = {0U};

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleRtcCommand called!\r\n");

    if (pRpcRtc->isSetNotGet)
    {
        DEBUG_LOG_I(DEBUG_M4_TAG "Setting time...\r\n");
        /* latch timestamp */
        newTimestamp = pRpcRtc->timestamp;
        ret          = QMC_CM4_SetRtcTimeIntsDisabled(&newTimestamp);
    }
    else
    {
        DEBUG_LOG_I(DEBUG_M4_TAG "Getting time...\r\n");
        ret = QMC_CM4_GetRtcTimeIntsDisabled(&newTimestamp);
        if (kStatus_QMC_Ok == ret)
        {
            pRpcRtc->timestamp = newTimestamp;
        }
    }

    return ret;
}

/*!
 * @brief Processes a pending firmware update command.
 *
 * @param[in,out] pArg Pointer to a rpc_fwupdate_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the QMC_CM4_RpcReturnAsynchronous function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleFirmwareUpdateCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    (void)pAsynchronous;

    volatile rpc_fwupdate_t *const pRpcFwupdate = pArg;
    uint8_t currentFwuStatus                    = 0U;
    (void)currentFwuStatus;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleFirmwareUpdateCommand called!\r\n");

    if (pRpcFwupdate->isReadNotWrite)
    {
        if (pRpcFwupdate->isStatusBitsNotResetCause)
        {
            DEBUG_LOG_I(DEBUG_M4_TAG "Reading firmware update status...\r\n");
            pRpcFwupdate->fwStatus = QMC_CM4_GetFwUpdateState();
        }
        else
        {
            DEBUG_LOG_I(DEBUG_M4_TAG "Reading reset cause ...\r\n");
            pRpcFwupdate->resetCause = QMC_CM4_GetResetCause();
        }
    }
    else
    {
        if (pRpcFwupdate->isCommitNotRevert)
        {
            DEBUG_LOG_I(DEBUG_M4_TAG "Communicating a commit request...\r\n");
            QMC_CM4_CommitFwUpdateIntsDisabled();
        }
        else
        {
            DEBUG_LOG_I(DEBUG_M4_TAG "Communicating a revert request...\r\n");
            QMC_CM4_RevertFwUpdateIntsDisabled();
        }
    }

    return kStatus_QMC_Ok;
}

/*!
 * @brief Processes a pending reset command.
 *
 * Resets SoC, this function should never return!
 * If an unknown reset cause is given, the reset is still performed with
 * kQMC_ResetSecureWd as cause to trigger a boot into recovery mode.
 *
 * @param[in,out] pArg Pointer to a rpc_reset_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the QMC_CM4_RpcReturnAsynchronous function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return This function only returns in case of a programming error (reset does not work)!
 * @retval kStatus_QMC_Err
 * If out of some reason the reset was not performed, should not occur!
 */
static qmc_status_t HandleResetCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    const volatile rpc_reset_t *const pRpcReset = pArg;
    qmc_status_t ret                            = kStatus_QMC_Err;
    (void)pAsynchronous;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleResetCommand called!\r\n");

    /* latch reset cause */
    qmc_reset_cause_id_t resetCause = pRpcReset->cause;

    /* check for invalid cause */
    if((kQMC_ResetNone != resetCause) && (kQMC_ResetRequest != resetCause) &&
       (kQMC_ResetFunctionalWd != resetCause) && (kQMC_ResetSecureWd != resetCause))
    {
        QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
    }
    else
    {
        QMC_CM4_ResetSystemIntsDisabled(resetCause);
    }

    /* only reached if reset does not work (programming error)! */
    return ret;
}

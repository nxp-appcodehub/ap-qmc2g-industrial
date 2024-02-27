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
 *    Further, this region has to be marked non-cacheable (can be done with MPU:
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
 *  - If the CM4 finished processing a command and notifies the CM7 about it, it will
 *    retrigger the inter-core interrupt until processing by the CM7 finished.
 *    As we do not want to miss incoming messages on the CM4, this also implies that the
 *    CM4 itself loops in the SW interrupt (similar to polling) until the communication has
 *    completed. As the processing by the CM7 should finish quickly this behaviour is accepted.
 *    This behavior does not interfere with other parts of the CM4 system as the GPR_IRQ SW
 *    interrupt has the lowest priority.
 *  - If events from the CM4 are incoming at a fast pace and the scheduling of the
 *    code aligns in a special way, it may happen that an older event is missed and a newer event
 *    is reported twice!
 *  - Command timeout handling depends on the cooperation of the CM4, as the CM7
 *    can not know in which state the CM4 is exactly when a timeout occurred. If the CM4
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
 *    As the inter-core IRQ triggering sequence also disables the inter-core IRQ on the CM7, communication
 *    is globally paused until the preempted locked section is exited again. So if a high-important task
 *    itself uses communication, it must wait for the mutex to become available. The mutex only becomes available
 *    after all tasks with a higher priority than the task with the preempted inter-core IRQ triggering sequence
 *    are not ready-to-run anymore. The execution of the inter-core IRQ triggering sequence takes on average 475
 *    cycles (interrupts disabled, without mutex, n = 1000000). Note that also events can not be received during
 *    this period.
 *    However, if this behaviour is not wanted, alternatively a FreeRTOS critical section
 *    (or less invasive suspending the scheduler) might be considered as alternatives.
 *    These alternatives have then a different side-effect: Higher priority tasks can not preempt the task
 *    currently using the inter-core IRQ triggering sequence (max. 19us; n = 100000).
 *  - Accessing the synchronization flags and the event data must be atomic!
 */
#include "api_rpc.h"
#include "rpc/rpc_api.h"
#include "hal/hal.h"
#include "qmc_cm4/qmc_cm4_api.h"
#include "awdg/awdg_api.h"
#include "soft_mpu/soft_mpu_api.h"

#include "utils/mem.h"
#include "utils/debug_log_levels.h"
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_SILENT
#endif
#include "utils/debug.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/* see i.MX RT1170 Processer Reference Manual Chapter 3 */
#define CCM_BASE_ADDR ((uintptr_t)0x40CC0000U)
#define CCM_SIZE      (32U * 1024U)
#define CCM_LAST_ADDR (CCM_BASE_ADDR + CCM_SIZE - 1U) /* inclusive */

#define ANADIG_BASE_ADDR ((uintptr_t)0x40C84000U)
#define ANADIG_SIZE      (16U * 1024U)
#define ANADIG_LAST_ADDR (ANADIG_BASE_ADDR + ANADIG_SIZE - 1U) /* inclusive */

/*!
 * @brief Type for RPC handler functions.
 *
 * @param[in,out] pArg Pointer to the shared memory section related to the message.
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
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
    volatile rpc_status_t *pStatus; /*!< Pointer to the RPC's associated status struct in the rpc_shm_t struct. */
    volatile void *pData;           /*!< Pointer to the RPC's associated shm portion in the rpc_shm_t struct. */
    bool triggerCM7;                /*!< If true the CM7 is notified about the processing result. */
    rpc_handler_func_t pHandler;    /*!< The handler function that should be called. */
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
static qmc_status_t HandleMcuTempCommand(volatile void *const pArg, bool *const pAsynchronous);
static qmc_status_t HandleMemWriteCommand(volatile void *const pArg, bool *const pAsynchronous);

/*******************************************************************************
 * Variables
 *******************************************************************************/
/* Structs describing all available RPCs.
 * NOTE: Add a struct for a new RPC here if needed.
 */
#if FEATURE_SECURE_WATCHDOG
/*! @brief Data for maintaining the secure watchdog RPC */
static const rpc_call_info_t gs_kSecWdCallInfo = {&g_rpcSHM.secWd.status, &g_rpcSHM.secWd, true,
                                                  HandleSecureWatchdogCommandISR};
#endif
/*! @brief Data for maintaining the functional watchdog RPC */
static const rpc_call_info_t gs_kFuncWdCallInfo = {&g_rpcSHM.funcWd.status, &g_rpcSHM.funcWd, true,
                                                   HandleFunctionalWatchdogCommand};
/*! @brief Data for maintaining the user GPIO out RPC */
static const rpc_call_info_t gs_kGpioOutCallInfo = {&g_rpcSHM.gpioOut.status, &g_rpcSHM.gpioOut, true,
                                                    HandleGpioOutCommand};
/*! @brief Data for maintaining the RTC RPC */
static const rpc_call_info_t gs_kRtcCallInfo = {&g_rpcSHM.rtc.status, &g_rpcSHM.rtc, true, HandleRtcCommand};
/*! @brief Data for maintaining the firmware update RPC */
static const rpc_call_info_t gs_kFwUpdateCallInfo = {&g_rpcSHM.fwUpdate.status, &g_rpcSHM.fwUpdate, true,
                                                     HandleFirmwareUpdateCommand};
/*! @brief Data for maintaining the reset RPC */
static const rpc_call_info_t gs_kResetCallInfo = {&g_rpcSHM.reset.status, &g_rpcSHM.reset, true, HandleResetCommand};
/*! @brief Data for maintaining the MCU temperature RPC */
static const rpc_call_info_t gs_kMcuTempCallInfo = {&g_rpcSHM.mcuTemp.status, &g_rpcSHM.mcuTemp, true,
                                                    HandleMcuTempCommand};
/*! @brief Data for maintaining the MCU temperature RPC 
 * It is not necessary to trigger the M7 as it pools for the result in case of this RPC. */
static const rpc_call_info_t gs_kMemWriteCallInfo = {&g_rpcSHM.memWrite.status, &g_rpcSHM.memWrite, false,
                                                     HandleMemWriteCommand};

/*!
 * @brief Registers all available RPCs.
 * NOTE: Register a new RPC here if needed.
 */
static const rpc_call_info_t *const gs_kRpcInfoPointers[] = {
#if FEATURE_SECURE_WATCHDOG
    &gs_kSecWdCallInfo,
#endif
    &gs_kFuncWdCallInfo, &gs_kGpioOutCallInfo, &gs_kRtcCallInfo, &gs_kFwUpdateCallInfo,
    &gs_kResetCallInfo,  &gs_kMcuTempCallInfo, &gs_kMemWriteCallInfo};

/*! @brief Soft MPU for checking accesses performed with the memory write RPC 
 *  
 * Blocks the memory write RPC from changing M4-related clock and power settings.
 * The settings must be aligned with the M4-related clock configuration done by 
 * the SBL to offer sufficient protection. 
 */
static const soft_mpu_entry_t gs_kMemWriteRpcMPU[] = {
    /* CCM module
     * deny access to certain clock source, root and gating configuration registers*/
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR, CCM_SIZE, kSOFT_MPU_AccessAllow),
    /* deny clock source 0, 2, 4, 5, 14, 15, 20  configuration */
    /* 0 (OSC_RC_16M) -> CCM, DCDC, GPC, SSARC */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x5000U, 0x20U, kSOFT_MPU_AccessDeny),
    /* 2 (OSC_RC_48M_DIV2) -> M4_SYSTICK_CLK */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x5040U, 0x20U, kSOFT_MPU_AccessDeny),
    /* 4 (OSC_24M), 5 (OSC_24M_CLK) -> PLLs */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x5080U, 0x40U, kSOFT_MPU_AccessDeny),
    /* 14 (SYS_PLL3), 15 (SYS_PLL3_CLK) -> M4_CLK, BUS_CLK, BUS_LPSR_CLK */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x51C0U, 0x40U, kSOFT_MPU_AccessDeny),
    /* 20 (SYS_PLL3_PFD3) -> M4_CLK */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x5280U, 0x20U, kSOFT_MPU_AccessDeny),
    /* deny clock root 1 (M4_CLK), 2 (BUS_CLK), 3 (BUS_LPSR_CLK), 7 (M4_SYSTICK) configuration*/
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x80U, 0x180U, kSOFT_MPU_AccessDeny),
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x380U, 0x80U, kSOFT_MPU_AccessDeny),
    /* deny clock gates 1, 5, 6, 7, 8, 9, 10, 11, 12, 14, 27, 30, 38, 39 configuration */
    /* 1 -> M4 */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x6020U, 0x20U, kSOFT_MPU_AccessDeny),
    /* 5, 6, 7, 8, 9, 10, 11, 12 -> AIPS1, AIPS4, ANADIG, DCDC, SRC, CCM, GPC, SSARC */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x60A0U, 0x100U, kSOFT_MPU_AccessDeny),
    /* 14 -> WDOG1 */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x61C0U, 0x20U, kSOFT_MPU_AccessDeny),
    /* 27 -> M4 LMEM */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x6360U, 0x20U, kSOFT_MPU_AccessDeny),
    /* 30 -> RDC */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x63C0U, 0x20U, kSOFT_MPU_AccessDeny),
    /* 38, 39 -> SNVS_HP, SNVS */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(CCM_BASE_ADDR + 0x64C0U, 0x40U, kSOFT_MPU_AccessDeny),
    
    /* ANALOG / ANADIG module
     * deny accesses to parts of XTALOSC, PLL and whole PMU, but allow accesses to the rest */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(ANADIG_BASE_ADDR, ANADIG_SIZE, kSOFT_MPU_AccessAllow),
    /* deny XTALOSC: OSC_48M_CTRL (M4_SYSTICK_CLK_ROOT), OSC_24M_CTRL (PLLs), OSC_16M_CTRL (CCM, DCDC, GPC, SSARC) */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(ANADIG_BASE_ADDR + 0x10U, 0x14U, kSOFT_MPU_AccessDeny),
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(ANADIG_BASE_ADDR + 0xC0U, 0x4U, kSOFT_MPU_AccessDeny),
    /* deny PLL: SYS_PLL3_* (M4_CLK_ROOT, BUS_CLK_ROOT, BUS_LPSR_CLK_ROOT) */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(ANADIG_BASE_ADDR + 0x210U, 0x24U, kSOFT_MPU_AccessDeny),
    /* deny PMU */
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(ANADIG_BASE_ADDR + 0x500U, 0x2D4U, kSOFT_MPU_AccessDeny),
    SOFT_MPU_STATIC_ENTRY_BASE_SIZE(ANADIG_BASE_ADDR + 0x3C00U, 0x254U, kSOFT_MPU_AccessDeny)};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Helper function for handling CM7 -> CM4 messages.
 *
 * Must be called in the communication interrupt handler!
 *
 * @startuml
 * start
 * :sendTrigger = false
 * asynchronous = false;
 * if (pRpcCallInfo->pStatus->waitForAsyncCompletionCM4) then (false)
 *   if (pRpcCallInfo->pStatus->isNew) then(true)
 *     :pRpcCallInfo->pStatus->retval = pRpcCallInfo->pHandler(pRpcCallInfo->pData, &asynchronous)
 *     HAL_DataMemoryBarrier();
 *     if (asynchronous) then (false)
 *       :pRpcCallInfo->pStatus->isNew = false
 *       sendTrigger = pRpcCallInfo->triggerCM7;
 *     else if (pRpcCallInfo->pStatus->retval) then (!= kStatus_QMC_Ok)
 *       :pRpcCallInfo->pStatus->isNew = false
 *       sendTrigger = pRpcCallInfo->triggerCM7;
 *     else
 *       :pRpcCallInfo->pStatus->waitForAsyncCompletionCM4 = true
 *       sendTrigger = false;
 *     endif
 *   else if (pRpcCallInfo->pStatus->isProcessed) then (false)
 *     :sendTrigger = pRpcCallInfo->triggerCM7;
 *   endif
 * endif
 * :return sendTrigger;
 * stop
 * @enduml
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
            /* if the result should be transmitted synchronously, then directly return it */
            if (false == asynchronous)
            {
                pRpcCallInfo->pStatus->isNew = false;
                sendTrigger                  = pRpcCallInfo->triggerCM7;
            }
            /* + if the initial handler function fails, we always return synchronously */
            else if (kStatus_QMC_Ok != pRpcCallInfo->pStatus->retval)
            {
                pRpcCallInfo->pStatus->isNew = false;
                sendTrigger                  = pRpcCallInfo->triggerCM7;
            }
            /* operation should be finalized asynchronously
             * NOTE: We processed the initial handler, but final processing is deferred.
             *       Therefore, isNew is not cleared here.
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
        else
        {
            /* no action required, "else" added for compliance */
            ;
        }
    }

    return sendTrigger;
}

#if FEATURE_SECURE_WATCHDOG
/*!
 * @brief Helper function for checking if a command should be processed asynchronously.
 *
 * Reentrant and concurrency safe. Just returns pRpcCallInfo->pStatus->waitForAsyncCompletionCM4.
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
 * @startuml
 * start
 * if (ShouldBeProcessedAsynchronous(pRpcCallInfo)) then (true)
 * :pRpcCallInfo->pStatus->retval = retval;
 * :HAL_DisableInterCoreIRQ();
 * :pRpcCallInfo->pStatus->waitForAsyncCompletionCM4 = false
 * HAL_DataMemoryBarrier()
 * pRpcCallInfo->pStatus->isNew = false;
 * :HAL_TriggerInterCoreIRQWhileIRQDisabled();
 * :HAL_EnableInterCoreIRQ();
 * endif
 * stop
 * @enduml
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
#endif

qmc_status_t RPC_NotifyCM7AboutReset(qmc_reset_cause_id_t resetCause)
{
    qmc_status_t ret = kStatus_QMC_Err;

    /* check if reset cause is supported */
    if ((kQMC_ResetFunctionalWd != resetCause) && (kQMC_ResetSecureWd != resetCause))
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
    for (size_t i = 0U; i < (sizeof(gs_kRpcInfoPointers) / sizeof(rpc_call_info_t *)); i++)
    {
        sendTrigger = ProcessRpc(gs_kRpcInfoPointers[i]) || sendTrigger;
    }

    /* event notification */
    sendTrigger = !g_rpcSHM.events.isResetProcessed || sendTrigger;
    sendTrigger = !g_rpcSHM.events.isGpioProcessed || sendTrigger;

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
        qmc_status_t swdgRet = HandleSecureWatchdogCommandMain((volatile rpc_sec_wd_t *const)gs_kSecWdCallInfo.pData);
        if (NULL != pRet)
        {
            *pRet = swdgRet;
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
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
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
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
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

    if ((watchdog >= kRPC_FunctionalWatchdogFirst) && (watchdog <= kRPC_FunctionalWatchdogLast))
    {
        ret = QMC_CM4_KickFunctionalWatchdogIntsDisabled((uint8_t)watchdog);
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
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * One or multiple specified bit values were not recognized. Hardware state might not reflect expectation!
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
    if (0U != (gpioState & ~RPC_OUTPUT_CONTROL_MASK))
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
 * @param[in,out] pArg Pointer to a rpc_rtc_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
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

    qmc_status_t ret                  = kStatus_QMC_Err;
    volatile rpc_rtc_t *const pRpcRtc = pArg;
    qmc_timestamp_t newTimestamp      = {0U};

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
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
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
            pRpcFwupdate->fwStatus = QMC_CM4_GetFwUpdateStateIntsDisabled();
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
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
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
    if ((kQMC_ResetNone != resetCause) && (kQMC_ResetRequest != resetCause) && (kQMC_ResetFunctionalWd != resetCause) &&
        (kQMC_ResetSecureWd != resetCause))
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

/*!
 * @brief Processes a pending MCU temperature command.
 *
 * Measures and returns the current MCU temperature.
 *
 * @param[in,out] pArg Pointer to a rpc_mcu_temp_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleMcuTempCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    volatile rpc_mcu_temp_t *const pRpcMcuTemp = pArg;
    (void)pAsynchronous;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleMcuTempCommand called!\r\n");

    pRpcMcuTemp->temp = HAL_GetMcuTemperature();

    /* can not fail */
    return kStatus_QMC_Ok;
}

/*!
 * @brief Writes a word, halfword or byte to a memory location.
 *
 * Writes the lower 1, 2 or 4 bytes of data to address.
 *
 * Only accesses to certain parts of the CCM and ANADIG memory regions which
 * do not influence the M4's (secure watchdog's) behaviour are allowed (see
 * gs_kMemWriteRpcMPU).
 * Other accesses are seen as security violation and the system will be reset with
 * "kQMC_ResetSecureWd" as reason.
 *
 * @param[in, out] address Pointer to the memory location where data should be written to.
 * @param[in] data Data that should be written to the memory location.
 * @param[in] size Number of bytes which should be written (1, 2, or 4 is supported).
 */
static void secureMemWrite(const uintptr_t address, const uint32_t data, const uint8_t size)
{
    /* additional simple check to ensure the write is within the CCM or ANADIG range */
    if (((address < CCM_BASE_ADDR) || (address > CCM_LAST_ADDR)) &&
        ((address < ANADIG_BASE_ADDR) || (address > ANADIG_LAST_ADDR)))
    {
        /* security violation -> trigger secure watchdog reset */
        QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
    }
    else if (!SOFT_MPU_IsAccessAllowed(gs_kMemWriteRpcMPU, SOFT_MPU_STATIC_ENTRIES(gs_kMemWriteRpcMPU), address,
                                       size))
    {
        /* security violation -> trigger secure watchdog reset */
        QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
    }
    else
    {
        switch (size)
        {
            case 0U:
                return;
            case sizeof(uint8_t):
                *((volatile uint8_t *) address) = (uint8_t) data;
                break;
            case sizeof(uint16_t):
                *((volatile uint16_t *) address) = (uint16_t) data;
                break;
            case sizeof(uint32_t):
                *((volatile uint32_t *) address) = data;
                break;
            default:
                /* security violation -> trigger secure watchdog reset */
                QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
        }
    }
}

/*!
 * @brief Processes a pending mem write command.
 *
 * Performs a memory access as specified by the M7 application in the
 * rpc_mem_write_t struct.
 *
 * Only accesses to certain parts of the CCM and ANADIG memory regions which
 * do not influence the M4's (secure watchdog's) behaviour are allowed (see
 * gs_kMemWriteRpcMPU).
 * Other accesses are seen as security violation and the system will be reset with
 * "kQMC_ResetSecureWd" as reason.
 *
 * @param[in,out] pArg Pointer to a rpc_mem_write_t struct (member of rpc_shm_t).
 * @param[out] pAsynchronous Pointer to a bool, if set true the user has to manually
 *  notify the CM7 about the result of the operation using the ReturnAsynchronous() function.
 *  Useful if part of the operation is performed asynchronously from the communication interrupt
 *  context. Note that if this handler function fails, then the result is directly
 *  returned synchronously.
 * @return A qmc_status_t status code.
 *         In case of a forbidden access the system is reset with "kQMC_ResetSecureWd" as reason.
 * @retval kStatus_QMC_Ok
 * The command was successful.
 */
static qmc_status_t HandleMemWriteCommand(volatile void *const pArg, bool *const pAsynchronous)
{
    assert((NULL != pArg) && (NULL != pAsynchronous));
    volatile rpc_mem_write_t *const pRpcMemWrite = pArg;
    (void)pAsynchronous;

    DEBUG_LOG_I(DEBUG_M4_TAG "HandleMemWriteCommand called!\r\n");

    /* latch */
    qmc_mem_write_t write = pRpcMemWrite->write;

    /* access smaller than a word */
    if (write.accessSize < sizeof(uint32_t))
    {
        /* must be at most one word */
        if (1U == write.dataWords)
        {
            secureMemWrite(write.baseAddress, write.data[0], write.accessSize);
        }
        else
        {
            /* security violation -> trigger secure watchdog reset */
            QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
        }
    }
    /* word access */
    else if (sizeof(uint32_t) == write.accessSize)
    {
        /* multi-word accesses */
        for (size_t i = 0U; i < write.dataWords; i++)
        {
            secureMemWrite(write.baseAddress + (i * sizeof(uint32_t)), write.data[i], write.accessSize);
        }
    }
    else
    {
        /* security violation -> trigger secure watchdog reset */
        QMC_CM4_ResetSystemIntsDisabled(kQMC_ResetSecureWd);
    }

    /* ensure the memory writes have retired before giving control back to the M7 application */
    HAL_DataSynchronizationBarrier();

    return kStatus_QMC_Ok;
}

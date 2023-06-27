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
 * @brief Implements functions for communicating with the CM4.
 *
 * The RPC module allows the CM7 core to communicate with the CM4 core. This
 * module depends on certain preconditions:
 *  - The module is run on top of the IMX RT1176 SOC.
 *  - The module is run on top of FreeRTOS.
 *  - The GPR_IRQ SW interrupt is required for operation and the RPC_HandleISR()
 *    function must be called from this interrupt.
 *  - The linker has to include a "rpc_shm_section" section with a suitable size
 *    (at least as big as the rpc_shm_t object) in a RAM region that is shared
 *    between the two cores.
 *    Further, this region has to be marked non-cacheable (can be done with MPU:
 *    developer.arm.com/documentation/ddi0439/b/Memory-Protection-Unit/About-the-MPU).
 *  - The CM4 implementation must include a RPC client implementation matching to
 *    this module.
 *
 * If the above preconditions are met, then the module can be used to make synchronous
 * remote command request from the CM7 to the CM4 and retrieve the corresponding return values.
 * Further, the CM4 can trigger asynchronous event notifications received by this module.
 *
 * The module enables communication using a shared memory region which contains
 * command data and synchronization flags. By atomically modifying the synchronization
 * flags and using an inter-core software interrupt to notify the other side about pending
 * commands / events the communication is established.
 *
 * The static initializer RPC_SHM_STATIC_INIT should be used once before either side starts the communication
 * as it initializes the shared memory.
 * Note that also the CM4 can initialize the shared memory if applicable.
 * RPC_Init() should be called after the synchronization completed, it initializes
 * the FreeRTOS objects needed for communication and finally enables the communication
 * itself.
 *
 * After initializing, all public RPC functions can be run. Note that they are *not*
 * interrupt-safe! Incoming events can be waited for using the global g_inputButtonEventGroupHandle
 * for user input change events and the global g_systemStatusEventGroupHandle for reset events.
 *
 * Known side-effects / limitations (CM4 and CM7 implementation):
 *  - If a command was requested by the CM7, the CM4 will retrigger the inter-core interrupt until
 *    processing by the CM7 finished. As we do not want to miss incoming messages on the
 *    CM4, this also implies that the CM4 itself loops in the SW interrupt (similar to polling) until
 *    the communication has completed. As the processing by the CM7 should finish quickly
 *    this behavior is accepted.
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
 *    As the inter-core IRQ triggering sequence also disables the inter-core IRQ on the CM7, communication
 *    is globally paused until the preempted locked section is exited again. So if a high-important task
 *    itself uses communication, it must wait for the mutex to become available. The mutex only becomes available
 *    after all tasks with a higher priority than the task with the preempted inter-core-IRQ-triggering sequence
 *    are not ready-to-run anymore. The execution of the inter-core IRQ triggering sequence takes on average 475
 *    cycles (interrupts disabled, without mutex, n = 1000000). Note that also events can not be received during
 *    this period.
 *    If this behavior is not wanted, alternatively a FreeRTOS critical section
 *    (or less invasive suspending the scheduler) might be considered as alternatives.
 *    These alternatives have then a different side-effect: Higher priority tasks can not preempt the task
 *    currently using the inter-core IRQ triggering sequence (max. 25us).
 *  - Accessing the synchronization flags and the event data must be atomic!
 */
#include "rpc/rpc_int.h"

#include <stdbool.h>

#include "MIMXRT1176_cm7.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"

#include "api_qmc_common.h"
#include "api_logging.h"
#include "api_usermanagement.h"
#include "main_cm7.h"

#include "utils/mem.h"
#include "utils/debug_log_levels.h"
#define DEBUG_LOG_PRINT_FUNC PRINTF
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_SILENT
#endif
#include "utils/debug.h"
#include "utils/testing.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern bool g_needsRefresh_getTime;
extern void (*g_TimeChangedCallback)(void);
extern TaskHandle_t g_datalogger_task_handle;

/*!
 * @brief Cross-core shared memory, referenced by section name.
 *
 * The shared memory is statically initialized here as the CM7 starts first.
 * Note that the shared memory must be initialized by one core before the
 * communication is activated on both cores. Therefore, this can also be done
 * by the CM4 if more appropriate.
 *
 */

/* if no SBL is available we initialize the shared memory here */
#if defined(NO_SBL)
#if defined(__ICCARM__) /* IAR Workbench */
#pragma location                                 = RPC_SHM_SECTION_NAME
STATIC_TEST_VISIBLE volatile rpc_shm_t gs_rpcSHM = RPC_SHM_STATIC_INIT;
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION) /* Keil MDK */
STATIC_TEST_VISIBLE volatile rpc_shm_t gs_rpcSHM __attribute__((section(RPC_SHM_SECTION_NAME))) = RPC_SHM_STATIC_INIT;
#elif defined(__GNUC__)
STATIC_TEST_VISIBLE volatile rpc_shm_t gs_rpcSHM __attribute__((section(".data.$" RPC_SHM_SECTION_NAME))) =
    RPC_SHM_STATIC_INIT;
#else
#error "gs_rpcSHM: Please provide your definition of gs_rpcSHM!"
#endif
#else
#if defined(__ICCARM__) /* IAR Workbench */
#pragma location = RPC_SHM_SECTION_NAME
STATIC_TEST_VISIBLE volatile rpc_shm_t gs_rpcSHM;
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION) /* Keil MDK */
STATIC_TEST_VISIBLE volatile rpc_shm_t gs_rpcSHM __attribute__((section(RPC_SHM_SECTION_NAME)));
#elif defined(__GNUC__)
/* This structure must be aligned with CM4 implementation - both size and address. Details can be found in noinit_section.ltd file.*/
STATIC_TEST_VISIBLE volatile rpc_shm_t gs_rpcSHM __attribute__((section(".noinit_RAM5_RPC_API")));
#else
#error "gs_rpcSHM: Please provide your definition of gs_rpcSHM!"
#endif
#endif

static StaticSemaphore_t gs_rpcTriggerIrqMutexStaticBuffer; /*!< Buffer for creating an event group statically. */
static SemaphoreHandle_t gs_rpcTriggerIrqMutex; /*!< Mutex protecting the inter-core IRQ triggering sequence.*/

static StaticEventGroup_t gs_rpcDoneEventGroupStaticBuffer; /*!< Buffer for creating an event group statically. */
STATIC_TEST_VISIBLE EventGroupHandle_t
    gs_rpcDoneEventGroup; /*!< Event group used to wait for communication sequence completion.*/

/* NOTE: Add a rpc_call_data_t struct for registering a new command here. */
#if FEATURE_SECURE_WATCHDOG
STATIC_TEST_VISIBLE rpc_call_data_t gs_secWdCallData = {&gs_rpcSHM.secWd.status, kRPC_EventSecWdDone,
                                                        RPC_SECWD_TIMEOUT_TICKS}; /*!< Sec WD PRC data. */
#endif
STATIC_TEST_VISIBLE rpc_call_data_t gs_funcWdCallData   = {&gs_rpcSHM.funcWd.status, kRPC_EventFuncWdDone,
                                                           RPC_FUNCWD_TIMEOUT_TICKS}; /*!< Func WD RPC data. */
STATIC_TEST_VISIBLE rpc_call_data_t gs_gpioOutCallData  = {&gs_rpcSHM.gpioOut.status, kRPC_EventGpioOutDone,
                                                           RPC_GPIOOUT_TIMEOUT_TICKS}; /*!< GPIO out RPC data. */
STATIC_TEST_VISIBLE rpc_call_data_t gs_rtcCallData      = {&gs_rpcSHM.rtc.status, kRPC_EventRtcDone,
                                                           RPC_RTC_TIMEOUT_TICKS}; /*!< RTC RPC data. */
STATIC_TEST_VISIBLE rpc_call_data_t gs_fwUpdateCallData = {&gs_rpcSHM.fwUpdate.status, kRPC_EventFwUpdateDone,
                                                           RPC_FWUPDATE_TIMEOUT_TICKS}; /*!< FWU RPC data. */
STATIC_TEST_VISIBLE rpc_call_data_t gs_resetCallData    = {&gs_rpcSHM.reset.status, kRPC_EventResetDone,
                                                           RPC_RESET_TIMEOUT_TICKS}; /*!< Reset RPC data. */
/*!
 * @brief Pointers to information about all available RPCs.
 *
 * NOTE: If you want to add a new command, add a rpc_call_data_t struct here.
 */
static rpc_call_data_t *const gs_kRpcDataPointers[] = {
#if FEATURE_SECURE_WATCHDOG
    &gs_secWdCallData,
#endif
    &gs_funcWdCallData, &gs_gpioOutCallData, &gs_rtcCallData, &gs_fwUpdateCallData, &gs_resetCallData};

/*******************************************************************************
 * Code
 ******************************************************************************/

/* put interrupt handler into ITC */
__RAMFUNC(SRAM_ITC_cm7) void RPC_HandleISR(void)
{
    /* xHigherPriorityTaskWoken must be initialized to pdFALSE.
     * See Section 6.2  "Mastering the FreeRTOS TM Real Time Kernel"
     */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult                  = pdFALSE;
    rpc_done_event_t rpcDoneEvents      = 0U;
    EventBits_t gpioEventsSet           = 0U;
    EventBits_t gpioEventsClear         = 0U;
    uint8_t gpioStateLatched            = 0U;
    uint8_t resetCauseLatched           = 0U;
    (void)xResult;
    (void)resetCauseLatched;

    /* process available RPC responses */
    for (size_t i = 0U; i < sizeof(gs_kRpcDataPointers) / sizeof(rpc_call_data_t *); i++)
    {
        assert((NULL != gs_kRpcDataPointers[i]) && (NULL != gs_kRpcDataPointers[i]->pStatus));
        if (!gs_kRpcDataPointers[i]->pStatus->isProcessed)
        {
            if (!gs_kRpcDataPointers[i]->pStatus->isNew)
            {
                rpcDoneEvents |= gs_kRpcDataPointers[i]->completionEvent;
                /* ok we processed the event */
                gs_kRpcDataPointers[i]->pStatus->isProcessed = true;
            }
        }
    }
    /* set bits for finished calls in RPC done event group */
    if (0U != rpcDoneEvents)
    {
        /* NOTE: the setting of the event group bits is deferred to the RTOS daemon task (timer service task)
         * therefore, if the operation should complete immediately before any other task, then the
         * timer service task priority must be higher than the priority of any application task
         * that uses the event group */
        xResult = xEventGroupSetBitsFromISR(gs_rpcDoneEventGroup, rpcDoneEvents, &xHigherPriorityTaskWoken);
#ifdef DEBUG
        /* happens if timer service queue is full
         * command will time out on CM7, which gives FreeRTOS time to process the timer service queue */
        if (pdFAIL == xResult)
        {
            DEBUG_LOG_E(DEBUG_M7_TAG "Timer service queue full - lost RPC done event!\r\n");
        }
#endif
    }

    /* process received GPIO events */
    if (!gs_rpcSHM.events.isGpioProcessed)
    {
        /* while setting the processed flag seems to be at the wrong place here
         * we must do it before latching the data as otherwise we could loose a newer event
         * event data accesses must be atomic in any case as the CM4 does not
         * wait for the processing to complete */
        gs_rpcSHM.events.isGpioProcessed = true;
        /* must latch event data atomically */
        gpioStateLatched = gs_rpcSHM.events.gpioState;

        if (0x00U == (gpioStateLatched & RPC_DIGITAL_INPUT4_DATA))
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT4_LOW;
            gpioEventsClear |= QMC_IOEVENT_INPUT4_HIGH;
        }
        else
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT4_HIGH;
            gpioEventsClear |= QMC_IOEVENT_INPUT4_LOW;
        }

        if (0x00U == (gpioStateLatched & RPC_DIGITAL_INPUT5_DATA))
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT5_LOW;
            gpioEventsClear |= QMC_IOEVENT_INPUT5_HIGH;
        }
        else
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT5_HIGH;
            gpioEventsClear |= QMC_IOEVENT_INPUT5_LOW;
        }

        if (0x00U == (gpioStateLatched & RPC_DIGITAL_INPUT6_DATA))
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT6_LOW;
            gpioEventsClear |= QMC_IOEVENT_INPUT6_HIGH;
        }
        else
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT6_HIGH;
            gpioEventsClear |= QMC_IOEVENT_INPUT6_LOW;
        }

        if (0x00U == (gpioStateLatched & RPC_DIGITAL_INPUT7_DATA))
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT7_LOW;
            gpioEventsClear |= QMC_IOEVENT_INPUT7_HIGH;
        }
        else
        {
            gpioEventsSet |= QMC_IOEVENT_INPUT7_HIGH;
            gpioEventsClear |= QMC_IOEVENT_INPUT7_LOW;
        }

        /* clear previous input state */
        xResult = xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, gpioEventsClear);
#ifdef DEBUG
        /* only happens if timer service queue is full
           event is lost! */
        if (pdFAIL == xResult)
        {
            DEBUG_LOG_E(DEBUG_M7_TAG "Timer service queue full - lost event!\r\n");
        }
#endif
        /* set new input state */
        xResult = xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, gpioEventsSet, &xHigherPriorityTaskWoken);
#ifdef DEBUG
        /* only happens if timer service queue is full
           event is lost! */
        if (pdFAIL == xResult)
        {
            DEBUG_LOG_E(DEBUG_M7_TAG "Timer service queue full - lost event!\r\n");
        }
#endif
    }

    /* processed received RESET events */
    if (!gs_rpcSHM.events.isResetProcessed)
    {
        /* while setting the processed flag seems to be at the wrong place here
         * we must do it before latching the data as otherwise we could loose a newer event
         * event data accesses must be atomic in any case as the CM4 does not
         * wait for the processing to complete */
        gs_rpcSHM.events.isResetProcessed = true;
        /* must latch event data atomically */
        resetCauseLatched = gs_rpcSHM.events.resetCause;
        /* NOTE: reset cause value is currently not used (should only be watchdog reset)!
         *       we also do not clear this event flag as anyhow a reset will happen soon */
        assert((kQMC_ResetSecureWd == resetCauseLatched) || (kQMC_ResetFunctionalWd == resetCauseLatched));
        (void) resetCauseLatched;

        /* watchdog reset direct-to-task notification to logging service */
        xResult = xTaskNotifyFromISR(g_datalogger_task_handle, kDLG_SHUTDOWN_WatchdogReset, eSetBits, &xHigherPriorityTaskWoken);
        /* according to documentation can not fail with action eSetBits
         * and even if it would reset would be anyhow performed, however logs may be lost */
        assert(pdPASS == xResult);
        (void) xResult;

        xResult = xEventGroupSetBitsFromISR(g_systemStatusEventGroupHandle, QMC_SYSEVENT_SHUTDOWN_WatchdogReset,
                                            &xHigherPriorityTaskWoken);
#ifdef DEBUG
        /* happens if timer service queue is full
           event is lost */
        if (pdFAIL == xResult)
        {
            DEBUG_LOG_E(DEBUG_M7_TAG "Timer service queue full - lost event!\r\n");
        }
#endif
        (void) xResult;
    }

    /* ensure all writes retired */
    __DSB();

    /*
     * If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
     * should be requested at the end of the ISR.
     * The macro used is port specific and will be either portYIELD_FROM_ISR() or
     * portEND_SWITCHING_ISR() - refer to the port's documentation.
     */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

qmc_status_t RPC_InitSecureWatchdog(const uint8_t *seed, size_t seedLen, const uint8_t *key, size_t keyLen)
{
    qmc_status_t ret = kStatus_QMC_Err;

    if ((NULL == seed) || (seedLen > RPC_SECWD_MAX_RNG_SEED_SIZE) || (NULL == key) || (keyLen > RPC_SECWD_MAX_PK_SIZE))
    {
        ret = kStatus_QMC_ErrArgInvalid;
    }
    else
    {
        rpc_secwd_init_data_t *pInitData = (rpc_secwd_init_data_t *)RPC_SECWD_INIT_DATA_ADDRESS;
        (void)memcpy(pInitData->rngSeed, seed, seedLen);
        pInitData->rngSeedLen = seedLen;
        (void)memcpy(pInitData->pk, key, keyLen);
        pInitData->pkLen = keyLen;
        ret              = kStatus_QMC_Ok;
    }

    return ret;
}

void RPC_Init(void)
{
    /* initialize RPC trigger IRQ mutex */
    gs_rpcTriggerIrqMutex = xSemaphoreCreateMutexStatic(&gs_rpcTriggerIrqMutexStaticBuffer);
    assert(NULL != gs_rpcTriggerIrqMutex);

    /* initialize RPC done event group */
    gs_rpcDoneEventGroup = xEventGroupCreateStatic(&gs_rpcDoneEventGroupStaticBuffer);
    assert(NULL != gs_rpcDoneEventGroup);

    /* initialize RPC call mutexes */
    for (size_t i = 0U; i < sizeof(gs_kRpcDataPointers) / sizeof(rpc_call_data_t *); i++)
    {
        assert(NULL != gs_kRpcDataPointers[i]);
        gs_kRpcDataPointers[i]->mutex = xSemaphoreCreateMutexStatic(&gs_kRpcDataPointers[i]->mutexStaticBuffer);
        assert(NULL != gs_kRpcDataPointers[i]->mutex);
    }

    /* priority of the inter-core interrupt and communication */
    NVIC_SetPriority(GPR_IRQ_IRQn, RPC_INTERCORE_IRQ_PRIORITY);
    /* enable interrupt */
    NVIC_EnableIRQ(GPR_IRQ_IRQn);
}

/*!
 * @brief Triggers an inter-core interrupt.
 *
 * Before this function is called the inter-core interrupt must have been disabled!
 *
 * Not reentrant! Make sure this function is not interrupted by another invocation
 * of itself!
 * Problem: A task switch occurs after IOMUXC_GPR->GPR7 |= IOMUXC_GPR_GPR7_GINT(1)
 * and another task resumes after NVIC_ClearPendingIRQ(GPR_IRQ_IRQn) with enabling
 * the inter-core interrupt again -> endless interrupts!
 *
 */
static void TriggerInterCoreIRQWhileIRQDisabled(void)
{
    /* ensures memory accesses are retired and visible by the other core before the ISR is triggered */
    __DSB();
    IOMUXC_GPR->GPR7 |= IOMUXC_GPR_GPR7_GINT(1);
    IOMUXC_GPR->GPR7 &= ~IOMUXC_GPR_GPR7_GINT(1);
    /* NOTE this might clear an incoming pending interrupt request from the CM4
     * which occurred after NVIC_DisableIRQ(GPR_IRQ_IRQn) and before NVIC_ClearPendingIRQ(GPR_IRQ_IRQn),
     * but as we poll for communication completion on the CM4 side this does not matter
     * as we will retrigger the interrupt!
     */
    NVIC_ClearPendingIRQ(GPR_IRQ_IRQn);
}

/*!
 * @brief Triggers an inter-core interrupt.
 *
 * Not reentrant! Make sure this function is not interrupted by another invocation
 * of itself (see TriggerInterCoreIRQWhileIRQDisabled())!
 *
 * Version which also disables and enables the inter-core interrupt.
 *
 */
static void TriggerInterCoreIRQ(void)
{
    NVIC_DisableIRQ(GPR_IRQ_IRQn);
    /* barrier for interrupt disabling
     * also ensures memory accesses are retired and visible by the other core before the ISR is triggered
     * also avoids that a pending IRQ handler is executed after the disabling instruction to avoid side effects
     * documentation-service.arm.com/static/5efefb97dbdee951c1cd5aaf?token=
     */
    __DSB();
    __ISB();

    IOMUXC_GPR->GPR7 |= IOMUXC_GPR_GPR7_GINT(1);
    IOMUXC_GPR->GPR7 &= ~IOMUXC_GPR_GPR7_GINT(1);
    /* NOTE this might clear an incoming pending interrupt request from the CM4
     * which occurred after NVIC_DisableIRQ(GPR_IRQ_IRQn) and before NVIC_ClearPendingIRQ(GPR_IRQ_IRQn)
     * but as we poll for communication completion on the CM4 side this does not matter
     * as we will retrigger the interrupt!
     */
    NVIC_ClearPendingIRQ(GPR_IRQ_IRQn);

    NVIC_EnableIRQ(GPR_IRQ_IRQn);
    /* barrier for interrupt enabling
     * avoids that following instructions are executed before calling the ISR to avoid side effects
     * documentation-service.arm.com/static/5efefb97dbdee951c1cd5aaf?token= */
    __DSB();
    __ISB();
}

/*!
 * @brief Checks if the SHM for communication is in a consistent state.
 *
 * Reentrant function.
 *
 * @param[in] pRpcCallData Pointer to a rpc_call_data_t struct describing the remote call.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrSync
 * The given RPC is not in a consistent state.
 * @retval kStatus_QMC_Ok
 * The given RPC is in a consistent state, communication can be established.
 *
 */
static qmc_status_t ConsistencyCheck(const rpc_call_data_t *const pRpcCallData)
{
    qmc_status_t ret        = kStatus_QMC_Err;
    BaseType_t semaphoreRet = pdFALSE;
    (void)semaphoreRet;

    /* should a command still be pending from a previos invocation we have to wait
     * until its completed or else time out and return an inconsistency error
     * if isNew is reset to false by the CM4, we can be sure that the CM4 wrote
     * the return value of the old command
     * then a new command execution can be triggered
     */
    if (pRpcCallData->pStatus->isNew)
    {
        /* if we can not get the inter-core mutex, then continue normally
         *  -> some task has recently performed or will shortly perform an inter-core interrupt */
        if (pdTRUE == xSemaphoreTake(gs_rpcTriggerIrqMutex, RPC_TRIGGER_IRQ_MUTEX_TIMEOUT_TICKS))
        {
            /* maybe the IRQ was missed on the CM4 */
            TriggerInterCoreIRQ();
            semaphoreRet = xSemaphoreGive(gs_rpcTriggerIrqMutex);
            /* would be an programming error */
            assert(pdTRUE == semaphoreRet);
        }

        /* wait */
        vTaskDelay(pRpcCallData->timeoutTicks);
    }

    /* still not processed */
    if (pRpcCallData->pStatus->isNew)
    {
        /* return inconsistency error */
        ret = kStatus_QMC_ErrSync;
    }
    else
    {
        ret = kStatus_QMC_Ok;
    }

    return ret;
}

/*!
 * @brief Prepares the given remote call for execution.
 *
 * This function should be called at the beginning of a remote call request.
 * It acquires the remote call's associated mutex and checks its corresponding
 * SHM region's state for consistency. If the remote calls state is not consistent
 * with the expectations, this functions tries to solve this issue by retriggering
 * the CM4 and waiting for completion of an old remote call request. If this
 * wait should time out an error is returned.
 *
 * In case of an error the acquired mutex is always released.
 * Otherwise, the remote call is left locked.
 *
 * Reentrant, multi-task-safe function.
 *
 * @param[in] pRpcCallData Pointer to a rpc_call_data_t struct describing the remote call.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring the remote call's mutex timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_Ok
 * The operation was successful.
 *
 */
static qmc_status_t PrepareCall(const rpc_call_data_t *const pRpcCallData)
{
    qmc_status_t ret        = kStatus_QMC_Err;
    BaseType_t semaphoreRet = pdFALSE;
    (void)semaphoreRet;

    /* try to take semaphore
     * another task might be processing this call right now,
     * so wait for the max. amount of time the processing might take before timeout
     * (consistency check + processing time)
     */
    if (pdTRUE ==
        xSemaphoreTake(pRpcCallData->mutex, 2 * (pRpcCallData->timeoutTicks + RPC_TRIGGER_IRQ_MUTEX_TIMEOUT_TICKS)))
    {
        /* critical section entered */

        /* check communication state consistency */
        if (kStatus_QMC_Ok != ConsistencyCheck(pRpcCallData))
        {
            /* unlock */
            semaphoreRet = xSemaphoreGive(pRpcCallData->mutex);
            /* would be an programming error */
            assert(pdTRUE == semaphoreRet);
            /* critical section left */

            /* return inconsistency error */
            DEBUG_LOG_W(DEBUG_M7_TAG "RPC call communication state inconsistent!\r\n");
            ret = kStatus_QMC_ErrSync;
        }
        else
        {
            ret = kStatus_QMC_Ok;
        }
    }
    else
    {
        DEBUG_LOG_W(DEBUG_M7_TAG "RPC call lock timeout!\r\n");
        ret = kStatus_QMC_Timeout;
    }

    return ret;
}

/*! @brief Notifies the CM4 about a pending remote call.
 *
 * This function should only be called if calling the PrepareCall() function
 * for the same remote call succeeded before. Further, all necessary input data for
 * the command should have been set in the shared memory region dedicated for communication.
 * This function sets the necessary synchronization flags and notifies the CM4
 * about the pending remote call request.
 *
 * Reentrant function, multiple invocations for different remote calls are
 * (different rpc_call_data_t structures) allowed.
 *
 * @param[in] pRpcCallData Pointer to a rpc_call_data_t struct describing the remote call.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Getting the mutex protecting the inter-core IRQ triggering timed out on the CM7.
 * @retval kStatus_QMC_Ok
 * The operation was successful.
 *
 */
static qmc_status_t NotifyCM4(const rpc_call_data_t *const pRpcCallData)
{
    qmc_status_t ret        = kStatus_QMC_Err;
    BaseType_t semaphoreRet = pdFALSE;
    (void)semaphoreRet;

    /* to allow the use of the preemptive scheduler we have to ensure
     * that the following part is not entered by any other task
     * as this would lead to potential problems with triggering the inter-core
     * interrupt (see also TriggerInterCoreIRQ()) */
    if (pdTRUE == xSemaphoreTake(gs_rpcTriggerIrqMutex, RPC_TRIGGER_IRQ_MUTEX_TIMEOUT_TICKS))
    {
        /* disable communication interrupt during these operations, otherwise
         * an event interrupt after setting "isProcessed = false" might
         * lead to a missed command return (precondition isNew = false):
         * isProcessed = false -> event -> RPC_HandleISR -> detects command
         * as processed -> sets event group flag -> RPC_HandleISR ends ->
         * we clear event group here -> command return missed -> command timeout
         */
        NVIC_DisableIRQ(GPR_IRQ_IRQn);
        /* see triggerInterCoreIRQ for reasoning of barrier */
        __DSB();
        __ISB();

        /* critical section */
        pRpcCallData->pStatus->isProcessed = false;
        __DMB();
        pRpcCallData->pStatus->isNew = true;
        (void)xEventGroupClearBits(gs_rpcDoneEventGroup, pRpcCallData->completionEvent);

        /* notify CM4 */
        TriggerInterCoreIRQWhileIRQDisabled();

        NVIC_EnableIRQ(GPR_IRQ_IRQn);
        /* see triggerInterCoreIRQ for reasoning of barrier */
        __DSB();
        __ISB();
        semaphoreRet = xSemaphoreGive(gs_rpcTriggerIrqMutex);
        /* would be an programming error */
        assert(pdTRUE == semaphoreRet);

        ret = kStatus_QMC_Ok;
    }
    else
    {
        DEBUG_LOG_W(DEBUG_M7_TAG "Acquiring mutex for triggering inter-core IRQ timed out!\r\n");
        ret = kStatus_QMC_Timeout;
    }

    return ret;
}

/*!
 * @brief Notifies the CM4 about a pending remote call and returns its result.
 *
 * This function should only be called if calling the PrepareCall() function
 * for the same remote call succeeded before. Further, all necessary input data for
 * the command should have been set in the shared memory region dedicated for communication.
 * This function sets the necessary synchronization flags and notifies the CM4
 * about the pending remote call request. Then, it waits for the remote command
 * to finish (with a timeout defined in the remote call's rpc_call_data_t
 * struct).
 *
 * Reentrant function, multiple invocations for different remote calls
 * (different rpc_call_data_t structures) are allowed.
 *
 * @param[in] pRpcCallData Pointer to a rpc_call_data_t struct describing the remote call.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * The command execution timed out on the CM7 or the CM4.
 * @retval Any return value the remote command might return.
 *
 */
static qmc_status_t NotifyWaitCM4(const rpc_call_data_t *const pRpcCallData)
{
    qmc_status_t ret        = kStatus_QMC_Err;
    EventBits_t uxEventBits = 0U;

    ret = NotifyCM4(pRpcCallData);
    /* everything ok, no timeout */
    if (kStatus_QMC_Ok == ret)
    {
        /* wait until RPC call finishes */
        uxEventBits = xEventGroupWaitBits(gs_rpcDoneEventGroup, pRpcCallData->completionEvent, pdFALSE, pdFALSE,
                                          pRpcCallData->timeoutTicks);
        if (0x00U == (uxEventBits & pRpcCallData->completionEvent))
        {
            /* timed out
             * set back isProcessed to true
             * the event bit for the timed out command is not set anymore in the communication ISR
             * after this point (so the result of the timed out command is ignored)
             * still, there might be a problem if a command hangs until we are at this point again
             * then, the result of the old (hanging) invocation might be returned
             * to avoid such cases the state of the communication flags is checked beforehand
             * (see ConsistencyCheck)
             */
            DEBUG_LOG_W(DEBUG_M7_TAG "RPC call timed out!\r\n");
            pRpcCallData->pStatus->isProcessed = true;
            ret                                = kStatus_QMC_Timeout;
        }
        else
        {
            /* pass on received response */
            ret = pRpcCallData->pStatus->retval;
        }
    }

    DEBUG_LOG_I(DEBUG_M7_TAG "RPC call return status = %d\r\n.", ret);
    return ret;
}

/*!
 * @brief Remote call cleanup function.
 *
 * Cleans up the remote call after a successful communication. Currently,
 * it only releases the associated lock.
 *
 * Only call this function after an successful call to PrepareCall().
 *
 * @param[in] pRpcCallData Pointer to a rpc_call_data_t struct describing the remote call.
 *
 */
static void RPC_CleanupCall(const rpc_call_data_t *const pRpcCallData)
{
    BaseType_t semaphoreRet = pdFALSE;
    (void)semaphoreRet;

    /* unlock */
    semaphoreRet = xSemaphoreGive(pRpcCallData->mutex);
    /* would be an programming error */
    assert(pdTRUE == semaphoreRet);
    /* critical section left */
}

qmc_status_t RPC_KickFunctionalWatchdog(rpc_watchdog_id_t id)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_funcWdCallData;

    /* check arguments */
    if ((id >= kRPC_FunctionalWatchdog1) && (id <= kRPC_FunctionalWatchdogLast))
    {
        /* valid -> prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }
    else
    {
        /* unexpected watchdog id */
        ret = kStatus_QMC_ErrArgInvalid;
    }

    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.funcWd.watchdog = id;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* clean up remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

#if FEATURE_SECURE_WATCHDOG
qmc_status_t RPC_KickSecureWatchdog(const uint8_t *data, const size_t dataLen)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_secWdCallData;

    /* invalid arguments */
    if ((NULL == data) || (dataLen > RPC_SECWD_MAX_MSG_SIZE))
    {
        DEBUG_LOG_E(DEBUG_M7_TAG "RPC call invalid arguments!\r\n");
        ret = kStatus_QMC_ErrArgInvalid;
    }
    /* valid arguments */
    else
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        (void)vmemcpy(gs_rpcSHM.secWd.data, data, dataLen);
        gs_rpcSHM.secWd.dataLen        = dataLen;
        gs_rpcSHM.secWd.isNonceNotKick = false;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* cleanup remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_RequestNonceFromSecureWatchdog(uint8_t *nonce, size_t *length)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_secWdCallData;

    /* invalid arguments */
    if ((NULL == nonce) || (NULL == length))
    {
        DEBUG_LOG_E(DEBUG_M7_TAG "RPC call invalid arguments!\r\n");
        ret = kStatus_QMC_ErrArgInvalid;
    }
    /* valid arguments */
    else
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.secWd.isNonceNotKick = true;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* call specific return data passing */
        if (kStatus_QMC_Ok == ret)
        {
            if (*length >= gs_rpcSHM.secWd.dataLen)
            {
                (void)vmemcpy(nonce, gs_rpcSHM.secWd.data, gs_rpcSHM.secWd.dataLen);
                *length = gs_rpcSHM.secWd.dataLen;
            }
            else
            {
                ret = kStatus_QMC_ErrNoBufs;
            }
        }

        /* cleanup remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}
#endif

qmc_status_t RPC_SetSnvsOutput(uint16_t gpioState)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_gpioOutCallData;

    /* prepare remote call */
    ret = PrepareCall(pRpcCallData);
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.gpioOut.gpioState = gpioState;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* clean up remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_SelectPowerStageBoardSpiDevice(qmc_spi_id_t mode)
{
    qmc_status_t ret   = kStatus_QMC_Err;
    uint16_t gpioState = 0U;

    /* decode + sanity check */
    ret = kStatus_QMC_Ok;
    switch (mode)
    {
        case kQMC_SpiNone:
            gpioState = (RPC_SPI_CS0_DATA | RPC_SPI_CS1_DATA | RPC_SPI_CS0_MODIFY | RPC_SPI_CS1_MODIFY);
            break;
        case kQMC_SpiMotorDriver:
            gpioState = (RPC_SPI_CS0_MODIFY | RPC_SPI_CS1_MODIFY);
            break;
        case kQMC_SpiAfe:
            gpioState = (RPC_SPI_CS0_DATA | RPC_SPI_CS0_MODIFY | RPC_SPI_CS1_MODIFY);
            break;
        case kQMC_SpiAbsEncoder:
            gpioState = (RPC_SPI_CS1_DATA | RPC_SPI_CS0_MODIFY | RPC_SPI_CS1_MODIFY);
            break;
        default:
            ret = kStatus_QMC_ErrRange;
            break;
    }

    if (kStatus_QMC_Ok == ret)
    {
        ret = RPC_SetSnvsOutput(gpioState);
    }

    return ret;
}

qmc_status_t RPC_GetTimeFromRTC(qmc_timestamp_t *timestamp)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_rtcCallData;

    /* invalid arguments */
    if (NULL == timestamp)
    {
        DEBUG_LOG_E(DEBUG_M7_TAG "RPC call invalid arguments!\r\n");
        ret = kStatus_QMC_ErrArgInvalid;
    }
    /* valid arguments */
    else
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.rtc.isSetNotGet = false;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* call specific return data passing */
        if (kStatus_QMC_Ok == ret)
        {
            *timestamp = gs_rpcSHM.rtc.timestamp;
        }

        /* cleanup remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_SetTimeToRTC(const qmc_timestamp_t *timestamp)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_rtcCallData;

    /* TODO inform about time base change */

    /* invalid arguments */
    if (NULL == timestamp)
    {
        DEBUG_LOG_E(DEBUG_M7_TAG "RPC call invalid arguments!\r\n");
        ret = kStatus_QMC_ErrArgInvalid;
    }
    /* valid arguments */
    else
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.rtc.timestamp   = *timestamp;
        gs_rpcSHM.rtc.isSetNotGet = true;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* cleanup remote call */
        RPC_CleanupCall(pRpcCallData);

        /* make sure BOARD_GetTime synchronizes its internal timestamp with the RTC */
        g_needsRefresh_getTime = true;

        /* run callback function if it is registered */
        if(g_TimeChangedCallback)
        	g_TimeChangedCallback();
    }

    return ret;
}

qmc_status_t RPC_Reset(qmc_reset_cause_id_t cause)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_resetCallData;

    /* check for invalid arguments */
    if ((kQMC_ResetNone != cause) && (kQMC_ResetRequest != cause) && (kQMC_ResetFunctionalWd != cause) &&
        (kQMC_ResetSecureWd != cause))
    {
        cause = kQMC_ResetSecureWd;
        ret   = kStatus_QMC_Ok;
    }
    else
    {
        ret = kStatus_QMC_Ok;
    }

    /* valid arguments */
    if (kStatus_QMC_Ok == ret)
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* issue log entry if reset is about to be performed */
        log_record_t resetLogEntry              = {0};
        resetLogEntry.type                      = kLOG_SystemData;
        resetLogEntry.data.systemData.source    = LOG_SRC_SecureWatchdog;
        resetLogEntry.data.systemData.category  = LOG_CAT_General;
        resetLogEntry.data.systemData.eventCode = LOG_EVENT_ResetRequest;
        /* logging is best effort, even if it would fail, we still want to perform the reset */
        LOG_QueueLogEntry(&resetLogEntry, true);
        /* wait a bit so that log entries can be written (best effort not ensured) */
        vTaskDelay(RPC_WAIT_TICKS_BEFORE_RESET);

        /* call specific data preparation */
        gs_rpcSHM.reset.cause = cause;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* clean up remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_GetFwUpdateState(qmc_fw_update_state_t *state)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_fwUpdateCallData;

    /* invalid arguments */
    if (NULL == state)
    {
        ret = kStatus_QMC_ErrArgInvalid;
    }
    /* valid arguments */
    else
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.fwUpdate.isReadNotWrite            = true;
        gs_rpcSHM.fwUpdate.isStatusBitsNotResetCause = true;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* call specific return data passing */
        if (kStatus_QMC_Ok == ret)
        {
            *state = gs_rpcSHM.fwUpdate.fwStatus;
        }

        /* cleanup remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_GetResetCause(qmc_reset_cause_id_t *cause)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_fwUpdateCallData;

    /* invalid arguments */
    if (NULL == cause)
    {
        ret = kStatus_QMC_ErrArgInvalid;
    }
    /* valid arguments */
    else
    {
        /* prepare remote call */
        ret = PrepareCall(pRpcCallData);
    }

    /* process if no errors occurred */
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.fwUpdate.isReadNotWrite            = true;
        gs_rpcSHM.fwUpdate.isStatusBitsNotResetCause = false;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* call specific return data passing */
        if (kStatus_QMC_Ok == ret)
        {
            *cause = gs_rpcSHM.fwUpdate.resetCause;
        }

        /* cleanup remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_CommitFwUpdate(void)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_fwUpdateCallData;

    /* prepare remote call */
    ret = PrepareCall(pRpcCallData);
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.fwUpdate.isReadNotWrite    = false;
        gs_rpcSHM.fwUpdate.isCommitNotRevert = true;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* clean up remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

qmc_status_t RPC_RevertFwUpdate(void)
{
    qmc_status_t ret                          = kStatus_QMC_Err;
    const rpc_call_data_t *const pRpcCallData = &gs_fwUpdateCallData;

    /* prepare remote call */
    ret = PrepareCall(pRpcCallData);
    if (kStatus_QMC_Ok == ret)
    {
        /* call specific data preparation */
        gs_rpcSHM.fwUpdate.isReadNotWrite    = false;
        gs_rpcSHM.fwUpdate.isCommitNotRevert = false;

        /* notify CM4 and wait for result */
        ret = NotifyWaitCM4(pRpcCallData);

        /* clean up remote call */
        RPC_CleanupCall(pRpcCallData);
    }

    return ret;
}

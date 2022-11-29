/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/*!
 * @file rpc_int.h
 * @brief Implements functions for communicating with the CM4
 *
 * Do not include!
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
 *    currently using the inter-core IRQ triggering sequence (max. 19us, n = 100000).
 *  - Accessing the synchronization flags and the event data must be atomic!
 */
#ifndef _RPC_INT_H_
#define _RPC_INT_H_

#include "qmc_features_config.h"
#include "api_rpc.h"
#include "api_rpc_internal.h"
#include "api_rpc_boot.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* CM4 memory region is aliased in CM7! */
#define RPC_SECWD_INIT_DATA_ADDRESS                                                   \
    (0x2023ff00) /*!< address to which the secure watchdog init data should be copied \
                  */
/* TODO align values with QMC2G project */
#define RPC_INTERCORE_IRQ_PRIORITY                                \
    (15U) /*!< priority for the inter-core communication interrupt \
             (must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) */

#define RPC_INPUT_EVENTS_MASK                                                                              \
    (QMC_IOEVENT_INPUT4_HIGH | QMC_IOEVENT_INPUT4_LOW | QMC_IOEVENT_INPUT5_HIGH | QMC_IOEVENT_INPUT5_LOW | \
     QMC_IOEVENT_INPUT6_HIGH | QMC_IOEVENT_INPUT6_LOW | QMC_IOEVENT_INPUT7_HIGH |                          \
     QMC_IOEVENT_INPUT7_LOW) /*!< mask of all input events managed by RPC code */

/* the locked region needs (n=100000) w.c. 12602 cycles 19us */
#define RPC_TRIGGER_IRQ_MUTEX_TIMEOUT_TICKS \
    pdMS_TO_TICKS(5U) /*!< timeout in ms for acquiring the mutex protecting the inter-core IRQ triggering sequence */
#if FEATURE_SECURE_WATCHDOG
#define RPC_SECWD_TIMEOUT_TICKS pdMS_TO_TICKS(15500U)  /*!< timeout in ms for the secure watchdog RPC processing */
#endif
#define RPC_FUNCWD_TIMEOUT_TICKS  pdMS_TO_TICKS(5U)    /*!< timeout in ms for the functional watchdog RPC processing */
#define RPC_GPIOOUT_TIMEOUT_TICKS pdMS_TO_TICKS(5U)    /*!< timeout in ms for the GPIO out RPC processing */
/* To fullfill timestamp granularity requirements for RPC_GetTimeFromRTC (10ms) this timeout should be < 10ms! */
#define RPC_RTC_TIMEOUT_TICKS      pdMS_TO_TICKS(5U) /*!< timeout in ms for the RTC RPC processing */
#define RPC_FWUPDATE_TIMEOUT_TICKS pdMS_TO_TICKS(5U) /*!< timeout in ms for the FW update RPC processing */
#define RPC_RESET_TIMEOUT_TICKS    pdMS_TO_TICKS(5U) /*!< timeout in ms for the reset RPC processing */

/*!
 * @brief Bit flags for describing remote call completion. Only used internally.
 *
 * NOTE: If you want to add a new command, add a new event bit.
 */
typedef enum
{
#if FEATURE_SECURE_WATCHDOG
    kRPC_EventSecWdDone = (1U << 0U),
#endif
    kRPC_EventFuncWdDone   = (1U << 1U),
    kRPC_EventGpioOutDone  = (1U << 2U),
    kRPC_EventRtcDone      = (1U << 3U),
    kRPC_EventFwUpdateDone = (1U << 4U),
    kRPC_EventResetDone    = (1U << 5U)
} rpc_done_event_t;

/*!
 * @brief Structure describing a remote call. Only used internally.
 *
 */
typedef struct
{
    volatile rpc_status_t *const pStatus;   /*!< Link to the rpc_status_t struct of the call in the g_rpcSHM struct. */
    const rpc_done_event_t completionEvent; /*!< Event flag for notifying about completion of this remote call. */
    const TickType_t timeoutTicks;          /*!< Maximal allowed processing time for the remote call. */
    StaticSemaphore_t mutexStaticBuffer;    /*!< Buffer for creating a mutex statically. */
    SemaphoreHandle_t mutex;                /*!< The remote call's mutex. */
} rpc_call_data_t;

#endif /* _RPC_INT_H_ */

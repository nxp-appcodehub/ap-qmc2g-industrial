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
 * @file rpc_api.h
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
 * The module enables to communication using a shared memory region which contains
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
 *    currently using the inter-core IRQ triggering sequence (max. 19us; n = 100000).
 *  - Accessing the synchronization flags and the event data must be atomic!
 */
#ifndef _RPC_API_H_
#define _RPC_API_H_

#include <stdint.h>
#include <stddef.h>

#include "qmc_features_config.h"
#include "api_qmc_common.h"

/*******************************************************************************
 * API
 *******************************************************************************/

/*!
 * @brief Sets the reset event in the shared memory and triggers an inter-core IRQ.
 *
 * This function is not reentrant and must not be used in parallel with
 * other RPC functions which use the underlying inter-core communication:
 * RPC_Finish*, RPC_NotifyCM7AboutGpioChange, RPC_HandleISR!
 *
 * @startuml
 * start
 * :ret = kStatus_QMC_Err;
 * if () then (resetCause not supported)
 *      :ret = kStatus_QMC_ErrArgInvalid;
 * else (else)
 *      :g_rpcSHM.events.resetCause = resetCause
 *      HAL_DataMemoryBarrier()
 *      g_rpcSHM.events.isResetProcessed = false
 *      HAL_TriggerInterCoreIRQWhileIRQDisabled()
 *      ret = kStatus_QMC_Ok;
 * endif
 * :return ret;
 * stop
 * @enduml
 *
 * @param[in] resetCause A qmc_reset_cause_id_t status reason (only watchdog causes are supported!).
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The CM7 was notification was sent.
 * @retval kStatus_QMC_ErrArgInvalid
 * The specified reset cause is not supported.
 */
qmc_status_t RPC_NotifyCM7AboutReset(qmc_reset_cause_id_t resetCause);

/*!
 * @brief Sets the GPIO input change event in the shared memory and triggers an inter-core IRQ.
 *
 * This function is not reentrant and must not be used in parallel with the
 * other RPC functions which use the underlying inter-core communication:
 * RPC_Finish*, RPC_NotifyCM7AboutReset, RPC_HandleRpcISR!
 *
 * @startuml
 * start
 * :g_rpcSHM.events.gpioState = gpioInputStatus
 * HAL_DataMemoryBarrier()
 * g_rpcSHM.events.isGpioProcessed = false
 * HAL_TriggerInterCoreIRQWhileIRQDisabled();
 * stop
 * @enduml
 *
 * @param[in] gpioInputStatus A uint8_t encoding the status of the four SNVS user inputs (lower 4 bits).
 */
void RPC_NotifyCM7AboutGpioChange(uint8_t gpioInputStatus);

/*!
 * @brief Processes the messages received from the CM7.
 *
 * This function must be called from the GPR_IRQ interrupt!
 * Expects that the shared memory portion is accessible using a global variable named "g_rpcSHM".
 *
 * This function is not reentrant and must not be used in parallel with the
 * other RPC functions which use the underlying inter-core communication:
 * RPC_Finish*, RPC_NotifyCM7AboutReset, RPC_NotifyCM7AboutGpioChange!
 *
 * Worst case execution time on IMX RT1176 CM4 (n = 100000): 
 *      w.c. 23951 cycles 60us (without reset)
 *      w.c. 419525 cycles 1049us (with reset)
 * 
 * @startuml
 * start
 *   :sendTrigger = false;
 *   while(unprocessed kRpcInfoPointer in gs_kRpcInfoPointers)
 *       :sendTrigger |= RPC_Process(kRpcInfoPointer);
 *   endwhile (else)
 *   :sendTrigger |= !g_rpcSHM.events.isResetProcessed
 *   sendTrigger |= !g_rpcSHM.events.isGpioProcessed;
 *   if () then (sendTrigger == true)
 *      :HAL_TriggerInterCoreIRQWhileIRQDisabled();
 *   endif
 * stop
 * @enduml
 *
 */
void RPC_HandleISR(void);

#if FEATURE_SECURE_WATCHDOG
/*!
 * @brief Processes an asynchronous secure watchdog call.
 *
 * Processing is only done if the call is pending.
 *
 * Must not be called from an interrupt!
 *
 * This function is not reentrant and must not be used in parallel with other
 * functions using AWDG_GetNonce(), AWDG_ValidateTicket() or AWDG_DeferWatchdog().
 *
 * @startuml
 * start
 *   :processed = false;
 *   if(RPC_ShouldBeProcessedAsynchronous(&gs_kSecWdCallInfo)) then (true)
 *       :rtcRet = HandleSecureWatchdogCommandMain(gs_kSecWdCallInfo.pData);
 *       if() then (NULL != pRet)
 *           :~*pRet = rtcRet;
 *       endif 
 *       :processed = true;
 *   endif
 *   :return processed;
 * stop
 * @enduml
 *
 * @param[out] pRet Pointer to a buffer where the return value from processing
 * the secure watchdog call is stored (if it was pending).
 * @return A bool encoding if a secure watchdog call was pending and therefore processed.
 */
bool RPC_ProcessPendingSecureWatchdogCall(qmc_status_t *pRet);

/*!
 * @brief Finishes an asynchronous secure watchdog call.
 *
 * Only sends a notification to the CM7 if a call was pending.
 *
 * Must not be called from an interrupt!
 *
 * This function is not reentrant and must not be used in parallel with the
 * other RPC functions which use the underlying inter-core communication:
 * RPC_HandleISR, RPC_NotifyCM7AboutReset, RPC_NotifyCM7AboutGpioChange, RPC_Finish*!
 *
 * @startuml
 * start
 * :RPC_ReturnAsynchronous(&gs_kSecWdCallInfo, ret);
 * stop
 * @enduml
 *
 * @param[in] ret Return value from the secure watchdog call.
 */
void RPC_FinishSecureWatchdogCall(qmc_status_t ret);
#endif 

#endif /* _RPC_API_H_ */

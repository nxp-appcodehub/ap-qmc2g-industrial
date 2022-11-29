/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/*!
 * @file lwdg_unit_api.h
 * @brief Contains the LWDGU API for creating and managing a group of logical watchdogs.
 *
 * # 1 Introduction
 * This document describes the design of the logical watchdog module. The
 * logical watchdog module allows to instantiate and manage a group of virtual
 * watchdogs in software on top of an existing timer source. We call a managed
 * group of virtual watchdogs a logical watchdog unit (LWDGU).
 * It supports the static creation and allocation of logical watchdogs to units
 * (groups). The timeout ticks of the watchdogs are individually programmable,
 * the timeout time in seconds depends on the call interval of the associated
 * tick function. Each watchdog unit has a dedicated grace watchdog which
 * will be started if any watchdog of the group expires. The grace watchdog
 * is configured the same way as the other watchdogs and therefore the grace
 * period is programmable. After the grace watchdog expires the user shall
 * reset the system. The watchdog unit will stay in the expired state until a
 * reset has happened.
 * Calling the tick function of the watchdog unit, notifying other parts about
 * the upcoming system reset and resetting the system is part of the user's responsibility.
 * Further, the user has to ensure that only one of the provided functions
 * accesses a LWDGU unit at any given time (correct locking).
 * See Section 3 for details about the expected module context.
 * This library only provides an hardware-independent overlay to implement
 * logical watchdogs.
 * By using static memory allocations and sticking to C standard APIs,
 * the module can easily integrated into constrained environments. Note that no
 * locking was performed internally as the available locking mechanisms and their
 * side-effects highly depend on the used architecture and environment. Section 2
 * provides more insight into the design principles of this module.
 * Section 4 provides insight into the timing behaviour of the module.
 *
 * # 2 Design Principles
 * ## No internal dynamic memory allocations
 * The logical watchdog module does not depend on the availability of
 * any dynamic memory allocator. Therefore, it can easily be integrated
 * in constraint environments like microcontrollers without an operating
 * system.
 * ## No internal MT and interrupt safety (no internal locking)
 * The public callable functions are designed without considering any kind of
 * concurrent execution. This limitation was accepted as available locking
 * mechanisms and their side-effects do depend on the actual architecture and
 * environment the code is running on. For example, in case of a x64 Linux setup
 * a pthread_mutex lock can be used. On the other hand, on a single processor
 * bare metal environment disabling interrupts is sufficient (assuming some of the
 * functions are called in an interrupt).
 * Locking shall always be performed if two different execution contexts
 * (two threads, different interrupts, one interrupt and the main code, ...)
 * access the same LWDG unit concurrently.
 * However, note that all pointer arguments to the underlying structures are 
 * volatile which allows the underlying structures to be a global variables 
 * accessed by external agents if proper locking is applied.
 *
 * # 3 Expected Module Context
 * The logical watchdog module needs the support of external code and
 * components to achieve its functionality and typical additional tasks:
 * ## Timer Source
 * The LWDGU_Tick function needs to be called by a configured and
 * running timer. How this timer is configured and started is not scope of
 * this module. The user has to make sure that it works right and that
 * the LWDGU_Tick function is indeed called with the right period.
 * ## System Reset Notification
 * If LWDGU_Tick returns kStatus_LWDG_TickJustStarted, the grace watchdog
 * has been started and the system will be reset soon. It is left to the
 * user to take the appropriate actions to notify other parts of the system
 * of the pending reset so that they can prepare for it.
 * ## System Reset
 * If LWDGU_Tick returns kStatus_LWDG_TickJustExpired the user has
 * to ensure that a system reset happens. Otherwise, the logical watchdog
 * module will just stay in the expired state and can not be reset.
 * It is emphasized once again that the module is designed to have no
 * knowledge about the internals of the system and the user has to add
 * platform-specific functionalities.
 *
 * # 4 Timing Behaviour
 * The accuracy of a watchdog's timeout time depends on the tick frequency.
 * For example, suppose we have a watchdog with a tick frequency of 1kHz
 * (1ms interval) and a wanted timeout of 5ms (timeout ticks == 5).
 * Then, the following behaviour occurs (see also the description of LWDG_Init function
 * for more information):
 *
 *      |   K   |       |       |       |       |       |
 *              T       T       T       T       T       T
 *          ^                                        expires !
 * The last kick before an expiration can happen anywhere in the corresponding
 * tick interval (which is then counted as served)!
 * The max. time deviation caused by this behaviour is +1 tick interval.
 *
 * Note that for the internal conversion between timout times and timeout ticks
 * the results are rounded up to the next possible value (whole integers)
 * which can cause an additional deviation of max. +1 tick interval.
 *
 * So the expected accuracy (not considering any deviations of the used clock
 * or side-effects from locking) of the timeout times is:
 *      min: choosen timeout time   max: choosen timeout time + 2 * 1 / choosen tick frequency
 */

#ifndef LWDG_UNIT_API_H
#define LWDG_UNIT_API_H

#include <stdint.h>
#include <stddef.h>
#include "lwdg_int.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define kStatus_LWDGU_GetGraceTriggerLwdgIdNotRunning \
    -1 /**< Returned by LWDGU_GetGraceTriggerLwdgId if grace watchdog is not running. */

/*!
 * @brief Struct defining a logical watchdog unit and the corresponding grace watchdog.
 *
 */
typedef struct
{
    logical_watchdog_t graceLwdg;         /**< The grace watchdog. */
    int16_t graceTriggerLwdg;             /**< The ID of the watchdog which expired and triggered the grace watchdog. */
    volatile logical_watchdog_t *pLwdgs; /**< Array of logical watchdogs belonging to the unit. */
    uint8_t lwdgsCount;                   /**< Count of logical watchdogs belonging to the unit. */
    uint32_t tickFrequencyHz;             /**< The frequency at which the LWDGU_Tick function is called. */
} logical_watchdog_unit_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initialization function for a logical_watchdog_unit_t instance.
 *
 * Initialization function for the logical_watchdog_unit_t structure.
 * Before using any public API function on the instance the initialization must
 * be executed and return no error!
 *
 * Saves a reference to an array of logical watchdogs and their count.
 * Initializes the grace watchdog. Sets the graceTriggerLwdg member to -1.
 * Stores the specified tick frequency for future use.
 *
 * The single watchdogs of the unit have to be initialized separately after an
 * successful call to this function using the LWDGU_InitLwdg function.
 * If a logical watchdog is not initialized it will expire immediately
 * at the next tick if is running.
 *
 * Note that the maximal setable graceTimeoutTimeMs depends on tickFrequencyHz:
 *  floor((graceTimeoutTimeMs * tickFrequencyHz + 999) / 1000) < UINT32_MAX!
 * must hold! Otherwise an error is returned.
 * See Section 4 about the expected timing behaviour of the logical watchdogs.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   :lwdgInitRet = kStatus_LWDG_Err;
 *   :timeoutTicks = 0;
 *   if () then (pUnit == NULL || pLwdgs = NULL || lwdgsCount == 0 || tickFrequencyHz == 0)
 *     :ret = kStatus_LWDG_InvArg;
 *   elseif (LWDG_ConvertTimeoutTimeMsToTimoutTicks(graceTimeoutTimeMs, tickFrequencyHz, &timeoutTicks)) then (!= kStatus_LWDG_Ok) 
 *     :ret = kStatus_LWDG_InvArg; 
 *   elseif (LWDG_Init(&pUnit->graceLwdg, timeoutTicks)) then (!= kStatus_LWDG_Ok) 
 *     :ret = kStatus_LWDG_InvArg; 
 *   else (else) 
 *     :lwdgInitRet = kStatus_LWDG_Ok; 
 *     :uint8_t currentLwdgI = 0;
 *     while () is (currentLwdgI < unit->lwdgsCount)
 *      if (LWDG_Init(&pLwdgs[id], 0U)) then (!= kStatus_LWDG_Ok)
 *          :lwdgInitRet = kStatus_LWDG_InvArg;
 *          break
 *      endif
 *      :currentLwdgI = currentLwdgI + 1;
 *     endwhile
 *     if () then (lwdgInitRet == kStatus_LWDG_Ok)
 *      :pUnit->tickFrequencyHz = tickFrequencyHz;
 *      :pUnit->pLwdgs = pLwdgs;
 *      :pUnit->lwdgsCount = lwdgsCount;
 *      :pUnit->graceTriggerLwdg = kStatus_LWDGU_GetGraceTriggerLwdgIdNotRunning;
 *      :ret = kStatus_LWDG_Ok;
 *     else (else)
 *      :ret = lwdgInitRet;
 *     endif
 *   endif
 *   : return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to the logical_watchdog_unit_t instance that should be initialized.
 * @param[in] graceTimeoutTimeMs
 * Specifies after how many ms without a kick the grace watchdog should expire (if it is running).
 * A timeout time value of 0 disables the grace period and the unit will expire
 * immediately when the first logical watchdog in it expires.
 * @param[in] tickFrequencyHz The frequency at which the LWDGU_Tick function is called.
 * Choose this high enough to achieve an appropriate accuracy, see Section 4!
 * @param[in] pLwdgs Pointer to an array of logical watchdogs.
 * @param[in] lwdgsCount Count of logical watchdogs in the array that pLwdgs points to.
 * Limited to UINT8_MAX (255).
 * @return A lwdg_status_t status code.
 * @retval kStatus_LWDG_InvArg
 * The initialization failed (invalid argument, overflow at timeout ticks calculation).
 * The watchdog unit instance was not modified.
 * @retval kStatus_LWDG_Ok
 * The initialization was successful.
 * Do not forget to also initialize the single watchdogs using LWDGU_Init!
 */
lwdg_status_t LWDGU_Init(volatile logical_watchdog_unit_t *const pUnit,
                         const uint32_t graceTimeoutTimeMs,
                         const uint32_t tickFrequencyHz,
                         volatile logical_watchdog_t *const pLwdgs,
                         const uint8_t lwdgsCount);

/*!
 * @brief Initialization function for a logical watchdog belonging to a logical_watchdog_unit_t instance.
 *
 * Initialization function for a logical watchdog belonging to a logical_watchdog_unit_t instance.
 * The tickFrequencyHz is internally retrieved from the one stored at the LWDGU_Init call.
 *
 * Note that the maximal setable timeoutTimeMs depends on tickFrequencyHz:
 *  floor((timeoutTimeMs * unit.tickFrequencyHz + 999) / 1000) < UINT32_MAX!
 * must hold! Otherwise an error is returned.
 * See Section 4 about the expected timing behaviour of the logical watchdogs.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   :timeoutTicks = 0;
 *   if () then (pUnit == NULL || id >= pUnit->lwdgsCount)
 *     :ret = kStatus_LWDG_TickInvArg;
 *   elseif (LWDG_ConvertTimeoutTimeMsToTimoutTicks(timeoutTimeMs, pUnit->tickFrequencyHz, &timeoutTicks)) then (!= kStatus_LWDG_Ok) 
 *     :ret = kStatus_LWDG_TickInvArg; 
 *   elseif (LWDG_Init(&pUnit->pLwdgs[id], timeoutTicks)) then (!= kStatus_LWDG_Ok) 
 *     :ret = kStatus_LWDG_TickInvArg; 
 *   else (else) 
 *     :ret = kStatus_LWDG_Ok; 
 *   endif 
 *   :return ret; 
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to the logical_watchdog_unit_t instance.
 * @param[in] id ID of the logical watchdog that should be initialized, equals its array position.
 * @param[in] timeoutTimeMs Specifies after how many ms the logical watchdog should expire.
 * @return A lwdg_status_t status code.
 * @retval kStatus_LWDG_InvArg
 * The initialization failed (invalid argument, overflow at timeout ticks calculation). 
 * The watchdog unit instance was not modified.
 * @retval kStatus_LWDG_Ok
 * The initialization was successful, all other public functions related
 * to this watchdog can be used now.
 */
lwdg_status_t LWDGU_InitWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                 const uint8_t id,
                                 const uint32_t timeoutTimeMs);

/*!
 * @brief Ticks a logical watchdog unit instance.
 *
 * Ticks a logical_watchdog_unit_t instance, hence all logical watchdogs
 * which are registered inside the unit will be ticked. If any logical watch-
 * dog expires, then the grace watchdog is started. The ID of the expired
 * logical watchdog is saved and can be retrieved using the
 * LWDGU_GetGraceTriggerLwdgId function. While the grace watchdog is running,
 * the other logical watchdogs are not ticked anymore. When the grace watchdog
 * finally expires, a system reset should be performed. A grace watchdog timeout
 * of 0 ticks will lead directly to expiration of the grace watchdog in case a
 * logical watchdog of the unit expires (no grace period). The function returns
 * the state of the grace watchdog.
 * This function shall always be called by the same execution context, e.g.,
 * a dedicated interrupt or thread.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_TickErr;
 *   :gwdgTickStatus = kStatus_LWDG_TickErr;
 *   :lwdgTickStatus = kStatus_LWDG_TickErr;
 *   if () then (pUnit == NULL)
 *     :ret = kStatus_LWDG_TickInvArg;
 *   elseif (LWDGU_TickGraceWatchdog(pUnit, &gwdgTickStatus)) then (== 1)
 *     :ret = gwdgTickStatus;
 *   else (else)
 *     :ret = kStatus_LWDG_TickNotRunning;
 *     :size_t currentLwdgI = 0;
 *     while () is (currentLwdgI != pUnit->lwdgsCount)
 *       :lwdgTickStatus = LWDG_Tick(&pUnit->pLwdgs[currentLwdogI]);
 *       if () then (lwdgTickStatus == kStatus_LWDG_TickJustExpired)
 *         :ret = LWDGU_StartGraceWatchdog(pUnit);
 *         :pUnit->graceTriggerLwdg = currentLwdogI;
 *         break
 *       endif
 *      :currentLwdgI = currentLwdgI + 1;
 *      endwhile
 *    endif
 *    :return ret;
 *    stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @return A lwdg_tick_status_t status code. 
 * @retval kStatus_LWDGU_TickInvArg
 * A passed pointer was NULL.
 * @retval kStatus_LWDG_TickNotRunning
 * The grace watchdog is not running yet.
 * @retval kStatus_LWDG_TickJustStarted
 * The grace watchdog was started just after this function call (tick)
 * and is running now. Does not appear if the grace timeout is set to 0 ticks
 * (no grace period).
 * @retval kStatus_LWDG_TickRunning
 * The grace watchdog is running and has not expired yet. Does not appear if
 * the grace timeout is set to 0 ticks (no grace period).
 * @retval kStatus_LWDG_TickJustExpired
 * The grace watchdog expired just after this function call (tick).
 * System should be reset!
 * @retval kStatus_LWDG_TickPreviouslyExpired
 * The grace watchdog was already expired before this function call
 * (tick). Only occurs if the user did not carry out a system reset,
 * or when the system reset was not successful.
 */
lwdg_tick_status_t LWDGU_Tick(volatile logical_watchdog_unit_t *const pUnit);

/*!
 * @brief Kick one logical watchdog instance of a logical watchdog unit
 * identified by its ID.
 *
 * Kicks a logical_watchdog_t instance which is part of a logical watchdog
 * unit. If the corresponding watchdog is not started yet, it is started. An
 * already expired watchdog will stay expired and its ticksToTimeout value 
 * will not be modified.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_KickErr;
 *   if () then (pUnit == NULL || id >= pUnit->lwdogsCount)
 *     :ret = kStatus_LWDG_KickInvArg;
 *   else (else)
 *     :ret = LWDG_Kick(&pUnit->pLwdogs[id]);
 *   endif
 *   : return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[in] id ID of the logical watchdog, equals to its array position.
 * @return A lwdg_kick_status_t status code. 
 * @retval kStatus_LWDG_KickInvArg
 * A watchdog with the given ID does not exist.
 * @retval kStatus_LWDG_KickStarted
 * This kick started the watchdog, it was not running before.
 * @retval kStatus_LWDG_KickKicked
 * The watchdog was already running and has been kicked.
 */
lwdg_kick_status_t LWDGU_KickOne(volatile logical_watchdog_unit_t *const pUnit, const uint8_t id);

/*!
 * @brief Get the ID of the logical watchdog which triggered the grace watchdog.
 *
 * Returns the ID of the logical watchdog which triggered the starting of the
 * grace watchdog. If the grace watchdog did not trigger yet the function
 * returns -1.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   if () then (pUnit == NULL || pId == NULL)
 *     :ret = kStatus_LWDG_InvArg;
 *   else (else)
 *     : *id = pUnit->graceTriggerLwdg;
 *     :ret = kStatus_LWDG_Ok;
 *   endif
 *   : return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[out] pId Pointer to a int16_t variable where the result is stored.
 * *pId == -1: The grace watchdog did not trigger yet.
 * else: ID of the logical watchdog which triggered the starting of the grace watchdog.
 * @return A lwdg_status_t status code. 
 * @retval kStatus_LWDG_InvArg
 * A passed pointer was NULL.
 * @retval kStatus_LWDG_Ok
 * The operation was successful.
 */
lwdg_status_t LWDGU_GetGraceTriggerLwdgId(const volatile logical_watchdog_unit_t *const pUnit, int16_t *const pId);

/*!
 * @brief Returns if the specified watchdog of the watchdog unit is currently running.
 *
 * Returns if the specified watchdog of the watchdog unit is currently running.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   if () then (pUnit == NULL || pRunning == NULL || id >= pUnit->lwdogsCount)
 *     :ret = kStatus_LWDG_InvArg;
 *   else (else)
 *     : *running = LWDG_IsRunning(&pUnit->pLwdgs[id]);
 *     :ret = kStatus_LWDG_Ok;
 *   endif
 *   : return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[in] id ID of the logical watchdog, equals to its array position.
 * @param[out] pIsRunning Pointer to a bool receiving the result of the query.
 * false == not running, true == running
 * @return A lwdg_status_t status code. 
 * @retval kStatus_LWDG_InvArg
 * A passed pointer was NULL, or a watchdog with the given ID did not exist.
 * @retval kStatus_LWDG_Ok
 * The operation was successful.
 */
lwdg_status_t LWDGU_IsWatchdogRunning(const volatile logical_watchdog_unit_t *const pUnit,
                                      const uint8_t id,
                                      bool *const pIsRunning);

/*!
 * @brief Changes the timeout ticks value.
 *
 * Changes the timeout ticks value (must be < UINT32_MAX).
 * The timeout ticks value specifies after how many tick intervals without a 
 * kick the watchdog should expire.
 * Note that the result is only effective after the next LWDGU_KickOne call
 * of the affected watchdog.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   if () then (pUnit == NULL || id >= pUnit->lwdogsCount)
 *     :ret = kStatus_LWDG_InvArg;
 *   elseif (LWDG_ChangeTimeoutTicks(&pUnit->pLwdgs[id], newTimeoutTicks)) then (!= kStatus_LWDG_Ok)
 *     :ret = kStatus_LWDG_InvArg;
 *   else (else)
 *     :ret = kStatus_LWDG_Ok;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[in] id ID of the logical watchdog, equals to its array position.
 * @param[in] newTimeoutTicks The new number of ticks until expiration (must be below UINT32_MAX).
 * @return A lwdg_status_t status code.
 * @retval kStatus_LWDG_InvArg
 * A passed pointer was NULL, or a watchdog with the given ID did not exist, or
 * newTimeoutTicks was too large (>= UINT32_MAX).
 * @retval kStatus_LWDG_Ok
 * The operation was successful.
 */
lwdg_status_t LWDGU_ChangeTimeoutTicksWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                               const uint8_t id,
                                               const uint32_t newTimeoutTicks);

/*!
 * @brief Changes the timeout time of the watchdog.
 *
 * Changes the timeout time of the watchdog. The timeout time value specifies
 * after how much time without a kick the watchdog should expire.
 * Note that the result is only effective after the next LWDGU_KickOne call
 * of the affected watchdog.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   :newTimeoutTicks = 0;
 *   if () then (pUnit == NULL || id >= pUnit->lwdogsCount)
 *     :ret = kStatus_LWDG_InvArg;
 *   elseif (LWDG_ConvertTimeoutTimeMsToTimoutTicks(newTimeoutTimeMs, pUnit->tickFrequencyHz, &newTimeoutTicks)) then (!= kStatus_LWDG_Ok) 
 *     :ret = kStatus_LWDG_InvArg; 
 *   elseif (LWDG_ChangeTimeoutTicks(&pUnit->pLwdgs[id], newTimeoutTicks)) then (!= kStatus_LWDG_Ok) 
 *     :ret = kStatus_LWDG_InvArg; 
 *   else (else) 
 *     :ret = kStatus_LWDG_Ok; 
 *   endif 
 *     :return ret; 
 *   stop
 * @enduml
 *
 * Note that the maximal setable timeoutTimeMs depends on tickFrequencyHz:
 *  floor((timeoutTimeMs * unit.tickFrequencyHz + 999) / 1000) < UINT32_MAX!
 * must hold! Otherwise an error is returned.
 * See Section 4 about the expected timing behaviour of the logical watchdogs.
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[in] id ID of the logical watchdog, equals to its array position.
 * @param[in] newTimeoutTimeMs The new timeout time in milliseconds.
 * @return A lwdg_status_t status code. 
 * @retval kStatus_LWDG_InvArg
 * A passed pointer was NULL, or a watchdog with the given ID did not exist, or
 * the internal timeout ticks calculation did overflow.
 * @retval kStatus_LWDG_Ok
 * The operation was successful.
 */
lwdg_status_t LWDGU_ChangeTimeoutTimeMsWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                                const uint8_t id,
                                                const uint32_t newTimeoutTimeMs);

/*!
 * @brief Gets the number of remaining ticks possible before expiration of the specified watchdog in the watchdog unit.
 *
 * Gets the number of remaining ticks possible before expiration of the specified watchdog in the watchdog unit.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   if () then (pUnit == NULL || pRemainingTicks == NULL || id >= pUnit->lwdogsCount)
 *     :ret = kStatus_LWDG_InvArg;
 *   else (else)
 *     : *pRemainingTicks = LWDG_GetRemainingTicks(&pUnit->pLwdgs[id]);
 *     :ret = kStatus_LWDG_Ok;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[in] id ID of the logical watchdog, equals to its array position.
 * @param[out] pRemainingTicks Pointer to a uint32_t receiving the result of the query.
 * @return A lwdg_status_t status code.
 * @retval kStatus_LWDG_InvArg
 * A passed pointer was NULL, or a watchdog with the given ID did not exist.
 * @retval kStatus_LWDG_Ok
 * The operation was successful.
 */
lwdg_status_t LWDGU_GetRemainingTicksWatchdog(const volatile logical_watchdog_unit_t *const pUnit,
                                              const uint8_t id,
                                              uint32_t *const pRemainingTicks);

#endif /* LWDG_UNIT_API_H */

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
 * @file lwdg_unit_api.c
 * @brief Contains the LWDGU implementation for creating and managing a group of logical watchdogs.
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

#include "lwdg_unit_api.h"
#include "lwdg_int.h"

#include <stddef.h>
#include <assert.h>

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Converts a timeout in ms to a tick value the logical watchdog module understands.
 *
 * Converts a timeout in ms to a tick value the logical watchdog module understands. If the given values
 * would lead to an overflow kStatus_LWDG_InvArg is returned.
 * Note that the calculated tick value is rounded up to the next possible one,
 * so the caller has to choose a tick frequency with an appropriate resolution.
 *
 * @startuml
 *   start
 *   :timeoutTicks = 0;
 *   if () then (tickFrequencyHz == 0)
 *     :ret = kStatus_LWDG_InvArg;
 *   else(else)
 *     :timeoutTicks = ((uint64_t) tickFrequencyHz * timeoutTimeMs + 999U) / 1000U;
 *     if () then (timeoutTicks > UINT32_MAX)
 *       :ret = kStatus_LWDG_TickInvArg;
 *     else (else)
 *       : *pTimeoutTicks = (uint32_t) timeoutTicks;
 *       :ret = kStatus_LWDG_Ok;
 *     endif
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] timeoutTimeMs Specifies after how many ms the logical watchdog should expire.
 * @param[in] tickFrequencyHz The frequency at which the LWDG_Tick function is called. Must not be 0!
 * @param[out] pTimeoutTicks A pointer to a uint32_t to which the resulting timeout ticks are written if the conversion
 * was successful. In case of an error the value is not touched.
 * @retval kStatus_LWDG_InvArg
 * The conversion failed (invalid argument, overflow). The *pTimeoutTicks value was not modified.
 * @retval kStatus_LWDG_Ok
 * The conversion was successful.
 */
static inline lwdg_status_t LWDGU_ConvertTimeoutTimeMsToTimoutTicks(const uint32_t timeoutTimeMs,
                                                                    const uint32_t tickFrequencyHz,
                                                                    uint32_t *const pTimeoutTicks)
{
    assert(NULL != pTimeoutTicks);

    lwdg_status_t ret     = kStatus_LWDG_Err;
    uint64_t timeoutTicks = 0ULL;

    /* check for invalid arguments and if an overflow would happen at
     * the timeoutTicks calculation
     */
    if (0U == tickFrequencyHz)
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* until now values seem OK => process further */
    else
    {
        /* rounds up
         * to allow timeout ticks up to UINT32_MAX we perform the calculation with an uint64_t
         * no overflow can happen at this calculation as:
         * (2^32 - 1) * (2^32 - 1) + 999 <= 2^64 - 1
         * 2^64 - 2^33 + 1 + 999 <= 2^64 - 1
         * 999 + 2 <= 2 ^ 33 */
        timeoutTicks = (((uint64_t)tickFrequencyHz * timeoutTimeMs) + 999ULL) / 1000ULL;
        /* would cause an overflow later */
        if (timeoutTicks > UINT32_MAX)
        {
            ret = kStatus_LWDG_TickInvArg;
        }
        else
        {
            *pTimeoutTicks = (uint32_t)timeoutTicks;
            ret             = kStatus_LWDG_Ok;
        }
    }

    return ret;
}

lwdg_status_t LWDGU_Init(volatile logical_watchdog_unit_t *const pUnit,
                         const uint32_t graceTimeoutTimeMs,
                         const uint32_t tickFrequencyHz,
                         volatile logical_watchdog_t *const pLwdgs,
                         const uint8_t lwdgsCount)
{
    lwdg_status_t ret         = kStatus_LWDG_Err;
    lwdg_status_t lwdgInitRet = kStatus_LWDG_Err;
    uint32_t timeoutTicks     = 0U;

    /* invalid arguments */
    if ((NULL == pUnit) || (NULL == pLwdgs) || (0U == lwdgsCount) || (0U == tickFrequencyHz))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* could not convert timeout time to timeout ticks (for grace watchdog) */
    else if (kStatus_LWDG_Ok !=
             LWDGU_ConvertTimeoutTimeMsToTimoutTicks(graceTimeoutTimeMs, tickFrequencyHz, &timeoutTicks))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* initialising grace watchdog failed => can only happen because of invalid arguments */
    else if (kStatus_LWDG_Ok != LWDG_Init(&pUnit->graceLwdg, timeoutTicks))
    {
        /* error handling - not reachable with current implementation of lwdg module */
        ret = kStatus_LWDG_InvArg;
    }
    /* so far everything was ok => initialize rest of unit */
    else
    {
        /* initialize all logical watchdogs to expire immediately
         * (to have a specified behaviour if subsequent initialization is forgotten) */
        lwdgInitRet = kStatus_LWDG_Ok;
        for (uint8_t id = 0U; id < lwdgsCount; id++)
        {
            /* stop if there was an initialization error */
            if (kStatus_LWDG_Ok != LWDG_Init(&pLwdgs[id], 0U))
            {
                /* error handling - not reachable with current implementation of lwdg module */
                lwdgInitRet = kStatus_LWDG_InvArg;
                break;
            }
        }

        /* only complete initialization if there was no error */
        if (kStatus_LWDG_Ok == lwdgInitRet)
        {
            pUnit->tickFrequencyHz  = tickFrequencyHz;
            pUnit->pLwdgs          = pLwdgs;
            pUnit->lwdgsCount       = lwdgsCount;
            pUnit->graceTriggerLwdg = kStatus_LWDGU_GetGraceTriggerLwdgIdNotRunning;
            ret                      = kStatus_LWDG_Ok;
        }
        else
        {
            /* error handling - not reachable with current implementation of lwdg module */
            ret = lwdgInitRet;
        }
    }

    return ret;
}

lwdg_status_t LWDGU_InitWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                 const uint8_t id,
                                 const uint32_t timeoutTimeMs)
{
    lwdg_status_t ret     = kStatus_LWDG_Err;
    uint32_t timeoutTicks = 0U;

    /* pointer is NULL pointer or out of bounds */
    if ((NULL == pUnit) || (id >= pUnit->lwdgsCount))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* could not convert timeout time to timeout ticks */
    else if (kStatus_LWDG_Ok !=
             LWDGU_ConvertTimeoutTimeMsToTimoutTicks(timeoutTimeMs, pUnit->tickFrequencyHz, &timeoutTicks))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* arguments ok, but initialization of lwdg failed */
    else if (kStatus_LWDG_Ok != LWDG_Init(&pUnit->pLwdgs[id], timeoutTicks))
    {
        /* error handling - not reachable with current implementation of lwdg module */
        ret = kStatus_LWDG_InvArg;
    }
    else
    {
        ret = kStatus_LWDG_Ok;
    }

    return ret;
}

/*!
 * @brief Internal helper function for ticking the unit's grace watchdog.
 *
 * Ticks the units grace watchdog and returns if its running. Additionally, the fine-grained status code of the grace
 * watchdog is returned using the pTickStatus pointer.
 *
 * @startuml
 *   start
 *   :gwdg_running = 0;
 *   : *pTickStatus = LWDG_Tick(&pUnit->graceLwdg);
 *   if () then (*pTickStatus != kStatus_LWDG_TickNotRunning)
 *      :gwdg_running = 1;
 *   else (else)
 *      :gwdg_running = 0;
 *   endif
 *   :return gwdg_running;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @param[out] pTickStatus Pointer to a lwdg_tick_status_t instance. The status code of the grace watchdog is written
 * there.
 * @retval 0
 * The grace watchdog is not running.
 * @retval 1
 * The grace watchdog is running.
 */
static inline uint8_t LWDGU_TickGraceWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                              lwdg_tick_status_t *const pTickStatus)
{
    assert((NULL != pUnit) && (NULL != pTickStatus));

    uint8_t gwdg_running = 0U;

    /* tick the grace watchdog and check if it is running
     * LWDG_Tick does never return an error! */
    *pTickStatus = LWDG_Tick(&pUnit->graceLwdg);
    if (kStatus_LWDG_TickNotRunning != *pTickStatus)
    {
        gwdg_running = 1U;
    }
    else
    {
        gwdg_running = 0U;
    }

    return gwdg_running;
}

/*!
 * @brief Internal helper function for starting the units grace watchdog.
 *
 * Starts the units grace watchdog and returns if it was just started or already expired.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_TickErr;
 *   :tick_status = kStatus_LWDG_TickErr;
 *   :LWDG_Kick(&pUnit->graceLwdg);
 *   :tick_status = LWDG_Tick(&pUnit->graceLwdg);
 *   if () then (tick_status == kStatus_LWDG_TickRunning)
 *      :ret = kStatus_LWDG_TickJustStarted;
 *   else (else)
 *      :ret = kStatus_LWDG_TickJustExpired;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pUnit Pointer to a logical_watchdog_unit_t instance.
 * @retval kStatus_LWDG_TickJustStarted
 * The grace watchdog was just started.
 * @retval kStatus_LWDG_TickJustExpired
 * The grace watchdog was just started and expired already (timeout of zero).
 */
static inline lwdg_tick_status_t LWDGU_StartGraceWatchdog(volatile logical_watchdog_unit_t *const pUnit)
{
    assert(NULL != pUnit);

    lwdg_kick_status_t kick_status = kStatus_LWDG_KickErr;
    lwdg_tick_status_t tick_status = kStatus_LWDG_TickErr;

    /* start grace watchdog with kick
     * counts this tick time interval as served
     * as the grace watchdog can not be served, we will undo this
     * with a tick afterwards
     * getting any status code apart from kStatus_LWDG_KickStarted should be
     * logically impossible (when the grace watchdog is running this code should not be reached) */
    kick_status = LWDG_Kick(&pUnit->graceLwdg);
    assert(kStatus_LWDG_KickStarted == kick_status);
    /* for production code */
    (void)kick_status;

    /* remove served tick time interval with an additional tick
     * also check if the grace watchdog expired already
     * (grace timeout ticks were set to 0 -> no grace period)
     * getting any status code apart from kStatus_LWDG_TickRunning or kStatus_LWDG_TickJustExpired should be
     * logically impossible (grace watchdog was freshly started by us beforehand)
     * LWDG_Tick does never return an error! */
    tick_status = LWDG_Tick(&pUnit->graceLwdg);
    assert((kStatus_LWDG_TickRunning == tick_status) || (kStatus_LWDG_TickJustExpired == tick_status));

    /* if we get kStatus_LWDG_TickRunning the grace watchdog was actually just started
     * as we kicked it before */
    return (kStatus_LWDG_TickRunning == tick_status) ? kStatus_LWDG_TickJustStarted : kStatus_LWDG_TickJustExpired;
}

lwdg_tick_status_t LWDGU_Tick(volatile logical_watchdog_unit_t *const pUnit)
{
    lwdg_tick_status_t ret            = kStatus_LWDG_TickErr;
    lwdg_tick_status_t gwdgTickStatus = kStatus_LWDG_TickErr;
    lwdg_tick_status_t lwdgTickStatus = kStatus_LWDG_TickErr;

    /* pointer is NULL pointer */
    if (NULL == pUnit)
    {
        ret = kStatus_LWDG_TickInvArg;
    }
    /* if grace watchdog runs, do not process other watchdogs
     * (anyhow a system reset will be pending) */
    else if (1U == LWDGU_TickGraceWatchdog(pUnit, &gwdgTickStatus))
    {
        ret = gwdgTickStatus;
    }
    /* tick logical watchdogs if grace watchdog is not running yet */
    else
    {
        /* if the loop runs through without break no logical watchdog expired */
        ret = kStatus_LWDG_TickNotRunning;
        for (uint8_t currentLwdogI = 0U; currentLwdogI < pUnit->lwdgsCount; currentLwdogI++)
        {
            /* start grace watchdog if logical watchdog did expire
             * LWDG_Tick does never return an error ! */
            lwdgTickStatus = LWDG_Tick(&pUnit->pLwdgs[currentLwdogI]);
            if (kStatus_LWDG_TickJustExpired == lwdgTickStatus)
            {
                /* start grace watchdog */
                ret = LWDGU_StartGraceWatchdog(pUnit);
                /* save which logical watchdog triggered the starting of the grace watchdog */
                pUnit->graceTriggerLwdg = currentLwdogI;
                break;
            }
        }
    }

    return ret;
}

lwdg_kick_status_t LWDGU_KickOne(volatile logical_watchdog_unit_t *const pUnit, const uint8_t id)
{
    lwdg_kick_status_t ret = kStatus_LWDG_KickErr;

    /* pointer is NULL pointer or out of bounds */
    if ((NULL == pUnit) || (id >= pUnit->lwdgsCount))
    {
        ret = kStatus_LWDG_KickInvArg;
    }
    /* arguments ok, proceed */
    else
    {
        ret = LWDG_Kick(&pUnit->pLwdgs[id]);
    }

    return ret;
}

lwdg_status_t LWDGU_GetGraceTriggerLwdgId(const volatile logical_watchdog_unit_t *const pUnit, int16_t *const pId)
{
    lwdg_status_t ret = kStatus_LWDG_Err;

    /* pointers are NULL pointers */
    if ((NULL == pUnit) || (NULL == pId))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* arguments ok, proceed */
    else
    {
        *pId = pUnit->graceTriggerLwdg;
        ret = kStatus_LWDG_Ok;
    }

    return ret;
}

lwdg_status_t LWDGU_IsWatchdogRunning(const volatile logical_watchdog_unit_t *const pUnit,
                                      const uint8_t id,
                                      bool *const pIsRunning)
{
    lwdg_status_t ret = kStatus_LWDG_Err;

    /* pointers are NULL pointers or out of bounds */
    if ((NULL == pUnit) || (NULL == pIsRunning) || (id >= pUnit->lwdgsCount))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* arguments ok, proceed */
    else
    {
        *pIsRunning = LWDG_IsRunning(&pUnit->pLwdgs[id]);
        ret          = kStatus_LWDG_Ok;
    }

    return ret;
}

lwdg_status_t LWDGU_ChangeTimeoutTicksWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                               const uint8_t id,
                                               const uint32_t newTimeoutTicks)
{
    lwdg_status_t ret = kStatus_LWDG_Err;

    /* pointer is NULL pointer or out of bounds or newTimeoutTicks negative
     * or newTimeoutTicks is too large */

    if ((NULL == pUnit) || (id >= pUnit->lwdgsCount))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* try to set new timout ticks value */
    else if (kStatus_LWDG_Ok != LWDG_ChangeTimeoutTicks(&pUnit->pLwdgs[id], newTimeoutTicks))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* all ok */
    else
    {
        ret = kStatus_LWDG_Ok;
    }

    return ret;
}

lwdg_status_t LWDGU_ChangeTimeoutTimeMsWatchdog(volatile logical_watchdog_unit_t *const pUnit,
                                                const uint8_t id,
                                                const uint32_t newTimeoutTimeMs)
{
    lwdg_status_t ret        = kStatus_LWDG_Err;
    uint32_t newTimeoutTicks = 0U;

    /* pointer is NULL pointer or out of bounds or newTimeoutTicks negative
     * or newTimeoutTicks is too large */
    if ((NULL == pUnit) || (id >= pUnit->lwdgsCount))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* could not convert timeout time to timeout ticks */
    else if (kStatus_LWDG_Ok !=
             LWDGU_ConvertTimeoutTimeMsToTimoutTicks(newTimeoutTimeMs, pUnit->tickFrequencyHz, &newTimeoutTicks))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* try to set new timout ticks value */
    else if (kStatus_LWDG_Ok != LWDG_ChangeTimeoutTicks(&pUnit->pLwdgs[id], newTimeoutTicks))
    {
        /* error handling - not reachable with current implementation of lwdg module */
        ret = kStatus_LWDG_InvArg;
    }
    /* all ok */
    else
    {
        ret = kStatus_LWDG_Ok;
    }

    return ret;
}

lwdg_status_t LWDGU_GetRemainingTicksWatchdog(const volatile logical_watchdog_unit_t *const pUnit,
                                              const uint8_t id,
                                              uint32_t *const pRemainingTicks)
{
    lwdg_status_t ret = kStatus_LWDG_Err;

    /* pointers are NULL pointers or out of bounds */
    if ((NULL == pUnit) || (NULL == pRemainingTicks) || (id >= pUnit->lwdgsCount))
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* arguments ok, proceed */
    else
    {
        *pRemainingTicks = LWDG_GetRemainingTicks(&pUnit->pLwdgs[id]);
        ret               = kStatus_LWDG_Ok;
    }

    return ret;
}

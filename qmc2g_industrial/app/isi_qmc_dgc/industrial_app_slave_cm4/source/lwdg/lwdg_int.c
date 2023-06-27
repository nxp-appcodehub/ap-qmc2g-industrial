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
 * @file lwdg_int.c
 * @brief Contains the LWG module implementation for creating and managing one logical watchdog.
 *
 * This module is designed for internal use only, for public use see the LWDGU module!
 * Pointer arguments are not checked for validity!
 * 
 * All pointer arguments are volatile which allows the logical_watchdog_t object
 * to be a global variable accessed by external agents if proper locking is 
 * applied.
 *
 */

#include "lwdg_int.h"

#include <stddef.h>
#include <assert.h>

/*******************************************************************************
 * Code
 ******************************************************************************/

lwdg_status_t LWDG_Init(volatile logical_watchdog_t *const pDog, const uint32_t timeoutTicks)
{
    assert(NULL != pDog);

    lwdg_status_t ret = kStatus_LWDG_Err;

    /* timeoutTicks must stay smaller as UINT32_MAX, as we still add 1 at LWDG_Kick*/
    if (timeoutTicks >= UINT32_MAX)
    {
        ret = kStatus_LWDG_InvArg;
    }
    /* no overflow possible => we can initialize the watchdog */
    else
    {
        pDog->isRunning = false;
        pDog->isExpired = false;
        /* actually does not matter here, the watchdog is anyhow only started later */
        pDog->ticksToTimeout = timeoutTicks + 1U;
        pDog->timeoutTicks   = timeoutTicks;

        ret = kStatus_LWDG_Ok;
    }

    return ret;
}

lwdg_tick_status_t LWDG_Tick(volatile logical_watchdog_t *const pDog)
{
    assert(NULL != pDog);
    /* check our assumptions on the watchdog state */
    assert((false == pDog->isRunning) || (true == pDog->isExpired) || (pDog->ticksToTimeout > 0U));

    lwdg_tick_status_t ret = kStatus_LWDG_TickErr;

    /* not running? */
    if (false == pDog->isRunning)
    {
        ret = kStatus_LWDG_TickNotRunning;
    }
    /* already timed out some time ago? */
    else if (true == pDog->isExpired)
    {
        ret = kStatus_LWDG_TickPreviouslyExpired;
    }
    /* tick and check if expired?
     * pDog->ticksToTimeout is never zero before this expression (see also assert) */
    else if (0U == --pDog->ticksToTimeout)
    {
        pDog->isExpired = true;
        ret             = kStatus_LWDG_TickJustExpired;
    }
    /* running and no timeout */
    else
    {
        ret = kStatus_LWDG_TickRunning;
    }

    return ret;
}

lwdg_kick_status_t LWDG_Kick(volatile logical_watchdog_t *const pDog)
{
    assert(NULL != pDog);
    /* check our assumptions about the watchdogs timeoutTicks value */
    assert(pDog->timeoutTicks < UINT32_MAX);

    lwdg_kick_status_t ret = kStatus_LWDG_KickErr;

    /* ticksToTimeout is set to timeoutTicks + 1 to count the whole time interval
     * in that the kick happened as served
     * otherwise, a watchdog with a timeoutTicks value of 1 would always expire at
     * the next tick (timeout time could be zero)
     * an already expired watchdog is not modified */
    if (false == pDog->isExpired)
    {
        pDog->ticksToTimeout = pDog->timeoutTicks + 1U;
    }

    /* start watchdog if not running already */
    if (false == pDog->isRunning)
    {
        pDog->isRunning = true;
        ret             = kStatus_LWDG_KickStarted;
    }
    else
    {
        ret = kStatus_LWDG_KickKicked;
    }

    return ret;
}

bool LWDG_IsRunning(const volatile logical_watchdog_t *const pDog)
{
    assert(NULL != pDog);

    return pDog->isRunning;
}

lwdg_status_t LWDG_ChangeTimeoutTicks(volatile logical_watchdog_t *const pDog, const uint32_t newTimeoutTicks)
{
    assert(NULL != pDog);

    lwdg_status_t ret = kStatus_LWDG_Err;

    /* timeoutTicks must stay smaller as UINT32_MAX, as we still add 1 at LWDG_Kick*/
    if (newTimeoutTicks >= UINT32_MAX)
    {
        ret = kStatus_LWDG_InvArg;
    }
    else
    {
        pDog->timeoutTicks = newTimeoutTicks;
        ret                = kStatus_LWDG_Ok;
    }

    return ret;
}

uint32_t LWDG_GetRemainingTicks(const volatile logical_watchdog_t *const pDog)
{
    assert(NULL != pDog);

    return pDog->ticksToTimeout;
}

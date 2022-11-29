/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/*!
 * @file lwdg_int.h
 * @brief Contains the LWG module API for creating and managing one logical watchdog.
 *
 * This module is designed for internal use only, for public use see the LWDGU module!
 * Pointer arguments are not checked for validity!
 * 
 * All pointer arguments are volatile which allows the logical_watchdog_t object
 * to be a global variable accessed by external agents if proper locking is 
 * applied.
 *
 */

#ifndef LWDG_INT_H
#define LWDG_INT_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Struct defining a single logical watchdog.
 *
 */
typedef struct
{
    bool isRunning;         /**< Indicates whether watchdog is running: false (stopped), true (running) */
    bool isExpired;         /**< Indicates whether watchdog is expired: false (not expired), true (expired) */
    uint32_t ticksToTimeout; /**< Remaining ticks until the watchdog expires. */
    uint32_t timeoutTicks;   /**< Number of ticks to which the watchdog will be initialized. */
} logical_watchdog_t;

/*!
 * @brief Return values for LWDG_Tick.
 *
 */
typedef enum
{
    kStatus_LWDG_TickInvArg            = -2,
    kStatus_LWDG_TickErr               = -1,
    kStatus_LWDG_TickNotRunning        = 1,
    kStatus_LWDG_TickJustStarted       = 2,
    kStatus_LWDG_TickRunning           = 3,
    kStatus_LWDG_TickJustExpired       = 4,
    kStatus_LWDG_TickPreviouslyExpired = 5
} lwdg_tick_status_t;

/*!
 * @brief Return values for LWDG_Kick.
 *
 */
typedef enum
{
    kStatus_LWDG_KickInvArg  = -2,
    kStatus_LWDG_KickErr     = -1,
    kStatus_LWDG_KickStarted = 1,
    kStatus_LWDG_KickKicked  = 2
} lwdg_kick_status_t;

/*!
 * @brief Generic return values for LWDG functions.
 *
 */
typedef enum
{
    kStatus_LWDG_InvArg = -2,
    kStatus_LWDG_Err    = -1,
    kStatus_LWDG_Ok     = 1
} lwdg_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initialization function for a logical_watchdog_t instance.
 *
 * Initialization function for a logical_watchdog_t structure.
 * Before using any public API function on the instance the initialization must
 * be executed and return no error!
 *
 * Initializes the watchdog with the wanted timeout ticks. The timeout ticks
 * value specifies after how many tick intervals without a kick the watchdog
 * should expire.
 * The initial state of the watchdog is set to not running and not expired. It
 * can be started by performing an initial kick (LWDG_Kick).
 *
 * Fails if timeoutTicks is >= UINT32_MAX as this would lead to an overflow!
 *
 * Examples for a freshly initialized watchdog with a timeout ticks value of 1
 * (T == tick, K == kick):
 *
 * 1) Expires:
 *
 *      |   K   |       |
 *              T       T
 *                   expires !
 *
 * 2) Keeps running:
 *
 *      |   K   | K     |     K |   K   | ... (keeps running)
 *              T       T       T       T
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   if () then (timeoutTicks >= UINT32_MAX)
 *     :ret = kStatus_LWDG_InvArg;
 *   else(else)
 *     :pDog->isRunning      = false;
 *     :pDog->isExpired      = false;
 *     :pDog->ticksToTimeout = timeoutTicks + 1;
 *     :pDog->timeoutTicks   = timeoutTicks;
 *
 *     :ret = kStatus_LWDG_Ok;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pDog Pointer to the logical_watchdog_t instance that should be initialized.
 * @param[in] timeoutTicks
 * Specifies after how many tick intervals without a kick the watchdog should expire.
 * @return A lwdg_status_t status code.
 * @retval kStatus_LWDGU_InvArg
 * The initialization failed (invalid argument). The watchdog instance was not modified.
 * @retval kStatus_LWDGU_Ok
 * The initialization was successful, all other public functions can be used now.
 */
lwdg_status_t LWDG_Init(volatile logical_watchdog_t *const pDog, const uint32_t timeoutTicks);

/*!
 * @brief Ticks a logical watchdog instance.
 *
 * Ticks (counts down the internal timer) a running logical_watchdog_t instance
 * and returns its current status after the tick.
 * This function shall always be called by the same execution context, e.g.,
 * a dedicated counter interrupt or thread.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_TickErr;
 *   if () then (pDog->isRunning == false)
 *     :ret = kStatus_LWDG_TickNotRunning;
 *   elseif () then (pDog->isExpired == true)
 *     :ret = kStatus_LWDG_TickPreviouslyExpired;
 *   elseif () then (--pDog->ticksToTimeout == 0)
 *     :pDog->isExpired = true;
 *     :ret = kStatus_LWDG_TickJustExpired;
 *   else (else)
 *     :ret = kStatus_LWDG_TickRunning;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pDog Pointer to a logical_watchdog_t instance.
 * @return A lwdg_tick_status_t status code.
 * @retval kStatus_LWDG_TickNotRunning
 * The logical watchdog is not running yet.
 * @retval kStatus_LWDG_TickRunning
 * The logical watchdog is running and has not expired yet.
 * @retval kStatus_LWDG_TickJustExpired
 * The logical watchdog expired just after this function call (tick).
 * @retval kStatus_LWDG_TickPreviouslyExpired
 * The logical watchdog was already expired before this function call
 * (tick).
 */
lwdg_tick_status_t LWDG_Tick(volatile logical_watchdog_t *const pDog);

/*!
 * @brief Kicks a logical watchdog instance.
 *
 * Kicks a logical_watchdog_t instance. If the corresponding watchdog is not
 * started yet, it is started. An already expired watchdog will stay expired
 * and its ticksToTimeout value will not be modified.
 * At a kick the timeout is set to timeoutTicks + 1 as the current tick
 * interval is counted as served and the user task should have another full
 * timeout period to serve the watchdog again. Otherwise, a timeout value of
 * 1 would not work at all as it always would decrement the current count to
 * zero at the next tick.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_KickErr;
 *   if () then (pDog->isExpired == false)
 *      :pDog->ticksToTimeout = pDog->timeoutTicks + 1;
 *   endif
 *   if () then (pDog->isRunning  == false)
 *     :pDog->isRunning = true;
 *     :ret = kStatus_LWDG_KickStarted;
 *   else (else)
 *     :ret = kStatus_LWDG_KickKicked;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pDog Pointer to a logical_watchdog_t instance.
 * @return A lwdg_kick_status_t status code.
 * @retval kStatus_LWDG_KickStarted
 * This kick started the watchdog, it was not running before.
 * @retval kStatus_LWDG_KickKicked
 * The watchdog was already running before and has been kicked.
 */
lwdg_kick_status_t LWDG_Kick(volatile logical_watchdog_t *const pDog);

/*!
 * @brief Returns if the specified watchdog is currently running.
 *
 * Returns if the specified watchdog is currently running.
 * Only an interface for accessing pDog->running.
 *
 * @param[in] pDog Pointer to a logical_watchdog_t instance.
 * @return A boolean.
 * @retval false not running
 * @retval true running
 */
bool LWDG_IsRunning(const volatile logical_watchdog_t *const pDog);

/*!
 * @brief Changes the timeout ticks value.
 *
 * Changes the timeout ticks value. The timeout ticks value specifies
 * after how many tick intervals without a kick the watchdog should expire.
 * Note that the result is only effective after the next LWDG_Kick call.
 *
 * @startuml
 *   start
 *   :ret = kStatus_LWDG_Err;
 *   if () then (newTimeoutTicks >= UINT32_MAX)
 *     :ret = kStatus_LWDG_InvArg;
 *   else (else)
 *     :pDog->timeoutTicks = newTimeoutTicks;
 *     :ret = kStatus_LWDG_Ok;
 *   endif
 *   :return ret;
 *   stop
 * @enduml
 *
 * @param[in] pDog Pointer to a logical_watchdog_t instance.
 * @param[in] newTimeoutTicks The new number of ticks until expiration (must be below UINT32_MAX).
 * @return A lwdg_status_t status code.
 * @retval kStatus_LWDGU_InvArg
 * The operation failed (invalid argument). The timeout ticks value was not modified.
 * @retval kStatus_LWDGU_Ok
 * The operation was successful, the new timeout value will be active after the
 * next kick operation.
 */
lwdg_status_t LWDG_ChangeTimeoutTicks(volatile logical_watchdog_t *const pDog, const uint32_t newTimeoutTicks);

/*!
 * @brief Gets the number of remaining ticks until expiration of the specified watchdog.
 *
 * Gets the number of remaining ticks until expiration of the specified watchdog.
 * Only an interface for accessing pDog->ticksToTimeout.
 *
 * @param[in] pDog Pointer to a logical_watchdog_t instance.
 * @return The number of remaining ticks as uint32_t.
 */
uint32_t LWDG_GetRemainingTicks(const volatile logical_watchdog_t *const pDog);

#endif /* LWDG_INT_H */

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "ThreadSafetyWorkaround.h"
#include "se_session.h"
#include "fsl_sss_api.h"
#include "fsl_sss_se05x_apis.h"
#include "fsl_sss_se05x_types.h"
#include "nxLog_App.h"

/* macros from fsl_sss_se05x_apis.c */
#if (USE_RTOS)
#define LOCK_TXN(lock)                                   \
    LOG_D("Trying to Acquire Lock");                     \
    if (xSemaphoreTake(lock, portMAX_DELAY) == pdTRUE) { \
        LOG_D("LOCK Acquired");                          \
    }                                                    \
    else {                                               \
        LOG_D("LOCK Acquisition failed");                \
    }
#define UNLOCK_TXN(lock)                  \
    LOG_D("Trying to Released Lock");     \
    if (xSemaphoreGive(lock) == pdTRUE) { \
        LOG_D("LOCK Released");           \
    }                                     \
    else {                                \
        LOG_D("LOCK Releasing failed");   \
    }
#elif (__GNUC__ && !AX_EMBEDDED)
#define LOCK_TXN(lock)                                           \
    LOG_D("Trying to Acquire Lock thread: %ld", pthread_self()); \
    pthread_mutex_lock(&lock);                                   \
    LOG_D("LOCK Acquired by thread: %ld", pthread_self());

#define UNLOCK_TXN(lock)                                             \
    LOG_D("Trying to Released Lock by thread: %ld", pthread_self()); \
    pthread_mutex_unlock(&lock);                                     \
    LOG_D("LOCK Released by thread: %ld", pthread_self());
#endif

sss_se05x_tunnel_context_t gs_tunnel_workaround;

sss_status_t ThreadSafetyWorkaround_Initialize()
{
	return sss_se05x_tunnel_context_init(&gs_tunnel_workaround, NULL);
}

void ThreadSafetyWorkaround_Deinitialize()
{
	sss_se05x_tunnel_context_free(&gs_tunnel_workaround);
}

void ThreadSafetyWorkaround_Lock()
{
	if(SE_IsInitialized()){
		LOCK_TXN(gs_tunnel_workaround.channelLock);
	}
}

void ThreadSafetyWorkaround_Unlock()
{
	if(SE_IsInitialized()){
		UNLOCK_TXN(gs_tunnel_workaround.channelLock);
	}
}

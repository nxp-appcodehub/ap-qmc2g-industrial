/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file task_awdg_connection_service.c
 * @brief Periodically tries to fetch tickets from the server and kick the authenticated watchdog.
 *
 */
#include "qmc_features_config.h"
#if FEATURE_SECURE_WATCHDOG
#include "FreeRTOS.h"
#include "event_groups.h"

#include "api_awdg_client.h"
#include "api_rpc.h"
#include "api_logging.h"

#include "fsl_debug_console.h"
#include "utils/debug_log_levels.h"
#define DEBUG_LOG_PRINT_FUNC PRINTF
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_SILENT
#endif
#include "utils/debug.h"
#include "utils/testing.h"
#include "main_cm7.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define WAIT_TICKS_SUCCESS pdMS_TO_TICKS(SECURE_WATCHDOG_KICK_INTERVAL_S * 1000U)
#define WAIT_TICKS_FAILURE pdMS_TO_TICKS(SECURE_WATCHDOG_KICK_RETRY_S * 1000U)
#define WAIT_TICKS_NO_LINK pdMS_TO_TICKS(2000U)

/*******************************************************************************
 * Variables
 ******************************************************************************/
TaskHandle_t g_awdg_connection_service_task_handle;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Helper function for logging errors related to the secure watchdog connection service.
 *
 * Logging is best effort, if it fails temporarily, we ignore this condition.
 * The watchdog connection service is retrying anyhow and if the error condition persists,
 * then logging will also be retried.
 */
static void LogAwdgConnectionServiceError(log_source_id_t source, log_event_code_t errorEventCode)
{
    log_record_t logEntry              = {0};
    logEntry.type                      = kLOG_SystemData;
    logEntry.data.systemData.source    = source;
    logEntry.data.systemData.category  = LOG_CAT_Fault;
    logEntry.data.systemData.eventCode = errorEventCode;
    /* logging is best effort, if it fails temporarily, we ignore it */
    (void) LOG_QueueLogEntry(&logEntry, false);
}

/*!
 * @brief Helper function for logging errors related to the RPC_RequestNonceFromSecureWatchdog() operation.
 */
static void LogAwdgConnectionServiceErrorRequestNonce(qmc_status_t err)
{
    switch(err)
    {
        case kStatus_QMC_Timeout:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestNonce, LOG_EVENT_Timeout);
            break;
        case kStatus_QMC_ErrSync:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestNonce, LOG_EVENT_SyncError);
            break;
        case kStatus_QMC_ErrArgInvalid:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestNonce, LOG_EVENT_InvalidArgument);
            break;
        case kStatus_QMC_ErrInternal:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestNonce, LOG_EVENT_InternalError);
            break;
        case kStatus_QMC_ErrNoBufs:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestNonce, LOG_EVENT_NoBufsError);
            break;
        /* other qmc_status_t errors should not apply, but still catch them */
        default:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestNonce, LOG_EVENT_RPCCallFailed);
            break;
    }
}

/*!
 * @brief Helper function for logging errors related to the AWDG_CLIENT_RequestTicket() operation.
 */
static void LogAwdgConnectionServiceErrorRequestTicket(awdg_client_status_code_t err)
{
    switch(err)
    { 
        case kStatus_AWDG_CLIENT_ErrArgInvalid:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestTicket, LOG_EVENT_InvalidArgument);
            break;
        case kStatus_AWDG_CLIENT_ErrConnection:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestTicket, LOG_EVENT_ConnectionError);
            break;
        case kStatus_AWDG_CLIENT_ErrRequest:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestTicket, LOG_EVENT_RequestError);
            break;
        case kStatus_AWDG_CLIENT_ErrJsonParsing:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestTicket, LOG_EVENT_JsonParsingError);
            break;
        /* currently no other error code exists, but added to be future proof */
        default:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceRequestTicket, LOG_EVENT_InternalError);
            break;
    }       
}

/*!
 * @brief Helper function for logging errors related to the RPC_KickSecureWatchdog() operation.
 */
static void LogAwdgConnectionServiceErrorKick(qmc_status_t err)
{
    switch(err)
    {
        case kStatus_QMC_Timeout:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_Timeout);
            break;
        case kStatus_QMC_ErrSync:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_SyncError);
            break;
        case kStatus_QMC_ErrArgInvalid:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_InvalidArgument);
            break;
        case kStatus_QMC_ErrInternal:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_InternalError);
            break;
        case kStatus_QMC_ErrSignatureInvalid:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_SignatureInvalid);
            break;
        case kStatus_QMC_ErrRange:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_RangeError);
            break;
        /* other qmc_status_t errors should not apply, but still catch them */
        default:
            LogAwdgConnectionServiceError(LOG_SRC_SecureWatchdogServiceKick, LOG_EVENT_RPCCallFailed);
            break;
    }    
}

/*!
 * @brief Interface task between the M4 secure watchdog implementation and the corresponding server.
 *
 * Periodically fetches a nonce from the secure watchdog, uses it to request a
 * new ticket from the server and kicks the secure watchdog using the received
 * ticket.
 *
 * In case of an error, the task logs the error and retries after a configurable wait
 * time. If the watchdog is not kicked for a longer time, it expires and the
 * system is reset. See "qmc2g_industrial_M4SLAVE/source/qmc_cm4_features_config.h"
 * for the initial secure watchdog configuration.
 *
 * See the @ref qmc_features_config.h header in the "Cyber Resilience" section
 * for the tasks configuration options.
 *
 * @startuml
 * start
 *  :Wait for the network to become ready (QMC_SYSEVENT_NETWORK_NoLink cleared);
 *  :uint8_t nonce[AWDG_CLIENT_TICKET_NONCE_SIZE] \n\
 * size_t nonceLen = 0 \n\
 * uint8_t ticket[AWDG_CLIENT_MAX_RAW_TICKET_SIZE] \n\
 * size_t ticketLen = 0;
 *  while(forever)
 *   if (RPC_RequestNonceFromSecureWatchdog(nonce, &nonceLen)) then(fail)
 *     :LogAwdgConnectionServiceErrorRequestNonce(error);
 *     :vTaskDelay(WAIT_TICKS_FAILURE);
 *   else if (AWDG_CLIENT_RequestTicket(SECURE_WATCHDOG_HOST, nonce, nonceLen, ticket, &ticketLen) then (fail)
 *     if (first failure with new error code) then (yes)
 *       :LogAwdgConnectionServiceErrorRequestTicket(error); 
 *     endif   
 *     :vTaskDelay(WAIT_TICKS_FAILURE);
 *   else if (RPC_KickSecureWatchdog(ticket, ticketLen)) then (fail)
 *     :LogAwdgConnectionServiceErrorKick(error);
 *     :vTaskDelay(WAIT_TICKS_FAILURE);
 *   else
 *   :vTaskDelay(WAIT_TICKS_SUCCESS);
 *   endif
 *  endwhile
 *  -[hidden]->
 *  detach
 * @enduml
 * 
 * @param[in] pvParameters FreeRTOS task parameter, unused for this task.
 */
void AwdgConnectionServiceTask(void *pvParameters)
{
    qmc_status_t cm4Ret                        = kStatus_QMC_Err;
    awdg_client_status_code_t remoteRequestRet = kStatus_AWDG_CLIENT_ErrRequest;
    uint8_t nonce[AWDG_CLIENT_TICKET_NONCE_SIZE];
    size_t nonceLen = 0U;
    uint8_t ticket[AWDG_CLIENT_MAX_RAW_TICKET_SIZE];
    size_t ticketLen = 0U;
    EventBits_t systemStatusEventBits = 0U;
    awdg_client_status_code_t lastRemoteRequestStatus = kStatus_AWDG_CLIENT_Ok;

    /* check if network is ready
     * FreeRTOS does not offer a function to wait for a event bit to become zero,
     * hence we manually check for this event
     */
    systemStatusEventBits = xEventGroupGetBits(g_systemStatusEventGroupHandle);
    while(systemStatusEventBits & QMC_SYSEVENT_NETWORK_NoLink)
    {
        DEBUG_LOG_W("[SecureWatchdog Task] Network link is not ready yet!\n");
        /* wait a bit and retry */
        vTaskDelay(WAIT_TICKS_NO_LINK);
        systemStatusEventBits = xEventGroupGetBits(g_systemStatusEventGroupHandle);
    }
    /* still sometimes the assert in SOCKETS_GetHostByName triggered, add additional wait */
    vTaskDelay(WAIT_TICKS_NO_LINK);

    /* repeat forever */
    while (FOREVER())
    {
        /* request nonce */
        nonceLen = AWDG_CLIENT_TICKET_NONCE_SIZE;
        cm4Ret   = RPC_RequestNonceFromSecureWatchdog(nonce, &nonceLen);
        if (kStatus_QMC_Ok != cm4Ret)
        {
            DEBUG_LOG_E("[SecureWatchdog Task] Failed (%d) to retrieve nonce from CM4!\n", cm4Ret);
            LogAwdgConnectionServiceErrorRequestNonce(cm4Ret);
            vTaskDelay(WAIT_TICKS_FAILURE);
            continue;
        }

        /* get ticket from remote server */
        ticketLen        = AWDG_CLIENT_MAX_RAW_TICKET_SIZE;
        remoteRequestRet = AWDG_CLIENT_RequestTicket(SECURE_WATCHDOG_HOST, nonce, nonceLen, ticket, &ticketLen);
        if (kStatus_AWDG_CLIENT_Ok != remoteRequestRet)
        {
            DEBUG_LOG_E("[SecureWatchdog Task] Failed (%d) to request ticket from remote server!\n", remoteRequestRet);
            /* only log first failure per error code
             * do not log sequential failures with the same error code to prevent saturating logging service */
            if(remoteRequestRet != lastRemoteRequestStatus)
            {
                LogAwdgConnectionServiceErrorRequestTicket(remoteRequestRet);
                lastRemoteRequestStatus = remoteRequestRet;
            }
            vTaskDelay(WAIT_TICKS_FAILURE);
            continue;
        }
        lastRemoteRequestStatus = kStatus_AWDG_CLIENT_Ok;

        /* kick secure watchdog */
        cm4Ret = RPC_KickSecureWatchdog(ticket, ticketLen);
        if (kStatus_QMC_Ok != cm4Ret)
        {
            DEBUG_LOG_E("[SecureWatchdog Task] Failed (%d) to kick watchdog on CM4!\n", cm4Ret);
            LogAwdgConnectionServiceErrorKick(cm4Ret);
            vTaskDelay(WAIT_TICKS_FAILURE);
            continue;
        }

        vTaskDelay(WAIT_TICKS_SUCCESS);
    }
}
#endif

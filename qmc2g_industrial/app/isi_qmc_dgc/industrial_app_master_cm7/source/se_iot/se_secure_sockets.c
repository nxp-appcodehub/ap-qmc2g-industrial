/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "se_secure_sockets.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "semphr.h"
#include "fsl_debug_console.h"

#include "iot_logging_task.h"
#include "iot_init.h"

#include "nxLog_App.h"

#define LOGGING_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)
#define LOGGING_TASK_STACK_SIZE (250)
#define LOGGING_QUEUE_LENGTH    (16)

static SemaphoreHandle_t gs_tlsClientConnectMutex;

static se_client_tls_ctx_t gs_se_client_tls_ctx;
se_client_tls_ctx_t* pse_client_tls_ctx = &gs_se_client_tls_ctx;

qmc_status_t SE_InitSecureSockets(void)
{
    BaseType_t xResult;

    xResult = IotSdk_Init();
    if (xResult != pdPASS) {
    	LOG_E("IotSdk_Init() failed");
        return kStatus_QMC_Err;
    }

    xResult = SOCKETS_Init();
    if (xResult != pdPASS) {
    	LOG_E("SOCKETS_Init failed");
        return kStatus_QMC_Err;
    }

    gs_tlsClientConnectMutex = xSemaphoreCreateMutex();
	if (gs_tlsClientConnectMutex == NULL) {
		LOG_E("xSemaphoreCreateMutex failed");
		return kStatus_QMC_Err;
	}

	xLoggingTaskInitialize(LOGGING_TASK_STACK_SIZE, LOGGING_TASK_PRIORITY, LOGGING_QUEUE_LENGTH);

	return kStatus_QMC_Ok;
}

TransportSocketStatus_t SE_SecureSocketsTransport_Connect( NetworkContext_t * pNetworkContext,
                                                        const ServerInfo_t * pServerInfo,
                                                        const SocketsConfig_t * pSocketsConfig,
														const se_client_tls_ctx_t * pSeClientTlsContext)
{
	/* We lock because the underlying lib will use the global structure
	 * to access the key ids and associate them to the context.
	 */
	if (xSemaphoreTake(gs_tlsClientConnectMutex, portMAX_DELAY) != pdTRUE) {
		LOG_E("Acquiring TLS semaphore failed");
	}

	pse_client_tls_ctx->server_root_cert_index = pSeClientTlsContext->server_root_cert_index;
	pse_client_tls_ctx->client_keyPair_index = pSeClientTlsContext->client_keyPair_index;
	pse_client_tls_ctx->client_cert_index = pSeClientTlsContext->client_cert_index;

	TransportSocketStatus_t ret = SecureSocketsTransport_Connect(pNetworkContext, pServerInfo, pSocketsConfig);

	if (xSemaphoreGive(gs_tlsClientConnectMutex) != pdTRUE) {
		LOG_E("Releasing TLS semaphore failed");
	}

	return ret;
}


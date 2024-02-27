/*
 * Copyright 2024 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CLOUD_SERVICE_CLOUD_CONN_COMMON_H_
#define CLOUD_SERVICE_CLOUD_CONN_COMMON_H_

#include "cloud_service.h"
#include "se_secure_sockets.h"

/* defines the parameters for a generic MQTT SE secured connection */
typedef struct cloud_conn_config
{
	char *pHostName;
	uint16_t port;
	char* pUserName;
	char* pPassword;
	char* pDeviceName;
	se_client_tls_ctx_t se_client_tls_ctx;
} cloud_conn_config_t;

BaseType_t cloud_conn_establish(const cloud_conn_config_t* pSeMqttConfig);
BaseType_t cloud_conn_publish(const char * pcTopicFilter, const char * pcPayload);
BaseType_t cloud_conn_status(void);
BaseType_t cloud_conn_disconnect(void);

#endif /* CLOUD_SERVICE_CLOUD_CONN_COMMON_H_ */

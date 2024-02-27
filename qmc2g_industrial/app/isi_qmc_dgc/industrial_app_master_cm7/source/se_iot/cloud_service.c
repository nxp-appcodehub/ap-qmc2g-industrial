/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdio.h>

#include "app.h"
#include "cloud_service.h"
#include "cloud_conn_common.h"
#include "se_session.h"
#include "api_configuration.h"
#include "api_board.h"
#include "api_motorcontrol.h"
#include "api_fault.h"
#include "api_logging.h"
#include "api_rpc.h"
#include "qmc_features_config.h"

/*
 * CLOUD SERVICE
 *
 * This service wraps around "cloud_conn_common", which is itself
 * an Amazon FreeRTOS sample code that wraps around "coreMQTT".
 * The purpose of this service is to send QMC2G device status
 * information through MQTT. The different values are to be
 * represented as MQTT topics. The "cloud_conn_common" publishes
 * strings, which means the various values will be stringified.
 * To be able to use the device's private key on the SE051
 * secure element, "cloud_conn_common" has been modified to
 * allow passing SE051 key IDs.
 *
 * (Note: cloud_conn_common.c was pulled from SDK 2.11, it had demo
 * specific code and compile time configuration for e.g. host
 * name, port and server ca certificate.
 * cloud_conn_common.h was created to provide a unifying API
 * and configuration that is passed at runtime, cloud_conn_common.c
 * was then modified to pass that configuration to the various
 * functions and replace the demo specific logic with the more
 * generic functions exposed in the API.)
 *
 * The service has two possible modes, Azure and Generic ,only
 * one can be selected at a time in the feature flags.
 * In generic mode, the service can connect to a generic MQTT
 * broker and publish the data as topics. In Azure mode, the
 * service connects specifically to Azure IoTHub. Since the
 * IoTHub does not act as an actual MQTT broker, it only allows
 * one MQTT topic, so in Azure mode, the topic is actually
 * represented as a property appended to this one topic.
 *
 * Example generic topic: QMC_<ID>/system/FW_version
 * Example Azure pseudo topic: devices/<DEVICE_NAME>/messages/events/topic=QMC_<ID>-system-FW_version
 *
 * On startup, the service configuration is fetched to prepare
 * for connection. The fixed prefix for the various topics is
 * also constructed e.g. QMC_<ID>. Once connected, the various
 * values are published according to different criteria (see
 * further below) to the correct topic.
 *
 * The state of the service configuration and other values are
 * stored in global static memory. The service is not designed
 * to be started multiple times and if anything goes wrong,
 * the service ends with no mechanism to restart.
 *
 */


#if defined(FEATURE_CLOUD_AZURE_IOTHUB) && FEATURE_CLOUD_AZURE_IOTHUB
#	define USE_CLOUD
#	define USE_AZURE
#elif defined(FEATURE_CLOUD_GENERIC_MQTT) && FEATURE_CLOUD_GENERIC_MQTT
#	define USE_CLOUD
#	define USE_GENERIC
#endif

#ifdef USE_CLOUD

extern EventGroupHandle_t g_systemStatusEventGroupHandle;
TaskHandle_t g_cloud_service_task_handle;

#define LIFECYCLE_COMMISSIONING_DISPLAY   "Commissioning"
#define LIFECYCLE_OPERATIONAL_DISPLAY     "Operational"
#define LIFECYCLE_ERROR_DISPLAY           "Error"
#define LIFECYCLE_MAINTENANCE_DISPLAY     "Maintenance"
#define LIFECYCLE_DECOMMISSIONING_DISPLAY "Decommissioning"
#define LIFECYCLE_DISPLAY_LONGEST         LIFECYCLE_DECOMMISSIONING_DISPLAY

/* Since Azure IoTHub cannot be a true MQTT broker and because it
 * only supports one actual topic, topics are simulated by publishing
 * them as an appended property named "topic". Since having a forward
 * slash is not allowed as-is in a property value, a "-" is substituted
 * in the case of Azure IoTHub */
#ifdef USE_AZURE
#	define FSLASH "-"
#else
#	define FSLASH "/"
#endif

/* unique section of topics, prefix e.g. QMC_<ID> to be prepended at runtime */
#define MQTT_TOPIC_FW_VERSION                     FSLASH "system"  FSLASH "FW_version"
#define MQTT_TOPIC_RESTART_REQUIRED_CONFIG_BACKUP FSLASH "system"  FSLASH "restart_required_configuration_backup"
#define MQTT_TOPIC_RESTART_REQUIRED_FW_UPDATE     FSLASH "system"  FSLASH "restart_required_fw_update_commit"
#define MQTT_TOPIC_AD_STATUS                      FSLASH "system"  FSLASH "AD_status"
#define MQTT_TOPIC_SD_CARD_AVAILABLE              FSLASH "system"  FSLASH "SD_card_available"
#define MQTT_TOPIC_SYSTEM_FAULT_STATUS            FSLASH "system"  FSLASH "system_fault_status"
#define MQTT_TOPIC_LIFE_CYCLE_STATE               FSLASH "system"  FSLASH "life_cycle_state"
#define MQTT_TOPIC_MOTOR1_FAULT_STATUS            FSLASH "motor_1" FSLASH "fault_status"
#define MQTT_TOPIC_MOTOR1_SPEED                   FSLASH "motor_1" FSLASH "speed"
#define MQTT_TOPIC_MOTOR1_POSITION                FSLASH "motor_1" FSLASH "position"
#define MQTT_TOPIC_MOTOR2_FAULT_STATUS            FSLASH "motor_2" FSLASH "fault_status"
#define MQTT_TOPIC_MOTOR2_SPEED                   FSLASH "motor_2" FSLASH "speed"
#define MQTT_TOPIC_MOTOR2_POSITION                FSLASH "motor_2" FSLASH "position"
#define MQTT_TOPIC_MOTOR3_FAULT_STATUS            FSLASH "motor_3" FSLASH "fault_status"
#define MQTT_TOPIC_MOTOR3_SPEED                   FSLASH "motor_3" FSLASH "speed"
#define MQTT_TOPIC_MOTOR3_POSITION                FSLASH "motor_3" FSLASH "position"
#define MQTT_TOPIC_MOTOR4_FAULT_STATUS            FSLASH "motor_4" FSLASH "fault_status"
#define MQTT_TOPIC_MOTOR4_SPEED                   FSLASH "motor_4" FSLASH "speed"
#define MQTT_TOPIC_MOTOR4_POSITION                FSLASH "motor_4" FSLASH "position"
#define MQTT_TOPIC_LOG_LATEST_RECORD              FSLASH "log"     FSLASH "latest_record"
#define MQTT_TOPIC_LOG_MESSAGE_LOST               FSLASH "log"     FSLASH "message_lost"
#define MQTT_TOPIC_LOG_MEMORY_LOW                 FSLASH "log"     FSLASH "memory_low"
#define MQTT_TOPIC_LOG_FLASH_ERROR                FSLASH "log"     FSLASH "flash_error"
#define MQTT_TOPIC_LONGEST                        MQTT_TOPIC_RESTART_REQUIRED_CONFIG_BACKUP

static const char* gs_mqttUniqueTopics[] = {
	MQTT_TOPIC_FW_VERSION,
	MQTT_TOPIC_RESTART_REQUIRED_CONFIG_BACKUP,
	MQTT_TOPIC_RESTART_REQUIRED_FW_UPDATE,
	MQTT_TOPIC_AD_STATUS,
	MQTT_TOPIC_SD_CARD_AVAILABLE,
	MQTT_TOPIC_SYSTEM_FAULT_STATUS,
	MQTT_TOPIC_LIFE_CYCLE_STATE,
	MQTT_TOPIC_MOTOR1_FAULT_STATUS,
	MQTT_TOPIC_MOTOR1_SPEED,
	MQTT_TOPIC_MOTOR1_POSITION,
	MQTT_TOPIC_MOTOR2_FAULT_STATUS,
	MQTT_TOPIC_MOTOR2_SPEED,
	MQTT_TOPIC_MOTOR2_POSITION,
	MQTT_TOPIC_MOTOR3_FAULT_STATUS,
	MQTT_TOPIC_MOTOR3_SPEED,
	MQTT_TOPIC_MOTOR3_POSITION,
	MQTT_TOPIC_MOTOR4_FAULT_STATUS,
	MQTT_TOPIC_MOTOR4_SPEED,
	MQTT_TOPIC_MOTOR4_POSITION,
	MQTT_TOPIC_LOG_LATEST_RECORD,
	MQTT_TOPIC_LOG_MESSAGE_LOST,
	MQTT_TOPIC_LOG_MEMORY_LOW,
	MQTT_TOPIC_LOG_FLASH_ERROR,
};

typedef enum _mqtt_topic_id
{
	kCLOUD_Topic_system_fw_version = 0,
	kCLOUD_Topic_system_restart_required_configuration_backup,
	kCLOUD_Topic_system_restart_required_fw_update_commit,
	kCLOUD_Topic_system_ad_status,
	kCLOUD_Topic_system_sd_card_available,
	kCLOUD_Topic_system_system_fault_status,
	kCLOUD_Topic_system_life_cycle_state,
	kCLOUD_Topic_motor_1_fault_status,
	kCLOUD_Topic_motor_1_speed,
	kCLOUD_Topic_motor_1_position,
	kCLOUD_Topic_motor_2_fault_status,
	kCLOUD_Topic_motor_2_speed,
	kCLOUD_Topic_motor_2_position,
	kCLOUD_Topic_motor_3_fault_status,
	kCLOUD_Topic_motor_3_speed,
	kCLOUD_Topic_motor_3_position,
	kCLOUD_Topic_motor_4_fault_status,
	kCLOUD_Topic_motor_4_speed,
	kCLOUD_Topic_motor_4_position,
	kCLOUD_Topic_log_latest_record,
	kCLOUD_Topic_log_message_lost,
	kCLOUD_Topic_log_memory_low,
	kCLOUD_Topic_log_flash_error,
	kCLOUD_Topic_num,
} mqtt_topic_id_t;

#define TOPICS_PER_MOTOR (3)

/* topic max length with ID prefix */
#define DEV_ID_PREFIX "QMC_"
#define DEV_ID_LEN (32 * 2)    /* SHA2-256 in hex notation*/
#define MQTT_UNIQUE_TOPIC_MAX_LEN (sizeof(MQTT_TOPIC_LONGEST) - 1)

/* azure specific string and lengths */
#ifdef USE_AZURE
#define AZURE_HOST_URL ".azure-devices.net"
#define AZURE_HOST_URL_SLASH AZURE_HOST_URL "/"
#define AZURE_API_VERSION "/?api-version=2021-04-12"
#define HOST_NAME_MAX_LEN (CONFIG_MAX_VALUE_LEN - 1 + \
	                      sizeof(AZURE_HOST_URL) - 1 + \
						  1)
#define DEVICE_NAME_MAX_LEN (DEV_ID_LEN + 1)
#define USER_NAME_MAX_LEN (CONFIG_MAX_VALUE_LEN -1 + \
	                      sizeof(AZURE_HOST_URL_SLASH) - 1 + \
						  DEV_ID_LEN + \
						  sizeof(AZURE_API_VERSION) - 1 + \
						  1)
#define PASSWORD_MAX_LEN (1) /* in azure IoTHub's case empty string */

#define AZURE_TOPIC_PREFIX1 "devices/"
#define AZURE_TOPIC_PREFIX2 "/messages/events/topic="
/* prefix1 + max azure device name size + prefix2 + QMC_<ID> + null termination */
#define MQTT_TOPIC_PREFIX_MAX_LEN (sizeof(AZURE_TOPIC_PREFIX1) - 1 + \
							       DEV_ID_LEN + \
							       sizeof(AZURE_TOPIC_PREFIX2) - 1 + \
							       sizeof(DEV_ID_PREFIX) - 1 + \
								   DEV_ID_LEN + \
								   1)

/* not azure specific */
#else
#define HOST_NAME_MAX_LEN   (CONFIG_MAX_VALUE_LEN)
#define DEVICE_NAME_MAX_LEN (CONFIG_MAX_VALUE_LEN)
#define USER_NAME_MAX_LEN   (CONFIG_MAX_VALUE_LEN)
#define PASSWORD_MAX_LEN    (CONFIG_MAX_VALUE_LEN)
/* QMC_<ID> + null termination */
#define MQTT_TOPIC_PREFIX_MAX_LEN  (sizeof(DEV_ID_PREFIX) - 1 + \
		                           DEV_ID_LEN + \
								   1)
#endif

/* the prefix length already accounts for the null terminator */
#define MQTT_TOPIC_MAX_LEN (MQTT_TOPIC_PREFIX_MAX_LEN + \
							MQTT_UNIQUE_TOPIC_MAX_LEN)

/* connection configuration */
static char gs_hostName[HOST_NAME_MAX_LEN];
static char gs_deviceName[DEVICE_NAME_MAX_LEN];
static char gs_userName[USER_NAME_MAX_LEN];
static char gs_password[PASSWORD_MAX_LEN];
static char gs_pubTopicPrefix[MQTT_TOPIC_PREFIX_MAX_LEN];
static char gs_pubTopic[MQTT_TOPIC_MAX_LEN];
static cloud_conn_config_t gs_cloudConnConfig;

/* AZURE IOTHUB specific code starts here */
#if defined(USE_AZURE)

/* sets the cloud conn config for Azure IoTHub service  */
static qmc_status_t AZURE_SetGlobalCloudConnConfig()
{
	qmc_status_t result;
	/* grab device id */
	const char* device_id = BOARD_GetDeviceIdentifier();
	if(NULL == device_id) {
		return kStatus_QMC_Err;
	}
	/* grab the needed information from configuration */
	unsigned char azureHubName[CONFIG_MAX_VALUE_LEN] = {0};
	result = CONFIG_GetStrValueById(kCONFIG_Key_CloudAzureHubName, azureHubName);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	if(strnlen((const char*)azureHubName, CONFIG_MAX_VALUE_LEN) == CONFIG_MAX_VALUE_LEN) {
		/* not a string */
		return kStatus_QMC_Err;
	}

	/* create the needed strings */
	gs_hostName[0] = '\0';
	strcat(gs_hostName, (const char*)azureHubName);
	strcat(gs_hostName, AZURE_HOST_URL);

	gs_deviceName[0] = '\0';
	strcat(gs_deviceName, device_id); //azure device name

	gs_userName[0] = '\0';
	strcat(gs_userName, (const char*)azureHubName);
	strcat(gs_userName, AZURE_HOST_URL_SLASH);
	strcat(gs_userName, device_id); //azure device name
	strcat(gs_userName, AZURE_API_VERSION);

	gs_password[0] = '\0';

	/* prepare cloud config struct */
	gs_cloudConnConfig.pHostName = gs_hostName;
	gs_cloudConnConfig.port = 8883;
	gs_cloudConnConfig.pUserName = gs_userName;
	gs_cloudConnConfig.pPassword = gs_password;
	gs_cloudConnConfig.pDeviceName = gs_deviceName;
	gs_cloudConnConfig.se_client_tls_ctx.client_cert_index = idCloud1DevCert;
	gs_cloudConnConfig.se_client_tls_ctx.client_keyPair_index= idCloud1DevKeyPair;
	gs_cloudConnConfig.se_client_tls_ctx.server_root_cert_index= idCloud1ServerCaCert;

	return kStatus_QMC_Ok;
}

/* Sets the topic prefix for Azure IoTHub service */
static qmc_status_t AZURE_SetGlobalTopicPrefix(const char* device_id)
{
	gs_pubTopicPrefix[0] = '\0';
	strcat(gs_pubTopicPrefix, AZURE_TOPIC_PREFIX1);
	strcat(gs_pubTopicPrefix, device_id); //azure device name
	strcat(gs_pubTopicPrefix, AZURE_TOPIC_PREFIX2);
	strcat(gs_pubTopicPrefix, "QMC_");
	strcat(gs_pubTopicPrefix, device_id);

	return kStatus_QMC_Ok;
}

#elif defined(USE_GENERIC)

/* sets the cloud conn config for generic MQTT broker  */
static qmc_status_t GENERIC_SetGlobalCloudConnConfig()
{
	qmc_status_t result;
	/* grab the needed information from configuration */
	result = CONFIG_GetStrValueById(kCONFIG_Key_CloudGenericHostName, (unsigned char*)gs_hostName);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	if(strnlen(gs_hostName, CONFIG_MAX_VALUE_LEN) == CONFIG_MAX_VALUE_LEN) {
		/* not a string */
		return kStatus_QMC_Err;
	}
	result = CONFIG_GetStrValueById(kCONFIG_Key_CloudGenericUserName, (unsigned char*)gs_userName);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	if(strnlen(gs_userName, CONFIG_MAX_VALUE_LEN) == CONFIG_MAX_VALUE_LEN) {
		/* not a string */
		return kStatus_QMC_Err;
	}
	result = CONFIG_GetStrValueById(kCONFIG_Key_CloudGenericPassword, (unsigned char*)gs_password);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	if(strnlen(gs_password, CONFIG_MAX_VALUE_LEN) == CONFIG_MAX_VALUE_LEN) {
		/* not a string */
		return kStatus_QMC_Err;
	}
	result = CONFIG_GetStrValueById(kCONFIG_Key_CloudGenericDeviceName, (unsigned char*)gs_deviceName);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	if(strnlen(gs_deviceName, CONFIG_MAX_VALUE_LEN) == CONFIG_MAX_VALUE_LEN) {
		/* not a string */
		return kStatus_QMC_Err;
	}
	unsigned char portArray[CONFIG_MAX_VALUE_LEN];
	result = CONFIG_GetBinValueById(kCONFIG_Key_CloudGenericPort, portArray, 2);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	uint16_t port = (portArray[0] << 8) | portArray[1];

	/* prepare cloud config struct */
	gs_cloudConnConfig.pHostName = gs_hostName;
	gs_cloudConnConfig.port = port;
	gs_cloudConnConfig.pUserName = gs_userName;
	gs_cloudConnConfig.pPassword = gs_password;
	gs_cloudConnConfig.pDeviceName = gs_deviceName;
	gs_cloudConnConfig.se_client_tls_ctx.client_cert_index = idCloud1DevCert;
	gs_cloudConnConfig.se_client_tls_ctx.client_keyPair_index= idCloud1DevKeyPair;
	gs_cloudConnConfig.se_client_tls_ctx.server_root_cert_index= idCloud1ServerCaCert;

	return kStatus_QMC_Ok;
}

/* sets the topic prefix for generic MQTT broker  */
static qmc_status_t GENERIC_SetGlobalTopicPrefix(const char* device_id)
{
	gs_pubTopicPrefix[0] = '\0';
	strcat(gs_pubTopicPrefix, "QMC_");
	strcat(gs_pubTopicPrefix, device_id);

	return kStatus_QMC_Ok;
}

#endif /* USE_AZURE || USE_GENERIC */

static qmc_status_t CLOUD_SetGlobalCloudConnConfig()
{
	( void ) memset( ( void * ) &gs_cloudConnConfig, 0x00, sizeof( gs_cloudConnConfig ) );
#ifdef USE_AZURE
	return AZURE_SetGlobalCloudConnConfig();
#else
	return GENERIC_SetGlobalCloudConnConfig();
#endif /* USE_AZURE || USE_GENERIC */
}

static qmc_status_t CLOUD_SetGlobalTopicPrefix()
{
	const char* device_id = BOARD_GetDeviceIdentifier();
	if(NULL == device_id) {
		return kStatus_QMC_Err;
	}
#ifdef USE_AZURE
	return AZURE_SetGlobalTopicPrefix(device_id);
#else
	return GENERIC_SetGlobalTopicPrefix(device_id);
#endif /* USE_AZURE || USE_GENERIC */
}

static qmc_status_t CLOUD_PublishStrToTopic(mqtt_topic_id_t topic_id, const char* payload)
{
	if(topic_id < 0 || topic_id >= kCLOUD_Topic_num || payload == NULL) {
		return kStatus_QMC_ErrArgInvalid;
	}

	gs_pubTopic[0] = '\0';
	strcat(gs_pubTopic, gs_pubTopicPrefix);
	strcat(gs_pubTopic, gs_mqttUniqueTopics[topic_id]);

	BaseType_t xResult;
	xResult = cloud_conn_publish(gs_pubTopic, payload);
	if (xResult != pdPASS) {
		return kStatus_QMC_Err;
	}
	return kStatus_QMC_Ok;
}

static qmc_status_t CLOUD_PublishEventBit(mqtt_topic_id_t topic_id, uint32_t event_bits, int32_t bit_mask)
{
	return CLOUD_PublishStrToTopic(topic_id,
			(event_bits & bit_mask) ? "true" : "false");
}

/* serializes and publishes the encrypted record */
static qmc_status_t CLOUD_PublishLogRecordEntry(const log_encrypted_record_t *pEntry)
{
	qmc_status_t result;

	const size_t entryHexSize = sizeof(log_encrypted_record_t) * 2;
	char entryStr[entryHexSize + 1];
	const char* hexChars = "0123456789ABCDEF";
	char* pEntryStr = entryStr;
	uint8_t* pEncBytes = (uint8_t*)pEntry;
	for(int i = 0; i < sizeof(log_encrypted_record_t); i++) {
		*pEntryStr = hexChars[(pEncBytes[i] >> 4) & 0x0Fu]; pEntryStr++;
		*pEntryStr = hexChars[(pEncBytes[i]     ) & 0x0Fu]; pEntryStr++;
	}
	*pEntryStr = '\0';
	result= CLOUD_PublishStrToTopic(kCLOUD_Topic_log_latest_record, entryStr);
	if(result != kStatus_QMC_Ok) {
		return result;
	}
	return kStatus_QMC_Ok;
}

/*
 * Publishes topic where changes can be signaled by
 * event bits. Publishes only the ones that changed
 * unless nothing changed, then all are published.
 */
static qmc_status_t CLOUD_PublishEventDriven(uint32_t systemStatus, uint32_t systemStatusChanged)
{
	qmc_status_t result;

	if(0 == systemStatusChanged) {
		/* if nothing changed then it is a timeout and we re-send everything */
		systemStatusChanged = ~systemStatusChanged;
	}

	/* life cycle */
	if((systemStatusChanged & QMC_SYSEVENT_LIFECYCLE_Commissioning) |
	   (systemStatusChanged & QMC_SYSEVENT_LIFECYCLE_Operational) |
	   (systemStatusChanged & QMC_SYSEVENT_LIFECYCLE_Error) |
	   (systemStatusChanged & QMC_SYSEVENT_LIFECYCLE_Maintenance) |
	   (systemStatusChanged & QMC_SYSEVENT_LIFECYCLE_Decommissioning))
	{
		char lifeCycleStr[sizeof(LIFECYCLE_DISPLAY_LONGEST)] = {0};
		if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Operational) {
			strcpy(lifeCycleStr, LIFECYCLE_OPERATIONAL_DISPLAY);
		}
		else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Maintenance) {
			strcpy(lifeCycleStr, LIFECYCLE_MAINTENANCE_DISPLAY);
		}
		else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Commissioning) {
			strcpy(lifeCycleStr, LIFECYCLE_COMMISSIONING_DISPLAY);
		}
		else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Decommissioning) {
			strcpy(lifeCycleStr, LIFECYCLE_DECOMMISSIONING_DISPLAY);
		}
		else {
			strcpy(lifeCycleStr, LIFECYCLE_ERROR_DISPLAY);
		}
		result = CLOUD_PublishStrToTopic(kCLOUD_Topic_system_life_cycle_state, lifeCycleStr);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}

	/* sd card available */
	/* restart: config changed / fw update */
	/* flash error / message lost / memory low */
	if(systemStatusChanged & QMC_SYSEVENT_MEMORY_SdCardAvailable) {
		result = CLOUD_PublishEventBit(kCLOUD_Topic_system_sd_card_available, systemStatus, QMC_SYSEVENT_MEMORY_SdCardAvailable);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}
	if(systemStatusChanged & QMC_SYSEVENT_CONFIGURATION_ConfigChanged) {
		result = CLOUD_PublishEventBit(kCLOUD_Topic_system_restart_required_configuration_backup, systemStatus, QMC_SYSEVENT_CONFIGURATION_ConfigChanged);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}
	if(systemStatusChanged & QMC_SYSEVENT_FWUPDATE_RestartRequired) {
		result = CLOUD_PublishEventBit(kCLOUD_Topic_system_restart_required_fw_update_commit, systemStatus, QMC_SYSEVENT_FWUPDATE_RestartRequired);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}
	if(systemStatusChanged & QMC_SYSEVENT_LOG_FlashError) {
		result = CLOUD_PublishEventBit(kCLOUD_Topic_log_flash_error, systemStatus, QMC_SYSEVENT_LOG_FlashError);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}
	if(systemStatusChanged & QMC_SYSEVENT_LOG_MessageLost) {
		result = CLOUD_PublishEventBit(kCLOUD_Topic_log_message_lost, systemStatus, QMC_SYSEVENT_LOG_MessageLost);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}
	if(systemStatusChanged & QMC_SYSEVENT_LOG_LowMemory) {
		result = CLOUD_PublishEventBit(kCLOUD_Topic_log_memory_low, systemStatus, QMC_SYSEVENT_LOG_LowMemory);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}

	/* system fault */
	if(systemStatusChanged & QMC_SYSEVENT_FAULT_System) {
		fault_system_fault_t sysFault = FAULT_GetSystemFault_fromISR();
		const size_t sysFaultHexSize = sizeof(fault_system_fault_t) * 2;
		char sysFaultStr[sysFaultHexSize + 1];
		if(snprintf(sysFaultStr, sizeof(sysFaultStr), "%0*X", sysFaultHexSize, (int)sysFault) < 0) {
			return kStatus_QMC_ErrNoBufs;
		}
		result = CLOUD_PublishStrToTopic(kCLOUD_Topic_system_system_fault_status, sysFaultStr);
		if(result != kStatus_QMC_Ok) {
			return result;
		}
	}

	return kStatus_QMC_Ok;
}

static qmc_status_t CLOUD_PublishMotorStatus(const mc_motor_status_t *pMotorStatus)
{
	if(pMotorStatus == NULL || pMotorStatus->eMotorId > MC_MAX_MOTORS) {
		return kStatus_QMC_ErrArgInvalid;
	}

	mqtt_topic_id_t speedTopic = kCLOUD_Topic_motor_1_speed + pMotorStatus->eMotorId * TOPICS_PER_MOTOR;
	mqtt_topic_id_t positionTopic = kCLOUD_Topic_motor_1_position + pMotorStatus->eMotorId * TOPICS_PER_MOTOR;
	mqtt_topic_id_t faultTopic = kCLOUD_Topic_motor_1_fault_status + pMotorStatus->eMotorId * TOPICS_PER_MOTOR;

	float speed = pMotorStatus->sSlow.fltSpeed; /* -4000 +4000 */
	int16_t turns = pMotorStatus->sSlow.uPosition.sPosValue.i16NumOfTurns;
	uint16_t rotorPosition = pMotorStatus->sSlow.uPosition.sPosValue.ui16RotorPosition;
	float position = turns + (rotorPosition * 1.0f / USHRT_MAX);
	mc_fault_t fault = pMotorStatus->sFast.eFaultStatus;

	/* stringify */
	const size_t faultHexSize = sizeof(mc_fault_t) * 2;
	char faultStr[faultHexSize + 1];
	char speedStr[10]; /* -4000.00 to +4000.00*/
	char positionStr[10]; /* -100.00 to + 100.00 */

	if(snprintf(speedStr, sizeof(speedStr), "%.2f", speed) < 0) {
		return kStatus_QMC_ErrNoBufs;
	}
	if(snprintf(positionStr, sizeof(positionStr), "%.2f", position) < 0) {
		return kStatus_QMC_ErrNoBufs;
	}
	if(snprintf(faultStr, sizeof(faultStr), "%0*X", faultHexSize, (int)fault) < 0) {
		return kStatus_QMC_ErrNoBufs;
	}

	/* publish */
	if(CLOUD_PublishStrToTopic(faultTopic, faultStr) ||
	   CLOUD_PublishStrToTopic(speedTopic, speedStr) ||
	   CLOUD_PublishStrToTopic(positionTopic, positionStr)) {
		return kStatus_QMC_Err;
	}

	return kStatus_QMC_Ok;
}

/* if elapsed, the lastTick value is updated */
static bool isIntervalElapsed(TickType_t *pLastTick, int intervalMs)
{
	TickType_t currentTick = xTaskGetTickCount();
	if(currentTick < *pLastTick) { /*roll over*/
		*pLastTick = currentTick;
		return true;
	}
	TickType_t tickDiff = currentTick - *pLastTick;
	TickType_t intervalTick = pdMS_TO_TICKS(intervalMs);
	if(tickDiff >= intervalTick) {
		*pLastTick = currentTick;
		return true;
	}

	return false;
}

#define MQTT_PUBLISH_MOTOR_STATUS_PER_MINUTE (40)
#define MQTT_FAST_PUBLISH_INTERVAL_MS (100)  /* e.g. for waiting on log */
#define MQTT_SLOW_PUBLISH_INTERVAL_MS (5000) /* e.g. for system status (earlier if changed) */

#define MOTOR_QUEUE_DECIMATION (60000 / (DATAHUB_STATUS_SAMPLING_INTERVAL_MS * MQTT_PUBLISH_MOTOR_STATUS_PER_MINUTE))

/* entry point */
void CloudServiceTask(void *pvParameters)
{
	if(CLOUD_IsServiceRunning()) {
		dbgCloudPRINTF("The cloud service is already started!\n");
		goto exit;
	}

	/* Wait for the link to be up */
	while(xEventGroupGetBits(g_systemStatusEventGroupHandle) & QMC_SYSEVENT_NETWORK_NoLink)
	{
		vTaskDelay(pdMS_TO_TICKS(250));
	}
	/* Wait for the interface to be available */
	vTaskDelay(pdMS_TO_TICKS(INTERFACE_SETUP_DELAY_MS));

	qmc_status_t result;
	/* setup configuration and topics */
	result = CLOUD_SetGlobalCloudConnConfig();
	if(result != kStatus_QMC_Ok) {
		dbgCloudPRINTF("Could not setup cloud connection configuration!\n");
		goto exit;
	}
	result = CLOUD_SetGlobalTopicPrefix();
	if(result != kStatus_QMC_Ok) {
		dbgCloudPRINTF("Could not setup cloud connection topic prefix!\n");
		goto exit;
	}

	/* connect to cloud */
	BaseType_t xResult = cloud_conn_establish(&gs_cloudConnConfig);
	if (xResult != pdPASS) {
		dbgCloudPRINTF("Could not connect to the cloud!\n");
		goto exit;
	}
	dbgCloudPRINTF("Successfully connected to the cloud!\n");

	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogCloudService) != kStatus_QMC_Ok)
	{
		FAULT_RaiseFaultEvent(kFAULT_FunctionalWatchdogInitFail);
	}

	/* setup */
	uint32_t systemStatusOld = 0;
	TickType_t fwVersionLastPubTime = 0;
	TickType_t systemStatusLastPubTime = 0;

	qmc_msg_queue_handle_t *mcHandle = NULL;
	mc_motor_status_t motorStatus = {0};
	result = MC_GetNewStatusQueueHandle(&mcHandle, MOTOR_QUEUE_DECIMATION);
	if(result != kStatus_QMC_Ok) {
		dbgCloudPRINTF("Could not get motor status queue handle! Disconnecting...\n");
		goto exit_connected;
	}

	qmc_msg_queue_handle_t *logHandle = NULL;
	log_encrypted_record_t  encryptedRecordEntry = {0};
	result = LOG_GetNewLoggingQueueHandle(&logHandle);
	if(result != kStatus_QMC_Ok) {
		dbgCloudPRINTF("Could not get log record queue handle! Disconnecting...\n");
		goto exit_connected;
	}

	/* Publish Strategy:
	 * FW Version: Every slow interval (5 sec).
	 * System status: Whenever event bits change or every slow interval (5 sec).
	 * Motor status: Whenever item is present in queue.
	 * Log: Blocks there for at most fast interval (100 ms) because we don't know when to expect these.
	 *      Bulk of waiting happens here moderating the rate for the other values
	 */
	while(true)
	{
		if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogCloudService) != kStatus_QMC_Ok)
		{
			log_record_t logEntryWithoutId = {
					.rhead = {
							.chksum				= 0,
							.uuid				= 0,
							.ts = {
								.seconds		= 0,
								.milliseconds	= 0
							}
					},
					.type = kLOG_SystemData,
					.data.systemData.source = LOG_SRC_CloudService,
					.data.systemData.category = LOG_CAT_General,
					.data.systemData.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed
			};

			LOG_QueueLogEntry(&logEntryWithoutId, false);
		}

		/* FW Version*/
		if(isIntervalElapsed(&fwVersionLastPubTime, MQTT_SLOW_PUBLISH_INTERVAL_MS)) {
			qmc_fw_version_t fwVersion = BOARD_GetFwVersion();
			char fwVersionStr[15];
			if(snprintf(fwVersionStr, sizeof(fwVersionStr), "%d.%d.%d", fwVersion.major, fwVersion.minor, fwVersion.bugfix) < 0) {
				dbgCloudPRINTF("Not enough buffer to store FW version! Disconnecting...");
				goto exit_connected;
			}
			result = CLOUD_PublishStrToTopic(kCLOUD_Topic_system_fw_version, fwVersionStr);
			if(result != kStatus_QMC_Ok) {
				dbgCloudPRINTF("Could not publish FW version to the cloud! Disconnecting...\n");
				goto exit_connected;
			}
		}

		/* log latest */
		result = LOG_DequeueEncryptedLogEntry(logHandle, MQTT_FAST_PUBLISH_INTERVAL_MS, &encryptedRecordEntry);
		if((result != kStatus_QMC_ErrNoMsg)&&(result != kStatus_QMC_Timeout)&&(result != kStatus_QMC_Ok))
		{
			dbgCloudPRINTF("Could not get log record queue contents! Disconnecting...");
			goto exit_connected;
		} else if(result == kStatus_QMC_Ok) {
			result = CLOUD_PublishLogRecordEntry(&encryptedRecordEntry);
			if(result != kStatus_QMC_Ok) {
				dbgCloudPRINTF("Could not publish latest log to the cloud! Disconnecting...\n");
				goto exit_connected;
			}
		}

		/* system status */
		uint32_t systemStatus = xEventGroupGetBits(g_systemStatusEventGroupHandle);
		if(systemStatus != systemStatusOld || isIntervalElapsed(&systemStatusLastPubTime, MQTT_SLOW_PUBLISH_INTERVAL_MS) ) {
			result = CLOUD_PublishEventDriven(systemStatus, systemStatus ^ systemStatusOld);
			if(result != kStatus_QMC_Ok) {
				dbgCloudPRINTF("Could not publish system status to the cloud! Disconnecting...\n");
				goto exit_connected;
			}
			if(systemStatus != systemStatusOld) {
				systemStatusOld = systemStatus;
				systemStatusLastPubTime = xTaskGetTickCount();
			}
		}

		/* motor status */
		result = MC_DequeueMotorStatus(mcHandle, 0, &motorStatus);
		if((result != kStatus_QMC_ErrNoMsg)&&(result != kStatus_QMC_Timeout)&&(result != kStatus_QMC_Ok))
		{
			dbgCloudPRINTF("Could not get motor status queue contents! Disconnecting...");
			goto exit_connected;
		} else if(result == kStatus_QMC_Ok) {
			result = CLOUD_PublishMotorStatus(&motorStatus);
			if(result != kStatus_QMC_Ok) {
				dbgCloudPRINTF("Could not publish motor status to the cloud! Disconnecting...\n");
				goto exit_connected;
			}
		}
	}

exit_connected:
	cloud_conn_disconnect();
	(void) MC_ReturnStatusQueueHandle(mcHandle);
	(void) LOG_ReturnLoggingQueueHandle(logHandle);
exit:
	vTaskDelete(NULL);
}

bool CLOUD_IsServiceRunning(void)
{
	BaseType_t status = cloud_conn_status();
	if(pdTRUE == status) {
		return true;
	}
	return false;
}

#else

void CloudServiceTask(void *pvParameters)
{
	vTaskDelete(NULL);
}

bool CLOUD_IsServiceRunning(void)
{
	return false;
}

#endif /* USE_CLOUD */

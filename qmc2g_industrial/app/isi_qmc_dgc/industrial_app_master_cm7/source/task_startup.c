/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "api_qmc_common.h"
#include "qmc_features_config.h"
#include "api_board.h"
#include "api_rpc_internal.h"
#include "api_logging.h"
#include "api_usermanagement.h"
#include "api_configuration.h"
#include "fsl_tempsensor.h"
#include "MIMXRT1176_cm7.h"
#include "clock_config.h"
#include "tsn_tasks.h"
#include "datalogger_tasks.h"
#include "local_service_tasks.h"
#include "se_session.h"
#include "se_secure_sockets.h"
#include "freemaster_tasks.h"
#include "tsn_tasks_config.h"
#include "webservice/webservice.h"
#include "qmc_features_config.h"
#include "webservice/webservice_logging_task.h"

extern qmc_status_t DataHubInit(void);
static qmc_status_t installInitialUser(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern EventGroupHandle_t g_inputButtonEventGroupHandle;
extern EventGroupHandle_t g_systemStatusEventGroupHandle;
extern mc_motor_command_t g_sMotorCmdTst; /* Motor commands from command queue */

extern TaskHandle_t g_fault_handling_task_handle;
extern TaskHandle_t g_board_service_task_handle;
extern TaskHandle_t g_datahub_task_handle;
extern TaskHandle_t g_datalogger_task_handle;
extern TaskHandle_t g_local_service_task_handle;
extern TaskHandle_t g_freemaster_task_handle;
extern TaskHandle_t g_getMotorStatus_task_handle;
extern TaskHandle_t g_webservice_logging_task_handle;
extern TaskHandle_t g_json_motor_api_service_task_handle;
extern TaskHandle_t g_cloud_service_task_handle;
extern TaskHandle_t g_awdg_connection_service_task_handle;

/*******************************************************************************
 * Code
 ******************************************************************************/

void StartupTask(void *pvParameters)
{
	qmc_fw_update_state_t fwUpdateState = 0;

	/* Initialize GPIO/button input handlers */
	xEventGroupSetBits(g_inputButtonEventGroupHandle, QMC_IOEVENT_BTN1_RELEASED | QMC_IOEVENT_BTN2_RELEASED | QMC_IOEVENT_BTN3_RELEASED  | QMC_IOEVENT_BTN4_RELEASED | QMC_IOEVENT_INPUT0_LOW | QMC_IOEVENT_INPUT1_LOW | QMC_IOEVENT_INPUT2_LOW | QMC_IOEVENT_INPUT3_LOW | QMC_IOEVENT_INPUT4_LOW | QMC_IOEVENT_INPUT5_LOW | QMC_IOEVENT_INPUT6_LOW | QMC_IOEVENT_INPUT7_LOW);
	NVIC_SetPriority(BOARD_USER_BUTTON1_IRQ, 15);
    EnableIRQ(BOARD_USER_BUTTON1_IRQ);
	NVIC_SetPriority(BOARD_DIG_IN0_IRQ, 15);
    EnableIRQ(BOARD_DIG_IN0_IRQ);
    NVIC_SetPriority(BOARD_DIG_IN2_IRQ, 15);
    EnableIRQ(BOARD_DIG_IN2_IRQ);
    NVIC_SetPriority(BOARD_DIG_IN3_IRQ, 15);
    EnableIRQ(BOARD_DIG_IN3_IRQ);

    if (kStatus_QMC_Ok != BOARD_SetLifecycle(kQMC_LcMaintenance))
    {
    	goto error_task_init_failed;
    }

	/* Create secure element session */
    if(kStatus_QMC_Ok != SE_Initialize())
		goto error_task_init_failed;

    /* Initialize RPC interface */
    RPC_Init();

    if(kStatus_QMC_Ok != DataloggerInit())
		goto error_task_init_failed;

    if(kStatus_QMC_Ok != installInitialUser())
		goto error_task_init_failed;

    /*Initialize network addresses*/
    if(kStatus_QMC_Ok != Init_network_addresses())
    {
    	goto error_task_init_failed;
    }

#if FEATURE_FREEMASTER_ENABLE
    /* Initialize FreeMASTER */
	if(kStatus_QMC_Ok != init_freemaster_lpuart())
	{
		goto error_task_init_failed;
	}
	FMSTR_Init();
#endif

    /* Initialize Board API */
    if(kStatus_QMC_Ok != BOARD_Init())
        goto error_task_init_failed;

    /* Initialize DataHub and Motor API */
    if(kStatus_QMC_Ok != DataHubInit())
        goto error_task_init_failed;
	
	/* Initialize Fault API's Fault Queue */
    if(kStatus_QMC_Ok != FAULT_InitFaultQueue())
        goto error_task_init_failed;

    /* Initialize TSN task */
    if(kStatus_QMC_Ok != TsnInit())
    	goto error_task_init_failed;

    /* Initialize the Local Service task */
    if(kStatus_QMC_Ok != LocalServiceInit())
    	goto error_task_init_failed;

    /* Initialize the Board Service task */
    if(kStatus_QMC_Ok != BoardServiceInit())
        goto error_task_init_failed;

	/* Initialize IOT SDK with SE enablement */
    if(kStatus_QMC_Ok != SE_InitSecureSockets())
        goto error_task_init_failed;
	
    /* Schedule the webserver to be started by LWIP */
    webservice_init();


	/* Initialize motor control related peripherals and enable state machines */
    peripherals_manual_init();
    M1_FASTLOOP_TIMER_ENABLE();
    M2_FASTLOOP_TIMER_ENABLE();
    M3_FASTLOOP_TIMER_ENABLE();
    M4_FASTLOOP_TIMER_ENABLE();
    M1_SLOWLOOP_TIMER_ENABLE();
    M2_SLOWLOOP_TIMER_ENABLE();
    M3_SLOWLOOP_TIMER_ENABLE();
    M4_SLOWLOOP_TIMER_ENABLE();
    START_TIMER1();
    START_TIMER2();
    START_TIMER3();

    g_sMotorCmdTst.uSpeed_pos.sPosParam.bIsRandomPosition = true;
    g_sMotorCmdTst.eAppSwitch = kMC_App_Off;
    g_sMotorCmdTst.eMotorId = kMC_Motor1;
    g_sMotorCmdTst.eControlMethodSel = kMC_ScalarControl;
    g_sMotorCmdTst.uSpeed_pos.sScalarParam.fltScalarControlFrequency = 20;
    g_sMotorCmdTst.uSpeed_pos.sScalarParam.fltScalarControlVHzGain = 0.08;

	if (kStatus_QMC_Ok != RPC_GetFwUpdateState(&fwUpdateState))
	{
		goto error_task_init_failed;
	}

    if (fwUpdateState & kFWU_VerifyFw)
    {
    	xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FWUPDATE_VerifyMode);
    	vTaskDelay(pdMS_TO_TICKS(2500)); /* wait for the lwIP stack to init the link status */
    	if (kStatus_QMC_Ok == SelfTest())
    	{
    		if (kStatus_QMC_Ok != RPC_CommitFwUpdate())
			{
				xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FWUPDATE_VerifyMode);
				goto error_task_init_failed;
			}
    	}
    	else
    	{
    		if (kStatus_QMC_Ok != RPC_RevertFwUpdate())
			{
				xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FWUPDATE_VerifyMode);
				goto error_task_init_failed;
			}
    	}
    	xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FWUPDATE_VerifyMode);

    	/* Reset the device to apply the FW changes */
    	while (RPC_Reset(kQMC_ResetRequest) != kStatus_QMC_Ok){}
    }

    vTaskResume(g_fault_handling_task_handle);
    vTaskResume(g_board_service_task_handle);
    vTaskResume(g_datahub_task_handle);
    vTaskResume(g_datalogger_task_handle);
    vTaskResume(g_local_service_task_handle);
    vTaskResume(g_webservice_logging_task_handle);
    vTaskResume(g_json_motor_api_service_task_handle);
#if FEATURE_FREEMASTER_ENABLE
    vTaskResume(g_freemaster_task_handle);
#endif
#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
    vTaskResume(g_getMotorStatus_task_handle);
#endif
#if FEATURE_CLOUD_AZURE_IOTHUB || FEATURE_CLOUD_GENERIC_MQTT
    vTaskResume(g_cloud_service_task_handle);
#endif
#if FEATURE_SECURE_WATCHDOG
    vTaskResume(g_awdg_connection_service_task_handle);
#endif

    if (fwUpdateState & kFWU_AwdtExpired)
    {
    	log_record_t logEntry = {
    			.rhead = {
    					.chksum				= 0,
    					.uuid				= 0,
    					.ts = {
    						.seconds		= 0,
    						.milliseconds	= 0
    					}
    			},
    			.type = kLOG_SystemData,
    			.data.systemData.source = LOG_SRC_TaskStartup,
    			.data.systemData.category = LOG_CAT_General,
				.data.systemData.eventCode = LOG_EVENT_AWDTExpired
    	};

		LOG_QueueLogEntry(&logEntry, false);
    }

    vTaskDelete(NULL);
    return;

	error_task_init_failed:
	while(1)
		NVIC_SystemReset();
}

#define MAX_USER_DATA_LENGTH (7/*role + length bytes*/ + USRMGMT_USER_NAME_MAX_LENGTH + USRMGMT_SALT_LENGTH + USRMGMT_USER_SECRET_LENGTH)
static qmc_status_t installInitialUser(void)
{
	usrmgmt_user_config_t config;
    qmc_status_t rc;
    config_id_t id;

    /* look for valid user configurations */
    for (id = kCONFIG_Key_UserFirst; id <= kCONFIG_Key_UserLast; id++)
    {
        rc = CONFIG_GetBinValueById(id, (unsigned char *)&config, sizeof(config));
        if ((rc == kStatus_QMC_Ok) && (config.role != kUSRMGMT_RoleEmpty) && (config.role != kUSRMGMT_RoleNone))
        {
            break; /* a valid user configuration was found */
        }
    }
    /* if no valid user configuration has been found, install the default user */
    if(kCONFIG_Key_UserLast < id)
    {
    	sss_key_store_t *seKeyStore;
    	sss_object_t initialUserKeyObject;
    	sss_status_t status;
    	usrmgmt_user_config_t config;
    	uint8_t userData[MAX_USER_DATA_LENGTH];
    	size_t userDataLength = MAX_USER_DATA_LENGTH, userDataLengthBits = 8*MAX_USER_DATA_LENGTH;
    	size_t nameLength, saltLength, hashLength, iterationsIndex;
    	qmc_timestamp_t timestamp;

    	seKeyStore = SE_GetKeystore();

        /* fetch user authentication key */
        status = sss_key_object_init(&initialUserKeyObject, seKeyStore);
        if (kStatus_SSS_Success != status)
        	return kStatus_QMC_Err;
        status = sss_key_object_get_handle(&initialUserKeyObject, idDefaultUser);
        if (kStatus_SSS_Success != status) {
        	sss_key_object_free(&initialUserKeyObject);
        	return kStatus_QMC_Err;
        }
        status = sss_key_store_get_key(seKeyStore, &initialUserKeyObject, userData, &userDataLength, &userDataLengthBits);
        if (kStatus_SSS_Success != status) {
        	sss_key_object_free(&initialUserKeyObject);
        	return kStatus_QMC_Err;
        }
        sss_key_object_free(&initialUserKeyObject);

        /* Data format of userData is:
         * role:       4 bytes (LE)
         * nameLength: 1 byte
         * name:       nameLength bytes
         * saltLength: 1 byte
         * salt:       saltLength bytes
         * hashLength: 1 byte
         * hash:       hashLength bytes
         * iterations: 4 bytes (LE)
         */

        nameLength = userData[4];
        saltLength = userData[5 + nameLength];
        hashLength = userData[6 + nameLength + saltLength];
        iterationsIndex = 7 + nameLength + saltLength + hashLength;

        /* limit checks */
        if(   (nameLength > USRMGMT_USER_NAME_MAX_LENGTH-1)
           || (saltLength > USRMGMT_SALT_LENGTH)
		   || (hashLength > USRMGMT_USER_SECRET_LENGTH)
		   || (userDataLength < iterationsIndex )
		   )
        	return kStatus_QMC_Err;

        /* create configuration entry */
        config.role = userData[0] + (userData[1] << 8) + (userData[2] << 16) + (userData[3] << 24);
        memcpy(config.name, userData+5, nameLength);
        config.name[nameLength] = '\0';
        memcpy(config.salt, userData+6+nameLength, saltLength);
        memcpy(config.secret, userData+7+nameLength+saltLength, hashLength);
        config.lockout_timestamp = 0;
        config.iterations = USRMGMT_MIN_PASSPHRASE_ITERATIONS;
        config.validity_timestamp = USRMGMT_PASSPHRASE_DURATION;
        if (kStatus_QMC_Ok == BOARD_GetTime(&timestamp))
            config.validity_timestamp += timestamp.seconds;
        if(userDataLength > iterationsIndex) /* new provisioning tool */
        	config.iterations = userData[iterationsIndex] + (userData[iterationsIndex+1] << 8)
			                  + (userData[iterationsIndex+2] << 16) + (userData[iterationsIndex+3] << 24);

        rc = CONFIG_SetBinValueById(kCONFIG_Key_UserFirst, (const unsigned char *)&config, sizeof(config));
        return rc;
    }
    return kStatus_QMC_Ok;
}

/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "local_service_tasks.h"
#include "lvgl_support.h"
#include "gui_guider.h"
#include "qmc_features_config.h"
#include "api_fault.h"
#include "api_board.h"
#include "api_motorcontrol.h"
#include "api_logging.h"
#include "api_usermanagement.h"
#include "api_rpc.h"
#include "fsl_debug_console.h"
#include "constants.h"
#include "dispatcher.h"

#include <stdio.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LS_OPACITY_SHOW                 255u
#define LS_OPACITY_HIDE                 0u
#define MOTOR_QUEUE_TIMEOUT_ATTEMPTS    20u
#define MAX_LOG_LABELS					3u

typedef enum _tampering_type
{
    kLST_SdTamperingType,
	kLST_ButtonTamperingType
} ls_tampering_type_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief This function initializes the colors of the motor status tables and the necessary variables
 *
 * @param[in] index Index of the motor status table to be configured
 * @param[in] color_mode The color mode to be configured for the given motor status table
 */
static void initColor_Motor(mc_motor_id_t index, ls_color_mode_t color_mode);

/*!
 * @brief This function sets the colors of the motor status tables
 *
 * @param[in] index Index of the motor status table to be configured
 * @param[in] color_mode The color mode to be configured for the given motor status table
 */
static void setColor_Motor(mc_motor_id_t index, ls_color_mode_t color_mode);

/*!
 * @brief This function sets the text of a chosen motor status
 *
 * @param[in] index Index of the motor status table to be configured
 * @param[in] value Pointer to the array of characters to be set as the text
 * @param[in] motor_label_id The status that should be configured
 */
static void setText_MotorLabel(mc_motor_id_t index, const char *value, ls_motor_label_id_t motor_label_id);

/*!
 * @brief This function initializes the colors of the log labels and the necessary variables
 *
 * @param[in] index Index of the log row to be configured
 * @param[in] color_mode The color mode to be configured for the given log row
 */
static void initColor_LogLabel(ls_log_label_id_t index, ls_color_mode_t color_mode);

/*!
 * @brief This function sets the colors of the log labels
 *
 * @param[in] index Index of the log row to be configured
 * @param[in] color_mode The color mode to be configured for the given log row
 */
static void setColor_LogLabel(ls_log_label_id_t index, ls_color_mode_t color_mode);

/*!
 * @brief This function updates the log labels based on the array of last logs
 *
 * @param[in] lastLogs Pointer to an array of last logs
 * @param[in] activeLogLabels Amount of already active log rows
 */
static void update_LogLabels(log_record_t *lastLogs, unsigned int activeLogLabels);

/*!
 * @brief This function sets the text of the log labels
 *
 * @param[in] index Index of the log row to be configured
 * @param[in] message The log message to be displayed
 * @param[in] timestamp_string The timestamp of the log message
 */
static void setText_LogLabel(ls_log_label_id_t index, const char *message, const char *timestamp_string);

/*!
 * @brief This function updates the specified icon's style
 *
 * @param[in] icon The icon to be updated
 * @param[in] opacity Opacity settings [0-255]
 */
static void updateIconOpacity(lv_obj_t *icon, uint8_t opacity);

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
/*!
 * @brief This function starts or stops a motors based it's current state
 *
 * @param[in] motorId Id of the motor that should be started or stopped
 */
static void motorStartStop(mc_motor_id_t motorId);
#endif

/*!
 * @brief This function handles opening or closing of the button and SD lids
 *
 * @param[in] inputButtonEvent Input event that determines if the SD or the button lid was opened or closed
 */
static void lidOpenClose(uint32_t inputButtonEvent);

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
/*!
 * @brief This function handles pressing of the emergency button
 *
 */
static void emergencyPressed();
#endif

/*!
 * @brief This function handles a tampering event
 *
 * @param[in] type The type of the tampering event
 */
static void tamperingDetected(ls_tampering_type_t type);

/*!
 * @brief This logs that an invalid argument was received
 */
static void logInvalidArgument();
/*******************************************************************************
 * Globals
 ******************************************************************************/
static volatile bool gs_lvgl_initialized = false;
static lv_ui gs_guider_ui;
static lv_style_t gs_color_style_motor[MC_MAX_MOTORS];
static lv_style_t gs_color_style_log[3];
static bool gs_wasSetLidSd = false;
#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
	static bool gs_wasSet[4] = { false };
	static bool gs_wasSetLidButton = false;
	static bool gs_wasSetEmergency = false;
#endif
static bool gs_isRunning[4] = { false };
static bool gs_sdCardAvailablePrevious = false;
static bool gs_tamperingReported = false;
static qmc_msg_queue_handle_t *motorStatusQueue;

extern fault_system_fault_t g_systemFaultStatus;
extern EventGroupHandle_t g_systemStatusEventGroupHandle;
extern EventGroupHandle_t g_inputButtonEventGroupHandle;

#if (MC_HAS_AFE_ANY_MOTOR != 0)
	extern double g_PSBTemps[8];
#endif  /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

TaskHandle_t g_local_service_task_handle;
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief The init task that prepares the environment during initialization for the Local Service Task to work properly.
 *
 * @return QMC status
 */
qmc_status_t LocalServiceInit(void)
{
	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogLocalService) != kStatus_QMC_Ok)
	{
		FAULT_RaiseFaultEvent(kFAULT_FunctionalWatchdogInitFail);
	}

	uint32_t systemStatus = xEventGroupGetBits(g_systemStatusEventGroupHandle);
	uint32_t inputButtonEvent = xEventGroupGetBits(g_inputButtonEventGroupHandle);
	gs_sdCardAvailablePrevious = systemStatus & QMC_SYSEVENT_MEMORY_SdCardAvailable;

	if (systemStatus & QMC_SYSEVENT_LOG_FlashError)
	{
		BOARD_SetLifecycle(kQMC_LcError);

		for (mc_motor_id_t stopMotorId = kMC_Motor1; stopMotorId < MC_MAX_MOTORS; stopMotorId++)
		{
			mc_motor_command_t cmd = {0};
			cmd.eMotorId = stopMotorId;
			cmd.eAppSwitch = kMC_App_FreezeAndStop;

			int motorQueueTimeout = MOTOR_QUEUE_TIMEOUT_ATTEMPTS;
			do
			{
				qmc_status_t status = MC_QueueMotorCommand(&cmd);
				if ((status == kStatus_QMC_Ok) || (status == kStatus_QMC_ErrBusy))
				{
					break;
				}

				motorQueueTimeout--;
				vTaskDelay(10);
			} while (motorQueueTimeout > 0);

			/* Timed out */
			if (motorQueueTimeout <= 0)
			{
				/* Logging doesn't work */
				return kStatus_QMC_Err;
			}
		}
	}

	if (inputButtonEvent & QMC_IOEVENT_LID_OPEN_SD)
	{
		gs_wasSetLidSd = true;
	}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
	if (inputButtonEvent & QMC_IOEVENT_LID_OPEN_BUTTON)
	{
		gs_wasSetLidButton = true;
	}
#endif

	if (MC_GetNewStatusQueueHandle(&motorStatusQueue, 10) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}
	
	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogLocalService) != kStatus_QMC_Ok)
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
				.data.systemData.source = LOG_SRC_LocalService,
				.data.systemData.category = LOG_CAT_General,
				.data.systemData.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed
		};

		LOG_QueueLogEntry(&logEntryWithoutId, false);
	}

	return kStatus_QMC_Ok;
}


/*!
 * @brief The main Local Service task. It takes care of the GUI and button events, monitors tampering attempts and logs them.
 *
 * @param pvParameters Unused.
 */
void LocalServiceTask(void *pvParameters)
{
	qmc_status_t qmcStatus;
	mc_motor_status_t newMotorStatusFromDataHub;
	mc_motor_status_t oldMotorStatusArray[4];
	mc_state_t currentFastMotorState[4] = { kMC_Init, kMC_Init, kMC_Init, kMC_Init };
	uint32_t oldSystemStatus = 0;
	uint32_t oldLogId = UINT32_MAX;
	double oldTemps[4] = { 0.0, 0.0, 0.0, 0.0 };
	log_record_t lastLogs[3] = { 0 };
	unsigned int activeLogLabels = 0;
	int checkResult = 0;
	bool inErrorState = false;
	bool inMaintenanceState = false;
	bool reloadFastMotorStates = false;
	bool sdCardAvailableCurrent = gs_sdCardAvailablePrevious;
	TickType_t lastProcessedGUI = 0;
	TickType_t lastProcessedMotorLogs = 0;
	uint32_t inputButtonEvent = xEventGroupGetBits(g_inputButtonEventGroupHandle);

	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogLocalService) != kStatus_QMC_Ok)
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
				.data.systemData.source = LOG_SRC_LocalService,
				.data.systemData.category = LOG_CAT_General,
				.data.systemData.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed
		};

		LOG_QueueLogEntry(&logEntryWithoutId, false);
	}

	memset(oldMotorStatusArray, 0, sizeof(mc_motor_status_t) * 4);

    lv_init();

    lv_port_disp_init();

    gs_lvgl_initialized = true;

    setup_ui(&gs_guider_ui);

	initColor_Motor(kMC_Motor1, kLST_Color_Off);
	initColor_Motor(kMC_Motor2, kLST_Color_Off);
	initColor_Motor(kMC_Motor3, kLST_Color_Off);
	initColor_Motor(kMC_Motor4, kLST_Color_Off);

	initColor_LogLabel(kLST_Log_Label1, kLST_Color_Off);
	initColor_LogLabel(kLST_Log_Label2, kLST_Color_Off);
	initColor_LogLabel(kLST_Log_Label3, kLST_Color_Off);

	for (mc_motor_id_t motorId = kMC_Motor1; motorId <= kMC_Motor4; motorId++)
	{
		oldMotorStatusArray[motorId].sFast.eMotorState = kMC_Init;
		oldMotorStatusArray[motorId].eMotorId = motorId;

		setText_MotorLabel(motorId, "Init", kLST_Motor_Label_State);
		setText_MotorLabel(motorId, "Off", kLST_Motor_Label_Control_En);
		setText_MotorLabel(motorId, "0", kLST_Motor_Label_Speed);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Position);
		if (MC_PSBx_HAS_AFE(motorId))
		{
			setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Temperature);
		}
		else
		{
			setText_MotorLabel(motorId, "N/A", kLST_Motor_Label_Temperature);
		}
		setText_MotorLabel(motorId, "", kLST_Motor_Label_Fault);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Phase_A_Curr);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Phase_B_Curr);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Phase_C_Curr);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Alpha_V);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Beta_V);
		setText_MotorLabel(motorId, "0.0", kLST_Motor_Label_Db_Bus_V);
	}

	lv_task_handler();
	lastProcessedGUI = xTaskGetTickCount();

    for (;;)
    {
    	/* System Status - begin */
    	uint32_t newSystemStatus = xEventGroupGetBits(g_systemStatusEventGroupHandle);
    	if (newSystemStatus != oldSystemStatus)
    	{
    		oldSystemStatus = newSystemStatus;

    		if (newSystemStatus & QMC_SYSEVENT_LIFECYCLE_Operational)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_operational, LS_OPACITY_SHOW);
            	gs_tamperingReported = false;
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_operational, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_LIFECYCLE_Error)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_error, LS_OPACITY_SHOW);

            	if (!inErrorState)
            	{
					inErrorState = true;
					reloadFastMotorStates = true;

					for (mc_motor_id_t motorId = kMC_Motor1; motorId <= kMC_Motor4; motorId++)
					{
						setColor_Motor(motorId,  kLST_Color_Error);
					}
            	}
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_error, LS_OPACITY_HIDE);
            	inErrorState = false;
    		}

    		if (newSystemStatus & QMC_SYSEVENT_LIFECYCLE_Maintenance)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_maintenance, LS_OPACITY_SHOW);

            	if (!inMaintenanceState)
            	{
            		inMaintenanceState = true;
            		reloadFastMotorStates = true;

					for (mc_motor_id_t motorId = kMC_Motor1; motorId <= kMC_Motor4; motorId++)
					{
						setColor_Motor(motorId,  kLST_Color_Maintenance);
					}
            	}
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_maintenance, LS_OPACITY_HIDE);
            	inMaintenanceState = false;
    		}

    		if (newSystemStatus & QMC_SYSEVENT_FAULT_Motor1 || newSystemStatus & QMC_SYSEVENT_FAULT_Motor2 ||\
    			newSystemStatus & QMC_SYSEVENT_FAULT_Motor3 || newSystemStatus & QMC_SYSEVENT_FAULT_Motor4 ||\
				newSystemStatus & QMC_SYSEVENT_FAULT_System)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_fault, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_fault, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_FWUPDATE_RestartRequired || newSystemStatus & QMC_SYSEVENT_FWUPDATE_VerifyMode)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_fw_update, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_fw_update, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_CONFIGURATION_ConfigChanged || newSystemStatus & QMC_SYSEVENT_LIFECYCLE_Commissioning ||\
    			newSystemStatus & QMC_SYSEVENT_LIFECYCLE_Decommissioning)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_configuration, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_configuration, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_ANOMALYDETECTION_Audio || newSystemStatus & QMC_SYSEVENT_ANOMALYDETECTION_Current)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_anomaly_detection, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_anomaly_detection, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_SHUTDOWN_PowerLoss || newSystemStatus & QMC_SYSEVENT_SHUTDOWN_WatchdogReset)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_shutdown, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_shutdown, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_MEMORY_SdCardAvailable)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_memory, LS_OPACITY_SHOW);
            	sdCardAvailableCurrent = true;
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_memory, LS_OPACITY_HIDE);
				sdCardAvailableCurrent = false;
    		}

    		if (newSystemStatus & QMC_SYSEVENT_NETWORK_TsnSyncLost || newSystemStatus & QMC_SYSEVENT_NETWORK_NoLink)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_network, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_network, LS_OPACITY_HIDE);
    		}

    		if (newSystemStatus & QMC_SYSEVENT_LOG_FlashError || newSystemStatus & QMC_SYSEVENT_LOG_LowMemory)
			{
    			updateIconOpacity(gs_guider_ui.screen_img_log, LS_OPACITY_SHOW);
			}
    		else
    		{
    			updateIconOpacity(gs_guider_ui.screen_img_log, LS_OPACITY_HIDE);
    		}
    	}


		if (!inErrorState && !inMaintenanceState && reloadFastMotorStates)
		{
			for (mc_motor_id_t motorId = kMC_Motor1; motorId <= kMC_Motor4; motorId++)
			{
				switch (currentFastMotorState[motorId])
				{
				case kMC_Fault:
					setColor_Motor(motorId, kLST_Color_Fault);
					break;

				case kMC_Init:
					setColor_Motor(motorId, kLST_Color_Off);
					break;

				case kMC_Stop:
					setColor_Motor(motorId, kLST_Color_Off);
					break;

				case kMC_Run:
					setColor_Motor(motorId, kLST_Color_Operational);
					break;

				default:
					logInvalidArgument();
					break;
				}
			}

			reloadFastMotorStates = false;
		}
    	/* System Status - end */

		if ((xTaskGetTickCount() - lastProcessedMotorLogs) > pdMS_TO_TICKS(MOTOR_STATUS_AND_LOGS_DELAY_AT_LEAST_MS))
		{
			lastProcessedMotorLogs = xTaskGetTickCount();
			/* Log update - begin */
			uint32_t newLogId = LOG_GetLastLogId();

			if (newLogId != oldLogId)
			{
				if (activeLogLabels == 0)
				{
					activeLogLabels = 1;
				}
				else if (activeLogLabels == 1)
				{
					activeLogLabels = 2;
					lastLogs[1] = lastLogs[0];
				}
				else if (activeLogLabels >= 2)
				{
					activeLogLabels = 3;
					lastLogs[2] = lastLogs[1];
					lastLogs[1] = lastLogs[0];
				}
				else
				{
					/* Do Nothing*/
					;
				}

				qmcStatus = LOG_GetLogRecord(newLogId, &lastLogs[0]);

				if (qmcStatus == kStatus_QMC_Ok)
				{
					update_LogLabels(lastLogs, activeLogLabels);

					oldLogId = newLogId;
				}
				else
				{
					/* Failed to get a new log. Revert the changes to the lastLogs array. */
					lastLogs[0] = lastLogs[1];
					lastLogs[1] = lastLogs[2];

					if (activeLogLabels > 0)
					{
						activeLogLabels--;
					}
				}
			}
			/* Log update - end */



			/* Motor Status - begin */
			qmcStatus = MC_DequeueMotorStatus(motorStatusQueue, 0, &newMotorStatusFromDataHub);

			if (qmcStatus != kStatus_QMC_ErrNoMsg)
			{
				if(qmcStatus == kStatus_QMC_Ok)
				{
					mc_motor_status_t * oldMotorStatusPtr = &oldMotorStatusArray[newMotorStatusFromDataHub.eMotorId];

					if (newMotorStatusFromDataHub.sFast.eMotorState != oldMotorStatusPtr->sFast.eMotorState)
					{
						switch(newMotorStatusFromDataHub.sFast.eMotorState)
						{
						case kMC_Fault:
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Fault", kLST_Motor_Label_State);

							if (!inErrorState && !inMaintenanceState)
							{
								setColor_Motor(newMotorStatusFromDataHub.eMotorId, kLST_Color_Fault);
							}

							gs_isRunning[newMotorStatusFromDataHub.eMotorId] = false;
							break;

						case kMC_Init:
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Init", kLST_Motor_Label_State);

							if (!inErrorState && !inMaintenanceState)
							{
								setColor_Motor(newMotorStatusFromDataHub.eMotorId, kLST_Color_Off);
							}

							gs_isRunning[newMotorStatusFromDataHub.eMotorId] = false;
							break;

						case kMC_Stop:
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Stop", kLST_Motor_Label_State);

							if (!inErrorState && !inMaintenanceState)
							{
								setColor_Motor(newMotorStatusFromDataHub.eMotorId, kLST_Color_Off);
							}

							gs_isRunning[newMotorStatusFromDataHub.eMotorId] = false;
							break;

						case kMC_Run:
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Run", kLST_Motor_Label_State);

							if (!inErrorState && !inMaintenanceState)
							{
								setColor_Motor(newMotorStatusFromDataHub.eMotorId, kLST_Color_Operational);
							}

							gs_isRunning[newMotorStatusFromDataHub.eMotorId] = true;
							break;

						default:
							logInvalidArgument();
							break;
						}

						currentFastMotorState[newMotorStatusFromDataHub.eMotorId] = newMotorStatusFromDataHub.sFast.eMotorState;
						oldMotorStatusPtr->sFast.eMotorState = newMotorStatusFromDataHub.sFast.eMotorState;
					}

					if (newMotorStatusFromDataHub.sSlow.eAppSwitch != oldMotorStatusPtr->sSlow.eAppSwitch)
					{
						if (newMotorStatusFromDataHub.sSlow.eAppSwitch == 0)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Off", kLST_Motor_Label_Control_En);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "On", kLST_Motor_Label_Control_En);
						}

						oldMotorStatusPtr->sSlow.eAppSwitch = newMotorStatusFromDataHub.sSlow.eAppSwitch;
					}

					if (newMotorStatusFromDataHub.sSlow.fltSpeed != oldMotorStatusPtr->sSlow.fltSpeed)
					{
						char value[8] = {0};
						float fltSpeed = newMotorStatusFromDataHub.sSlow.fltSpeed;

						if (fltSpeed > 9999.0)
						{
							checkResult = snprintf(value, 8, "> 9999");
						}
						else if (fltSpeed < -9999.0)
						{
							checkResult = snprintf(value, 8, "< -9999");
						}
						else
						{
							checkResult = snprintf(value, 8, "%.0f", fltSpeed);
						}

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Speed);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Speed);
						}

						oldMotorStatusPtr->sSlow.fltSpeed = newMotorStatusFromDataHub.sSlow.fltSpeed;
					}

					if (newMotorStatusFromDataHub.sSlow.uPosition.i32Raw != oldMotorStatusPtr->sSlow.uPosition.i32Raw)
					{
						char value[8] = {0};
						double position = ((double) newMotorStatusFromDataHub.sSlow.uPosition.i32Raw/65536);

						if (position > 9999.0)
						{
							checkResult = snprintf(value, 8, "> 9999");
						}
						else if (position < -9999.0)
						{
							checkResult = snprintf(value, 8, "< -9999");
						}
						else
						{
							checkResult = snprintf(value, 8, "%.1f", position);
						}

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Position);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Position);
						}

						oldMotorStatusPtr->sSlow.uPosition.i32Raw = newMotorStatusFromDataHub.sSlow.uPosition.i32Raw;
					}

	#if (MC_HAS_AFE_ANY_MOTOR != 0)
					/* TEMP_AFE - first of the two measured temperatures per motor (it's connected to the PWM comparator)
					 * motor[0] => TEMP_AFE == g_PSBTemps[0], TEMP_AFE1 == g_PSBTemps[1]
					 * motor[1] => TEMP_AFE == g_PSBTemps[2], TEMP_AFE1 == g_PSBTemps[3]
					 * Index calculation: motorId * 2 = g_PSBTemps index => 0*2 = 0, 1*2 = 2...
					 */
					if ((MC_PSBx_HAS_AFE(newMotorStatusFromDataHub.eMotorId)) &&\
						(oldTemps[newMotorStatusFromDataHub.eMotorId] != g_PSBTemps[newMotorStatusFromDataHub.eMotorId * 2]))
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", g_PSBTemps[newMotorStatusFromDataHub.eMotorId * 2]);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Temperature);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Temperature);
						}

						oldTemps[newMotorStatusFromDataHub.eMotorId] = g_PSBTemps[newMotorStatusFromDataHub.eMotorId * 2];
					}
	#endif /* #if (MC_HAS_AFE_ANY_MOTOR != 0) */

					if (newMotorStatusFromDataHub.sFast.eFaultStatus != oldMotorStatusPtr->sFast.eFaultStatus)
					{
						if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_OverCurrent)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OCurr", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_UnderDcBusVoltage)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "UVolt", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_OverDcBusVoltage)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OVolt", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_OverLoad)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OLoad", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_OverSpeed)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OSpeed", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_RotorBlocked)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Rtr Blk", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_PsbOverTemperature1)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OT 1", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_PsbOverTemperature2)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OT 2", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_GD3000_OverTemperature)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OT GD", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_GD3000_Desaturation)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Desat", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_GD3000_LowVLS)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Low VLS", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_GD3000_OverCurrent)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "OC GD", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_GD3000_PhaseError)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "PhErr", kLST_Motor_Label_Fault);
						}
						else if (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_GD3000_Reset)
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "Rst GD", kLST_Motor_Label_Fault);
						}
						else if ((newMotorStatusFromDataHub.sFast.eFaultStatus == kMC_NoFaultMC) || (newMotorStatusFromDataHub.sFast.eFaultStatus & kMC_NoFaultBS))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "", kLST_Motor_Label_Fault);
						}

						oldMotorStatusPtr->sFast.eFaultStatus = newMotorStatusFromDataHub.sFast.eFaultStatus;
					}

					if (newMotorStatusFromDataHub.sFast.fltIa != oldMotorStatusPtr->sFast.fltIa)
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", newMotorStatusFromDataHub.sFast.fltIa);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Phase_A_Curr);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Phase_A_Curr);
						}

						oldMotorStatusPtr->sFast.fltIa = newMotorStatusFromDataHub.sFast.fltIa;
					}

					if (newMotorStatusFromDataHub.sFast.fltIb != oldMotorStatusPtr->sFast.fltIb)
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", newMotorStatusFromDataHub.sFast.fltIb);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Phase_B_Curr);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Phase_B_Curr);
						}

						oldMotorStatusPtr->sFast.fltIb = newMotorStatusFromDataHub.sFast.fltIb;
					}

					if (newMotorStatusFromDataHub.sFast.fltIc != oldMotorStatusPtr->sFast.fltIc)
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", newMotorStatusFromDataHub.sFast.fltIc);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Phase_C_Curr);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Phase_C_Curr);
						}

						oldMotorStatusPtr->sFast.fltIc = newMotorStatusFromDataHub.sFast.fltIc;
					}

					if (newMotorStatusFromDataHub.sFast.fltValpha != oldMotorStatusPtr->sFast.fltValpha)
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", newMotorStatusFromDataHub.sFast.fltValpha);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Alpha_V);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Alpha_V);
						}

						oldMotorStatusPtr->sFast.fltValpha = newMotorStatusFromDataHub.sFast.fltValpha;
					}

					if (newMotorStatusFromDataHub.sFast.fltVbeta != oldMotorStatusPtr->sFast.fltVbeta)
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", newMotorStatusFromDataHub.sFast.fltVbeta);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Beta_V);
						}
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Beta_V);
						}

						oldMotorStatusPtr->sFast.fltVbeta = newMotorStatusFromDataHub.sFast.fltVbeta;
					}

					if (newMotorStatusFromDataHub.sFast.fltVDcBus != oldMotorStatusPtr->sFast.fltVDcBus)
					{
						char value[8] = {0};
						checkResult = snprintf(value, 8, "%.2f", newMotorStatusFromDataHub.sFast.fltVDcBus);

						if (checkResult < 0 || checkResult >= sizeof(value))
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, "err", kLST_Motor_Label_Db_Bus_V);
						}
						else
						{
							setText_MotorLabel(newMotorStatusFromDataHub.eMotorId, value, kLST_Motor_Label_Db_Bus_V);
						}

						oldMotorStatusPtr->sFast.fltVDcBus = newMotorStatusFromDataHub.sFast.fltVDcBus;
					}
				}
				else
				{
					PRINTF("Failed to get Motor Status Queue contents");
				}
			}
			/* Motor Status - end */
		}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
		/* Button Events - begin */
		log_record_t logEntry = {0};
		logEntry.type = kLOG_DefaultData;
		logEntry.data.defaultData.source = LOG_SRC_LocalService;
		logEntry.data.defaultData.category = LOG_CAT_General;
		logEntry.data.defaultData.user = kUSRMGMT_RoleLocalButton;

		if (inputButtonEvent & QMC_IOEVENT_BTN1_PRESSED)
		{
			if (!gs_wasSet[kMC_Motor1])
			{
				gs_wasSet[kMC_Motor1] = true;

				if (gs_wasSetLidButton)
				{
					if (!inErrorState && !inMaintenanceState)
					{
						motorStartStop(kMC_Motor1);

						logEntry.data.defaultData.eventCode = LOG_EVENT_Button1Pressed;
						LOG_QueueLogEntry(&logEntry, true);
					}
				}
				else
				{
					if (!gs_tamperingReported)
					{
						tamperingDetected(kLST_ButtonTamperingType);
					}
				}
			}
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN2_PRESSED)
		{
			if (!gs_wasSet[kMC_Motor2])
			{
				gs_wasSet[kMC_Motor2] = true;

				if (gs_wasSetLidButton)
				{
					if (!inErrorState && !inMaintenanceState)
					{
						motorStartStop(kMC_Motor2);

						logEntry.data.defaultData.eventCode = LOG_EVENT_Button2Pressed;
						LOG_QueueLogEntry(&logEntry, true);
					}
				}
				else
				{
					if (!gs_tamperingReported)
					{
						tamperingDetected(kLST_ButtonTamperingType);
					}
				}
			}
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN3_PRESSED)
		{
			if (!gs_wasSet[kMC_Motor3])
			{
				gs_wasSet[kMC_Motor3] = true;

				if (gs_wasSetLidButton)
				{
					if (!inErrorState && !inMaintenanceState)
					{
						motorStartStop(kMC_Motor3);

						logEntry.data.defaultData.eventCode = LOG_EVENT_Button3Pressed;
						LOG_QueueLogEntry(&logEntry, true);
					}
				}
				else
				{
					if (!gs_tamperingReported)
					{
						tamperingDetected(kLST_ButtonTamperingType);
					}
				}
			}
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN4_PRESSED)
		{
			if (!gs_wasSet[kMC_Motor4])
			{
				gs_wasSet[kMC_Motor4] = true;

				if (gs_wasSetLidButton)
				{
					if (!inErrorState && !inMaintenanceState)
					{
						motorStartStop(kMC_Motor4);

						logEntry.data.defaultData.eventCode = LOG_EVENT_Button4Pressed;
						LOG_QueueLogEntry(&logEntry, true);
					}
				}
				else
				{
					if (!gs_tamperingReported)
					{
						tamperingDetected(kLST_ButtonTamperingType);
					}
				}
			}
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN1_RELEASED)
		{
			gs_wasSet[kMC_Motor1] = false;
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN2_RELEASED)
		{
			gs_wasSet[kMC_Motor2] = false;
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN3_RELEASED)
		{
			gs_wasSet[kMC_Motor3] = false;
		}

		if (inputButtonEvent & QMC_IOEVENT_BTN4_RELEASED)
		{
			gs_wasSet[kMC_Motor4] = false;
		}
#endif

		if (gs_sdCardAvailablePrevious != sdCardAvailableCurrent)
		{
			if (!gs_wasSetLidSd)
			{
				if (!gs_tamperingReported)
				{
					tamperingDetected(kLST_SdTamperingType);
				}
			}

			gs_sdCardAvailablePrevious = sdCardAvailableCurrent;
		}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
		if (inputButtonEvent & QMC_IOEVENT_LID_OPEN_SD|| inputButtonEvent & QMC_IOEVENT_LID_OPEN_BUTTON)
#else
		if (inputButtonEvent & QMC_IOEVENT_LID_OPEN_SD)
#endif
		{
			lidOpenClose(inputButtonEvent);
		}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
		if (inputButtonEvent & QMC_IOEVENT_EMERGENCY_PRESSED)
		{
			if (!inErrorState && !inMaintenanceState)
			{
				emergencyPressed();
			}
		}
#endif

		if (inputButtonEvent & QMC_IOEVENT_LID_CLOSE_SD)
		{
			gs_wasSetLidSd = false;
		}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
		if (inputButtonEvent & QMC_IOEVENT_LID_CLOSE_BUTTON)
		{
			gs_wasSetLidButton = false;
		}

		if (inputButtonEvent & QMC_IOEVENT_EMERGENCY_RELEASED)
		{
			gs_wasSetEmergency = false;
		}
		/* Button Events - end */
#endif

		/* Checking the subtraction result to satisfy CERT-C INT30-C rule */
		TickType_t currentTickCount = xTaskGetTickCount();
		TickType_t tickCountDifference = currentTickCount - lastProcessedGUI;

		if (tickCountDifference > currentTickCount)
		{
			tickCountDifference = 0;
		}

		if (tickCountDifference >= pdMS_TO_TICKS(GUI_HANDLER_DELAY_AT_LEAST_MS))
		{
			if( dispatcher_get_flash_lock(pdMS_TO_TICKS(1000)) == kStatus_QMC_Ok)
			{
				lv_task_handler();

				dispatcher_release_flash_lock();
				lastProcessedGUI = xTaskGetTickCount();
			}

	    	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogLocalService) != kStatus_QMC_Ok)
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
	    				.data.systemData.source = LOG_SRC_LocalService,
	    				.data.systemData.category = LOG_CAT_General,
	    				.data.systemData.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed
	    		};

	    		LOG_QueueLogEntry(&logEntryWithoutId, false);
	    	}
		}



		EventBits_t waitBits = gs_wasSet[kMC_Motor1] ? QMC_IOEVENT_BTN1_RELEASED : QMC_IOEVENT_BTN1_PRESSED;
		waitBits |= gs_wasSet[kMC_Motor2] ? QMC_IOEVENT_BTN2_RELEASED : QMC_IOEVENT_BTN2_PRESSED;
		waitBits |= gs_wasSet[kMC_Motor3] ? QMC_IOEVENT_BTN3_RELEASED : QMC_IOEVENT_BTN3_PRESSED;
		waitBits |= gs_wasSet[kMC_Motor4] ? QMC_IOEVENT_BTN4_RELEASED : QMC_IOEVENT_BTN4_PRESSED;
		waitBits |= gs_wasSetEmergency ? QMC_IOEVENT_EMERGENCY_RELEASED : QMC_IOEVENT_EMERGENCY_PRESSED;
		waitBits |= (gs_wasSetLidSd) ? QMC_IOEVENT_LID_CLOSE_SD : QMC_IOEVENT_LID_OPEN_SD;
		waitBits |= (gs_wasSetLidButton) ? QMC_IOEVENT_LID_CLOSE_BUTTON : QMC_IOEVENT_LID_OPEN_BUTTON;
		inputButtonEvent = xEventGroupWaitBits(g_inputButtonEventGroupHandle, waitBits, pdFALSE, pdFALSE, pdMS_TO_TICKS(TASK_DELAY_MS));
    }
}

/*!
 * @brief This function initializes the colors of the motor status tables and the necessary variables
 *
 * @param[in] index Index of the motor status table to be configured
 * @param[in] color_mode The color mode to be configured for the given motor status table
 */
static void initColor_Motor(mc_motor_id_t index, ls_color_mode_t color_mode)
{
	if (index >= MC_MAX_MOTORS)
	{
		return;
	}

	lv_style_t * color_style;
	color_style = &gs_color_style_motor[index];
	lv_style_init(color_style);

	switch(color_mode)
	{
	case kLST_Color_Operational:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Maintenance:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Fault:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Error:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Off:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_DARK));
		break;

	default:
		logInvalidArgument();
		return;
	}

	switch(index)
	{
	case kMC_Motor1:
		lv_obj_add_style(gs_guider_ui.screen_label_motor_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_state_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_state_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_temperature_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_temperature_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_cont_en_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_cont_en_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_speed_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_speed_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_position_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_position_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_fault_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_fault_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_a_curr_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_a_curr_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_b_curr_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_b_curr_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_c_curr_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_c_curr_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_alpha_v_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_alpha_v_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_beta_v_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_beta_v_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_dc_bus_v_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_dc_bus_v_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	case kMC_Motor2:
		lv_obj_add_style(gs_guider_ui.screen_label_motor_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_state_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_state_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_temperature_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_temperature_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_cont_en_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_cont_en_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_speed_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_speed_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_position_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_position_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_fault_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_fault_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_a_curr_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_a_curr_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_b_curr_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_b_curr_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_c_curr_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_c_curr_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_alpha_v_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_alpha_v_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_beta_v_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_beta_v_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_dc_bus_v_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_dc_bus_v_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	case kMC_Motor3:
		lv_obj_add_style(gs_guider_ui.screen_label_motor_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_state_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_state_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_temperature_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_temperature_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_cont_en_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_cont_en_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_speed_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_speed_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_position_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_position_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_fault_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_fault_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_a_curr_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_a_curr_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_b_curr_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_b_curr_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_c_curr_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_c_curr_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_alpha_v_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_alpha_v_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_beta_v_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_beta_v_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_dc_bus_v_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_dc_bus_v_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	case kMC_Motor4:
		lv_obj_add_style(gs_guider_ui.screen_label_motor_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_state_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_state_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_temperature_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_temperature_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_cont_en_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_cont_en_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_speed_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_speed_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_position_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_position_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_fault_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_fault_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_a_curr_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_a_curr_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_b_curr_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_b_curr_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_phase_c_curr_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_phase_c_curr_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_alpha_v_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_alpha_v_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_beta_v_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_beta_v_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);

		lv_obj_add_style(gs_guider_ui.screen_label_dc_bus_v_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_value_dc_bus_v_4, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	default:
		logInvalidArgument();
		return;
	}
}

/*!
 * @brief This function sets the colors of the motor status tables
 *
 * @param[in] index Index of the motor status table to be configured
 * @param[in] color_mode The color mode to be configured for the given motor status table
 */
static void setColor_Motor(mc_motor_id_t index, ls_color_mode_t color_mode)
{
	if (index >= MC_MAX_MOTORS)
	{
		return;
	}

	lv_style_t * color_style;
	color_style = &gs_color_style_motor[index];

	if (color_style->prop_cnt <= 1)
	{
		return;
	}
	else
	{
		lv_style_reset(color_style);
	}

	switch(color_mode)
	{
	case kLST_Color_Operational:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Maintenance:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Fault:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Error:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Off:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_DARK));
		break;

	default:
		logInvalidArgument();
		return;
	}

	switch(index)
	{
	case kMC_Motor1:
		lv_obj_refresh_style(gs_guider_ui.screen_label_motor_1, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	case kMC_Motor2:
		lv_obj_refresh_style(gs_guider_ui.screen_label_motor_2, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	case kMC_Motor3:
		lv_obj_refresh_style(gs_guider_ui.screen_label_motor_3, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	case kMC_Motor4:
		lv_obj_refresh_style(gs_guider_ui.screen_label_motor_4, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	default:
		logInvalidArgument();
		return;
	}
}

/*!
 * @brief This function sets the text of a chosen motor status
 *
 * @param[in] index Index of the motor status table to be configured
 * @param[in] value Pointer to the array of characters to be set as the text
 * @param[in] motor_label_id The status that should be configured
 */
static void setText_MotorLabel(mc_motor_id_t index, const char *value, ls_motor_label_id_t motor_label_id)
{
	switch(index)
	{
	case kMC_Motor1:
		switch(motor_label_id)
		{
		case kLST_Motor_Label_State:
			lv_label_set_text(gs_guider_ui.screen_value_state_1, value);
			break;

		case kLST_Motor_Label_Control_En:
			lv_label_set_text(gs_guider_ui.screen_value_cont_en_1, value);
			break;

		case kLST_Motor_Label_Speed:
			lv_label_set_text(gs_guider_ui.screen_value_speed_1, value);
			break;

		case kLST_Motor_Label_Position:
			lv_label_set_text(gs_guider_ui.screen_value_position_1, value);
			break;

		case kLST_Motor_Label_Temperature:
			lv_label_set_text(gs_guider_ui.screen_value_temperature_1, value);
			break;

		case kLST_Motor_Label_Fault:
			lv_label_set_text(gs_guider_ui.screen_value_fault_1, value);
			break;

		case kLST_Motor_Label_Phase_A_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_a_curr_1, value);
			break;

		case kLST_Motor_Label_Phase_B_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_b_curr_1, value);
			break;

		case kLST_Motor_Label_Phase_C_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_c_curr_1, value);
			break;

		case kLST_Motor_Label_Alpha_V:
			lv_label_set_text(gs_guider_ui.screen_value_alpha_v_1, value);
			break;

		case kLST_Motor_Label_Beta_V:
			lv_label_set_text(gs_guider_ui.screen_value_beta_v_1, value);
			break;

		case kLST_Motor_Label_Db_Bus_V:
			lv_label_set_text(gs_guider_ui.screen_value_dc_bus_v_1, value);
			break;

		default:
			logInvalidArgument();
			break;
		}
		break;

	case kMC_Motor2:
		switch(motor_label_id)
		{
		case kLST_Motor_Label_State:
			lv_label_set_text(gs_guider_ui.screen_value_state_2, value);
			break;

		case kLST_Motor_Label_Control_En:
			lv_label_set_text(gs_guider_ui.screen_value_cont_en_2, value);
			break;

		case kLST_Motor_Label_Speed:
			lv_label_set_text(gs_guider_ui.screen_value_speed_2, value);
			break;

		case kLST_Motor_Label_Position:
			lv_label_set_text(gs_guider_ui.screen_value_position_2, value);
			break;

		case kLST_Motor_Label_Temperature:
			lv_label_set_text(gs_guider_ui.screen_value_temperature_2, value);
			break;

		case kLST_Motor_Label_Fault:
			lv_label_set_text(gs_guider_ui.screen_value_fault_2, value);
			break;

		case kLST_Motor_Label_Phase_A_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_a_curr_2, value);
			break;

		case kLST_Motor_Label_Phase_B_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_b_curr_2, value);
			break;

		case kLST_Motor_Label_Phase_C_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_c_curr_2, value);
			break;

		case kLST_Motor_Label_Alpha_V:
			lv_label_set_text(gs_guider_ui.screen_value_alpha_v_2, value);
			break;

		case kLST_Motor_Label_Beta_V:
			lv_label_set_text(gs_guider_ui.screen_value_beta_v_2, value);
			break;

		case kLST_Motor_Label_Db_Bus_V:
			lv_label_set_text(gs_guider_ui.screen_value_dc_bus_v_2, value);
			break;

		default:
			logInvalidArgument();
			break;
		}
		break;

	case kMC_Motor3:
		switch(motor_label_id)
		{
		case kLST_Motor_Label_State:
			lv_label_set_text(gs_guider_ui.screen_value_state_3, value);
			break;

		case kLST_Motor_Label_Control_En:
			lv_label_set_text(gs_guider_ui.screen_value_cont_en_3, value);
			break;

		case kLST_Motor_Label_Speed:
			lv_label_set_text(gs_guider_ui.screen_value_speed_3, value);
			break;

		case kLST_Motor_Label_Position:
			lv_label_set_text(gs_guider_ui.screen_value_position_3, value);
			break;

		case kLST_Motor_Label_Temperature:
			lv_label_set_text(gs_guider_ui.screen_value_temperature_3, value);
			break;

		case kLST_Motor_Label_Fault:
			lv_label_set_text(gs_guider_ui.screen_value_fault_3, value);
			break;

		case kLST_Motor_Label_Phase_A_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_a_curr_3, value);
			break;

		case kLST_Motor_Label_Phase_B_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_b_curr_3, value);
			break;

		case kLST_Motor_Label_Phase_C_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_c_curr_3, value);
			break;

		case kLST_Motor_Label_Alpha_V:
			lv_label_set_text(gs_guider_ui.screen_value_alpha_v_3, value);
			break;

		case kLST_Motor_Label_Beta_V:
			lv_label_set_text(gs_guider_ui.screen_value_beta_v_3, value);
			break;

		case kLST_Motor_Label_Db_Bus_V:
			lv_label_set_text(gs_guider_ui.screen_value_dc_bus_v_3, value);
			break;

		default:
			logInvalidArgument();
			break;
		}
		break;

	case kMC_Motor4:
		switch(motor_label_id)
		{
		case kLST_Motor_Label_State:
			lv_label_set_text(gs_guider_ui.screen_value_state_4, value);
			break;

		case kLST_Motor_Label_Control_En:
			lv_label_set_text(gs_guider_ui.screen_value_cont_en_4, value);
			break;

		case kLST_Motor_Label_Speed:
			lv_label_set_text(gs_guider_ui.screen_value_speed_4, value);
			break;

		case kLST_Motor_Label_Position:
			lv_label_set_text(gs_guider_ui.screen_value_position_4, value);
			break;

		case kLST_Motor_Label_Temperature:
			lv_label_set_text(gs_guider_ui.screen_value_temperature_4, value);
			break;

		case kLST_Motor_Label_Fault:
			lv_label_set_text(gs_guider_ui.screen_value_fault_4, value);
			break;

		case kLST_Motor_Label_Phase_A_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_a_curr_4, value);
			break;

		case kLST_Motor_Label_Phase_B_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_b_curr_4, value);
			break;

		case kLST_Motor_Label_Phase_C_Curr:
			lv_label_set_text(gs_guider_ui.screen_value_phase_c_curr_4, value);
			break;

		case kLST_Motor_Label_Alpha_V:
			lv_label_set_text(gs_guider_ui.screen_value_alpha_v_4, value);
			break;

		case kLST_Motor_Label_Beta_V:
			lv_label_set_text(gs_guider_ui.screen_value_beta_v_4, value);
			break;

		case kLST_Motor_Label_Db_Bus_V:
			lv_label_set_text(gs_guider_ui.screen_value_dc_bus_v_4, value);
			break;

		default:
			logInvalidArgument();
			break;
		}
		break;

	default:
		logInvalidArgument();
		return;
	}
}

/*!
 * @brief This function initializes the colors of the log labels and the necessary variables
 *
 * @param[in] index Index of the log row to be configured
 * @param[in] color_mode The color mode to be configured for the given log row
 */
static void initColor_LogLabel(ls_log_label_id_t index, ls_color_mode_t color_mode)
{
	lv_style_t * color_style;
	color_style = &gs_color_style_log[index];

	lv_style_init(color_style);

	switch(color_mode)
	{
	case kLST_Color_Operational:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Maintenance:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Fault:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Error:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Off:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_DARK));
		break;

	default:
		logInvalidArgument();
		return;
	}

	switch(index)
	{
	case kLST_Log_Label1:
		lv_obj_add_style(gs_guider_ui.screen_label_log_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_label_log_datetime_1, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	case kLST_Log_Label2:
		lv_obj_add_style(gs_guider_ui.screen_label_log_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_label_log_datetime_2, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	case kLST_Log_Label3:
		lv_obj_add_style(gs_guider_ui.screen_label_log_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		lv_obj_add_style(gs_guider_ui.screen_label_log_datetime_3, color_style, LV_PART_MAIN|LV_STATE_DEFAULT);
		break;

	default:
		logInvalidArgument();
		return;
	}
}

/*!
 * @brief This function sets the colors of the log labels
 *
 * @param[in] index Index of the log row to be configured
 * @param[in] color_mode The color mode to be configured for the given log row
 */
static void setColor_LogLabel(ls_log_label_id_t index, ls_color_mode_t color_mode)
{
	lv_style_t * color_style;
	color_style = &gs_color_style_log[index];

	if (color_style->prop_cnt <= 1)
	{
		return;
	}
	else
	{
		lv_style_reset(color_style);
	}

	switch(color_mode)
	{
	case kLST_Color_Operational:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OPERATIONAL));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Maintenance:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_MAINTENANCE));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Fault:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_FAULT));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Error:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_ERROR));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_LIGHT));
		break;

	case kLST_Color_Off:
		lv_style_set_bg_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_bg_grad_color(color_style, lv_color_make(COLOR_OFF));
		lv_style_set_text_color(color_style, lv_color_make(COLOR_MAIN_DARK));
		break;

	default:
		logInvalidArgument();
		return;
	}

	switch(index)
	{
	case kLST_Log_Label1:
		lv_obj_refresh_style(gs_guider_ui.screen_label_log_1, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	case kLST_Log_Label2:
		lv_obj_refresh_style(gs_guider_ui.screen_label_log_2, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	case kLST_Log_Label3:
		lv_obj_refresh_style(gs_guider_ui.screen_label_log_3, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_BG_COLOR | LV_STYLE_BG_GRAD_COLOR | LV_STYLE_TEXT_COLOR);
		break;

	default:
		logInvalidArgument();
		return;
	}
}

/*!
 * @brief This function sets the text of the log labels
 *
 * @param[in] index Index of the log row to be configured
 * @param[in] message The log message to be displayed
 * @param[in] timestamp_string The timestamp of the log message
 */
static void setText_LogLabel(ls_log_label_id_t index, const char *message, const char *timestamp_string)
{
	switch(index)
	{
	case kLST_Log_Label1:
		lv_label_set_text(gs_guider_ui.screen_label_log_1, message);
		lv_label_set_text(gs_guider_ui.screen_label_log_datetime_1, timestamp_string);
		break;

	case kLST_Log_Label2:
		lv_label_set_text(gs_guider_ui.screen_label_log_2, message);
		lv_label_set_text(gs_guider_ui.screen_label_log_datetime_2, timestamp_string);
		break;

	case kLST_Log_Label3:
		lv_label_set_text(gs_guider_ui.screen_label_log_3, message);
		lv_label_set_text(gs_guider_ui.screen_label_log_datetime_3, timestamp_string);
		break;

	default:
		logInvalidArgument();
		return;
	}
}

/*!
 * @brief This function updates the log labels based on the array of last logs
 *
 * @param[in] lastLogs Pointer to an array of last logs
 * @param[in] activeLogLabels Amount of already active log rows
 */
static void update_LogLabels(log_record_t *lastLogs, unsigned int activeLogLabels)
{
	qmc_timestamp_t timestamp;
	qmc_datetime_t datetime;
	qmc_status_t qmcStatus;
	char message[GUI_MAX_MESSAGE_LENGTH] = {0};
	char timestamp_string[GUI_MAX_TIMESTAMP_LENGTH] = {0};
	int checkResultMessage = 0;
	int checkResultTimestamp = 0;

	if (activeLogLabels > MAX_LOG_LABELS)
	{
		activeLogLabels = MAX_LOG_LABELS;
	}

	for (int i = 0; i < activeLogLabels; i++)
	{
		switch(lastLogs[i].type)
		{
		case kLOG_DefaultData:
			switch (lastLogs[i].data.defaultData.eventCode)
			{
			case LOG_EVENT_LidOpenSd:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "SD lid was opened.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_LidOpenButton:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Button lid was opened.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_EmergencyButtonPressed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Emergency button was pressed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_Button1Pressed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Button1 was pressed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_Button2Pressed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Button2 was pressed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_Button3Pressed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Button3 was pressed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_Button4Pressed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Button4 was pressed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_TamperingButton:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "BUTTON TAMPERING EVENT!");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_TamperingSd:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "SD TAMPERING EVENT!");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_AccountResumed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Account resumed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			case LOG_EVENT_AccountSuspended:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Account suspended.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_LoginFailure:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Login failure.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_UserLogin:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "User login.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			case LOG_EVENT_UserLogout:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "User logout.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			case LOG_EVENT_InvalidArgument:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Invalid arguments used.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			default:
				logInvalidArgument();
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;
			}
			break;

		case kLOG_FaultDataWithID:
			if ((lastLogs[i].data.faultDataWithID.eventCode == LOG_EVENT_NoFaultMC) ||\
				(lastLogs[i].data.faultDataWithID.eventCode == LOG_EVENT_NoFaultBS))
			{
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Operational);
			}
			else
			{
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Fault);
			}

			switch (lastLogs[i].data.faultDataWithID.eventCode)
			{
			case LOG_EVENT_AfePsbCommunicationError:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d AfePsbCommunicationError", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_GD3000_Desaturation:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d GD3000_Desaturation", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_GD3000_LowVLS:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d GD3000_LowVLS", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_GD3000_OverCurrent:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d GD3000_OverCurrent", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_GD3000_OverTemperature:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d GD3000_OverTemperature", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_GD3000_PhaseError:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d GD3000_PhaseError", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_GD3000_Reset:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d GD3000_Reset", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_NoFaultBS:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d NoFaultBS", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_NoFaultMC:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d NoFaultMC", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_OverCurrent:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d OverCurrent", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_OverDcBusVoltage:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d OverDcBusVoltage", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_OverLoad:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d OverLoad", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_OverSpeed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d OverSpeed", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_PsbOverTemperature1:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d PsbOverTemperature1", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_PsbOverTemperature2:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d PsbOverTemperature2", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_RotorBlocked:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d RotorBlocked", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_UnderDcBusVoltage:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d UnderDcBusVoltage", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_QueueingCommandFailedInternal:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d SetMotorCommand failed", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_QueueingCommandFailedTSN:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Motor %d TSN MotorCommand failed", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			case LOG_EVENT_PmicUnderVoltage:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Pmic %d UnderVoltage", lastLogs[i].data.faultDataWithID.id + 1);
				break;

			default:
				logInvalidArgument();
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;
			}
			break;

		case kLOG_FaultDataWithoutID:
			if (lastLogs[i].data.faultDataWithoutID.eventCode == LOG_EVENT_NoFault)
			{
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Operational);
			}
			else
			{
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Fault);
			}

			switch (lastLogs[i].data.faultDataWithoutID.eventCode)
			{
			case LOG_EVENT_AfeDbCommunicationError:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "AfeDbCommunicationError");
				break;

			case LOG_EVENT_DBTempSensCommunicationError:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "DBTempSensCommunicationError");
				break;

			case LOG_EVENT_DbOverTemperature:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "DbOverTemperature");
				break;

			case LOG_EVENT_EmergencyStop:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "EmergencyStop");
				break;

			case LOG_EVENT_FaultBufferOverflow:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "FaultBufferOverflow");
				break;

			case LOG_EVENT_FaultQueueOverflow:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "FaultQueueOverflow");
				break;

			case LOG_EVENT_InvalidFaultSource:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "InvalidFaultSource");
				break;

			case LOG_EVENT_McuOverTemperature:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "McuOverTemperature");
				break;

			case LOG_EVENT_NoFault:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "NoFault");
				break;

			case LOG_EVENT_PmicOverTemperature:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "PmicOverTemperature");
				break;

			case LOG_EVENT_QueueingCommandFailedQueue:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "QueueMotorCommand failed.");
				break;

			case LOG_EVENT_RPCCallFailed:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "RPC call failed.");
				break;

			case LOG_EVENT_FunctionalWatchdogInitFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Functional Watchdog Init Failed.");
				break;

			case LOG_EVENT_Scp03ConnFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Scp03 Conn Failed");
				break;

			case LOG_EVENT_Scp03KeyReconFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Scp03 Key Recon Failed");
				break;

			case LOG_EVENT_NewFWRevertFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "New FW Revert Failed");
				break;

			case LOG_EVENT_NewFWCommitFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "New FW Commit Failed");
				break;

			case LOG_EVENT_AwdtExpired:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Awdt Expired");
				break;

			case LOG_EVENT_CfgDataBackUpFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Cfg Data Back Up Failed");
				break;

			case LOG_EVENT_MainFwAuthFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Main FW Auth Failed");
				break;

			case LOG_EVENT_FwuAuthFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "FWU Auth Failed");
				break;

			case LOG_EVENT_StackError:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Stack Error");
				break;

			case LOG_EVENT_InvalidFwuVersion:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Invalid FWU Version");
				break;

			case LOG_EVENT_ExtMemOprFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Ext Mem Opr Failed");
				break;

			case LOG_EVENT_BackUpImgAuthFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Back Up Img Auth Failed");
				break;

			case LOG_EVENT_SdCardFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "SD Card Failed");
				break;

			case LOG_EVENT_HwInitDeinitFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "HW Init Deinit Failed");
				break;

			case LOG_EVENT_SvnsLpGprOpFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "SNVS LP GPR Op Failed");
				break;

			case LOG_EVENT_Scp03KeyRotationFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Scp03 Key Rotation Failed");
				break;

			case LOG_EVENT_DecommissioningFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Decommissioning Failed");
				break;

			case LOG_EVENT_VerReadFromSeFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Ver Read From SE Failed");
				break;

			case LOG_EVENT_FwExecutionFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "FW Execution Failed");
				break;

			case LOG_EVENT_FwuCommitFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "FWU Commit Failed");
				break;

			case LOG_EVENT_RpcInitFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "RPC Init Failed");
				break;

			case LOG_EVENT_UnknownFWReturnStatus:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Unknown FW Return Status");
				break;

			default:
				logInvalidArgument();
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;
			}
			break;

		case kLOG_SystemData:
			switch (lastLogs[i].data.systemData.eventCode)
			{
			case LOG_EVENT_ResetRequest:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Watchdog: Reset Request");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			case LOG_EVENT_InvalidResetCause:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Invalid reset cause was used.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_InvalidArgument:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Invalid arguments used.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			case LOG_EVENT_AWDTExpired:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "AWDT Expired.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			case LOG_EVENT_RPCCallFailed:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "RPC call failed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Fault);
				break;

			case LOG_EVENT_SignatureInvalid:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Signature invalid.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_Timeout:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Timeout.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_SyncError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Synchronization failed.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_InternalError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Internal error.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_NoBufsError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Buffer too small.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_ConnectionError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Server connection failed.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_RequestError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Server request failed.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_JsonParsingError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "JSON parsing failed.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_RangeError:
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "Out of range.");
				setColor_LogLabel((ls_log_label_id_t)i, kLST_Color_Error);
				break;

			case LOG_EVENT_ResetSecureWatchdog:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Secure Watchdog Reset.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_ResetFunctionalWatchdog:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Functional Watchdog Reset.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_FunctionalWatchdogKickFailed:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Functional Watchdog Kick Failed.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_PowerLoss:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Power lost, system restarted.");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_DeviceDecommissioned:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Device Decommissioned");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_CfgDataBackedUp:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Cfg Data Backed Up");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_NewFWReverted:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "New FW Reverted");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_NewFWCommitted:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "New FW Committed");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
				break;

			case LOG_EVENT_KeyRevocation:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Key Revocation");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Error);
				break;

			case LOG_EVENT_NoLogEntry:
				checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "No Log Entry");
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;

			default:
				logInvalidArgument();
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
				break;
			}
			break;

		case kLOG_ErrorCount:
			if (lastLogs[i].data.errorCount.source == LOG_SRC_Webservice)
			{
				checkResultMessage = snprintf(message, GUI_MAX_MESSAGE_LENGTH, "[%u] %s.",
						lastLogs[i].data.errorCount.count,
						status_code_string(lastLogs[i].data.errorCount.errorCode)
						);
			}
			else
			{
				logInvalidArgument();
			}
			setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
			break;

		case kLOG_UsrMgmt:
			if (lastLogs[i].data.usrMgmt.source == LOG_SRC_UsrMgmt)
			{
				switch (lastLogs[i].data.usrMgmt.eventCode)
				{
					case LOG_EVENT_SessionTimeout:
						checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Session timeout.");
						setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
						break;
					case LOG_EVENT_UserCreated:
						checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "New user account created.");
						setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
						break;
					case LOG_EVENT_UserUpdate:
						checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "User account updated.");
						setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
						break;
					case LOG_EVENT_UserRemoved:
						checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "User account removed.");
						setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
						break;
					case LOG_EVENT_TerminateSession:
						checkResultMessage = snprintf(message , GUI_MAX_MESSAGE_LENGTH, "Terminated user session.");
						setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Maintenance);
						break;
					default:
						logInvalidArgument();
						setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
						break;
				}
			}
			else
			{
				logInvalidArgument();
				setColor_LogLabel((ls_log_label_id_t) i, kLST_Color_Off);
			}
			break;
		}

		timestamp.milliseconds = lastLogs[i].rhead.ts.milliseconds;
		timestamp.seconds = lastLogs[i].rhead.ts.seconds;

		qmcStatus = BOARD_ConvertTimestamp2Datetime(&timestamp, &datetime);
		if (qmcStatus == kStatus_QMC_Ok)
		{
			checkResultTimestamp = snprintf(timestamp_string, 20, "%02d:%02d:%02d %02d/%02d/%d", datetime.hour, datetime.minute, datetime.second,\
					datetime.month, datetime.day, datetime.year);
		}
		else
		{
			checkResultTimestamp = snprintf(timestamp_string, 20, "00:00:00 00/00/00");
		}


		if (checkResultMessage < 0 || checkResultMessage >= sizeof(message))
		{
			if (checkResultTimestamp < 0 || checkResultTimestamp >= sizeof(timestamp_string))
			{
				setText_LogLabel((ls_log_label_id_t) i, "err", "err");
			}
			else
			{
				setText_LogLabel((ls_log_label_id_t) i, "err", timestamp_string);
			}
		}
		else
		{
			if (checkResultTimestamp < 0 || checkResultTimestamp >= sizeof(timestamp_string))
			{
				setText_LogLabel((ls_log_label_id_t) i, message, "err");
			}
			else
			{
				setText_LogLabel((ls_log_label_id_t) i, message, timestamp_string);
			}
		}
	}
}

/*!
 * @brief This function updates the specified icon's style
 *
 * @param[in] icon The icon to be updated
 * @param[in] opacity Opacity settings [0-255]
 */
static void updateIconOpacity(lv_obj_t *icon, uint8_t opacity)
{
	lv_style_set_img_opa(icon->styles[icon->style_cnt - 1].style, opacity);
	lv_obj_refresh_style(icon, LV_PART_MAIN|LV_STATE_DEFAULT, LV_STYLE_OPA);
}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
/*!
 * @brief This function starts or stops a motors based it's current state
 *
 * @param[in] motorId Id of the motor that should be started or stopped
 */
static void motorStartStop(mc_motor_id_t motorId)
{
	if (IS_MOTORID_INVALID(motorId))
	{
		log_record_t logEntryError = {0};
		logEntryError.type = kLOG_FaultDataWithoutID;
		logEntryError.data.faultDataWithoutID.source = LOG_SRC_LocalService;
		logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
		logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_InvalidArgument;
		LOG_QueueLogEntry(&logEntryError, true);

		BOARD_SetLifecycle(kQMC_LcError);
		return;
	}

	mc_motor_command_t cmd = {0};
	cmd.eMotorId = motorId;

	if(gs_isRunning[motorId])
	{
		cmd.eAppSwitch = kMC_App_Off;
		gs_isRunning[motorId] = false;
	}
	else
	{
		cmd.eAppSwitch = kMC_App_On;
		cmd.eControlMethodSel = kMC_FOC_SpeedControl;
		cmd.uSpeed_pos.fltSpeed = DEFAULT_MOTOR_SPEED;
		gs_isRunning[motorId] = true;
	}

	int motorQueueTimeout = MOTOR_QUEUE_TIMEOUT_ATTEMPTS;

	do
	{
		qmc_status_t status = MC_QueueMotorCommand(&cmd);

		if ((status == kStatus_QMC_Ok) || (status == kStatus_QMC_ErrBusy))
		{
			break;
		}

		motorQueueTimeout--;
		vTaskDelay(10);
	} while (motorQueueTimeout > 0);

	/* Timed out */
	if (motorQueueTimeout <= 0)
	{
		log_record_t logEntryError = {0};
		logEntryError.type = kLOG_FaultDataWithoutID;
		logEntryError.data.faultDataWithoutID.source = LOG_SRC_LocalService;
		logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
		logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_QueueingCommandFailedQueue;
		LOG_QueueLogEntry(&logEntryError, true);

		BOARD_SetLifecycle(kQMC_LcError);
	}
}
#endif

/*!
 * @brief This function handles opening or closing of the button and SD lids
 *
 * @param[in] inputButtonEvent Input event that determines if the SD or the button lid was opened or closed
 */
static void lidOpenClose(uint32_t inputButtonEvent)
{
	log_record_t logEntry = {0};
	if (inputButtonEvent & QMC_IOEVENT_LID_OPEN_SD)
	{
		if (!gs_wasSetLidSd)
		{
			logEntry.type = kLOG_DefaultData;
			logEntry.data.defaultData.source = LOG_SRC_LocalService;
			logEntry.data.defaultData.category = LOG_CAT_General;
			logEntry.data.defaultData.eventCode = LOG_EVENT_LidOpenSd;
			logEntry.data.defaultData.user = kUSRMGMT_RoleLocalSd;
			LOG_QueueLogEntry(&logEntry, true);

			gs_wasSetLidSd = true;
		}
	}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS
	if (inputButtonEvent & QMC_IOEVENT_LID_OPEN_BUTTON)
	{
		if (!gs_wasSetLidButton)
		{
			logEntry.type = kLOG_DefaultData;
			logEntry.data.defaultData.source = LOG_SRC_LocalService;
			logEntry.data.defaultData.category = LOG_CAT_General;
			logEntry.data.defaultData.eventCode = LOG_EVENT_LidOpenButton;
			logEntry.data.defaultData.user = kUSRMGMT_RoleLocalButton;
			LOG_QueueLogEntry(&logEntry, true);

			gs_wasSetLidButton = true;
		}
	}
#endif
}

#if FEATURE_HANDLE_BUTTON_PRESS_EVENTS

/*!
 * @brief This function handles pressing of the emergency button
 *
 */
static void emergencyPressed()
{
	if (gs_wasSetEmergency)
	{
		return;
	}

	for (mc_motor_id_t stopMotorId = kMC_Motor1; stopMotorId < MC_MAX_MOTORS; stopMotorId++)
	{
		mc_motor_command_t cmd = {0};
		cmd.eMotorId = stopMotorId;
		cmd.eAppSwitch = kMC_App_FreezeAndStop;

		int motorQueueTimeout = MOTOR_QUEUE_TIMEOUT_ATTEMPTS;
		do
		{
			qmc_status_t status = MC_QueueMotorCommand(&cmd);
			if ((status == kStatus_QMC_Ok) || (status == kStatus_QMC_ErrBusy))
			{
				break;
			}

			motorQueueTimeout--;
			vTaskDelay(10);
		} while (motorQueueTimeout > 0);

		/* Timed out */
		if (motorQueueTimeout <= 0)
		{
			log_record_t logEntryError = {0};
			logEntryError.type = kLOG_FaultDataWithoutID;
			logEntryError.data.faultDataWithoutID.source = LOG_SRC_LocalService;
			logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
			logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_QueueingCommandFailedQueue;
			LOG_QueueLogEntry(&logEntryError, true);

			BOARD_SetLifecycle(kQMC_LcError);
			break;
		}
	}

	log_record_t logEntry = {0};
	logEntry.type = kLOG_DefaultData;
	logEntry.data.defaultData.source = LOG_SRC_LocalService;
	logEntry.data.defaultData.category = LOG_CAT_General;
	logEntry.data.defaultData.eventCode = LOG_EVENT_EmergencyButtonPressed;
	logEntry.data.defaultData.user = kUSRMGMT_RoleLocalEmergency;
	LOG_QueueLogEntry(&logEntry, true);

	gs_wasSetEmergency = true;
}
#endif

/*!
 * @brief This function handles a tampering event
 *
 * @param[in] type The type of the tampering event
 */
static void tamperingDetected(ls_tampering_type_t type)
{
	BOARD_SetLifecycle(kQMC_LcError);

	log_record_t logEntry = {0};
	logEntry.type = kLOG_DefaultData;
	logEntry.data.defaultData.source = LOG_SRC_LocalService;
	logEntry.data.defaultData.category = LOG_CAT_General;

	switch (type)
	{
	case kLST_SdTamperingType:
		logEntry.data.defaultData.user = kUSRMGMT_RoleLocalSd;
		logEntry.data.defaultData.eventCode = LOG_EVENT_TamperingSd;
		break;

	case kLST_ButtonTamperingType:
		logEntry.data.defaultData.user = kUSRMGMT_RoleLocalButton;
		logEntry.data.defaultData.eventCode = LOG_EVENT_TamperingButton;
		break;

	default:
		logInvalidArgument();
		return;
	}

	LOG_QueueLogEntry(&logEntry, true);

	for (mc_motor_id_t stopMotorId = kMC_Motor1; stopMotorId < MC_MAX_MOTORS; stopMotorId++)
	{
		mc_motor_command_t cmd = {0};
		cmd.eMotorId = stopMotorId;
		cmd.eAppSwitch = kMC_App_FreezeAndStop;

		int motorQueueTimeout = MOTOR_QUEUE_TIMEOUT_ATTEMPTS;
		do
		{
			qmc_status_t status = MC_QueueMotorCommand(&cmd);
			if ((status == kStatus_QMC_Ok) || (status == kStatus_QMC_ErrBusy))
			{
				break;
			}

			motorQueueTimeout--;
			vTaskDelay(10);
		} while (motorQueueTimeout > 0);

		/* Timed out */
		if (motorQueueTimeout <= 0)
		{
			log_record_t logEntryError = {0};
			logEntryError.type = kLOG_FaultDataWithoutID;
			logEntryError.data.faultDataWithoutID.source = LOG_SRC_LocalService;
			logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
			logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_QueueingCommandFailedQueue;
			LOG_QueueLogEntry(&logEntryError, true);

			/* Lifecycle Error already set*/
			break;
		}
	}

	gs_tamperingReported = true;
}

/*!
 * @brief FreeRTOS application tick hook
 *
 */
void vApplicationTickHook(void)
{
    if (gs_lvgl_initialized)
    {
        lv_tick_inc(1);
    }
}

/*!
 * @brief This logs that an invalid argument was received
 */
static void logInvalidArgument()
{
	log_record_t logEntry = {0};
	logEntry.type = kLOG_DefaultData;
	logEntry.data.defaultData.source = LOG_SRC_LocalService;
	logEntry.data.defaultData.category = LOG_CAT_General;
	logEntry.data.defaultData.eventCode = LOG_EVENT_InvalidArgument;
	logEntry.data.defaultData.user = kUSRMGMT_RoleNone;
	LOG_QueueLogEntry(&logEntry, true);
}

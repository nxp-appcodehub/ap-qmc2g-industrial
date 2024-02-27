/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "api_motorcontrol.h"
#include "api_motorcontrol_internal.h"
#include "qmc_features_config.h"
#include "mc_common.h"
#include "semphr.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define WRITE_PARAMETER_WITH_CHECK(x, y, f) { \
    (x) = (y);                                \
	if(false == (f))                          \
		return kStatus_QMC_ErrInterrupted;    \
}

#define IS_MOTORID_INVALID(x) \
		(((x) < kMC_Motor1) || ((x) >= (MC_MAX_MOTORS)))



/*******************************************************************************
 * Variables
 ******************************************************************************/

extern bool                   g_isInitialized_DataHub; /* indicates that the DataHub task initialized the message queues for motor commands and status values */
extern uint32_t               g_motorStatusQueuePrescalers[DATAHUB_MAX_STATUS_QUEUES];
extern uint32_t               g_motorStatusQueuePrescalerCounters[DATAHUB_MAX_STATUS_QUEUES];
extern QueueHandle_t          g_motorCommandQueue;
extern QueueHandle_t          g_motorStatusQueues[DATAHUB_MAX_STATUS_QUEUES];
extern qmc_msg_queue_handle_t g_motorStatusQueueHandles[DATAHUB_MAX_STATUS_QUEUES];
extern const EventBits_t      g_motorCommandQueueEventBit;
extern EventGroupHandle_t     g_motorQueueEventGroupHandle;
extern TimerHandle_t          g_statusSamplingTimerHandle;
extern SemaphoreHandle_t      g_statusQueueMutexHandle;
#if FEATURE_MC_PSB_TEMPERATURE_FAULTS
extern mc_fault_t             g_psbFaults[MC_MAX_MOTORS];
#endif

/* Shared memory to interact with the motor control loops.

   Motor control loops do not use eMotorId; the motor is rather referenced by the array index.
   eMotorId will get adjusted by the DataHub when queuing / de-queuing items.
*/
mc_control_shm g_p = {
		.bIsTsnCommandInjectionOn	 = { 0 },
		.ui16MotorFaultConfiguration = 0,
		.sCommands                   = { {kMC_Motor1, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#if (MC_MAX_MOTORS > 1)
										 {kMC_Motor2, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#endif
#if (MC_MAX_MOTORS > 2)
										 {kMC_Motor3, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#endif
#if (MC_MAX_MOTORS > 3)
										 {kMC_Motor4, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#endif
										},
		.sCommands_int               = { {kMC_Motor1, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#if (MC_MAX_MOTORS > 1)
										 {kMC_Motor2, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#endif
#if (MC_MAX_MOTORS > 2)
										 {kMC_Motor3, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#endif
#if (MC_MAX_MOTORS > 3)
										 {kMC_Motor4, kMC_App_Off, kMC_FOC_SpeedControl, {.fltSpeed = 0.0f}},
#endif
		},
		.bWriteInProgress            = { 0 },
		.bReadInProgress_fast        = { 0 },
		.bReadInProgress_slow        = { 0 },
		.bIsFrozen					 = { 0 },
		.sStatus                     = { {{kMC_Init, kMC_NoFaultMC, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {kMC_App_Off, 0.0f, {.i32Raw = 0}}, kMC_Motor1},
#if (MC_MAX_MOTORS > 1)
										 {{kMC_Init, kMC_NoFaultMC, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {kMC_App_Off, 0.0f, {.i32Raw = 0}}, kMC_Motor2},
#endif
#if (MC_MAX_MOTORS > 2)
										 {{kMC_Init, kMC_NoFaultMC, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {kMC_App_Off, 0.0f, {.i32Raw = 0}}, kMC_Motor3},
#endif
#if (MC_MAX_MOTORS > 3)
										 {{kMC_Init, kMC_NoFaultMC, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {kMC_App_Off, 0.0f, {.i32Raw = 0}}, kMC_Motor4},
#endif
		}
};

static bool gs_isStatusSamplingTimerStarted = false;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static bool isMotorCommandInRange(const mc_motor_command_t* cmd);



/*******************************************************************************
 * Code
 ******************************************************************************/

qmc_status_t MC_SetMotorCommand(const mc_motor_command_t* cmd)
{
	/* input sanitation and limit checks */
	if(NULL == cmd)
		return kStatus_QMC_ErrArgInvalid;
	if(IS_MOTORID_INVALID(cmd->eMotorId))
		return kStatus_QMC_ErrRange;

	if (g_p.bIsFrozen[cmd->eMotorId])
	{
		return kStatus_QMC_ErrBusy;
	}
	else if (cmd->eAppSwitch == kMC_App_Freeze)
	{
		 g_p.bIsFrozen[cmd->eMotorId] = true;
	}
	else if (cmd->eAppSwitch == kMC_App_FreezeAndStop)
	{
		g_p.bIsFrozen[cmd->eMotorId] = true;

		/* lock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = true;

		/* copy parameters */
		WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].eAppSwitch, kMC_App_Off, g_p.bWriteInProgress[cmd->eMotorId]);

		/* unlock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = false;
	}
	else
	{
		/* lock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = true;

		/* copy parameters */
		WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].eAppSwitch, cmd->eAppSwitch, g_p.bWriteInProgress[cmd->eMotorId]);
		WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].eControlMethodSel, cmd->eControlMethodSel, g_p.bWriteInProgress[cmd->eMotorId]);
		switch(cmd->eControlMethodSel){
		case kMC_ScalarControl:
			WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].uSpeed_pos.sScalarParam.fltScalarControlFrequency, cmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency, g_p.bWriteInProgress[cmd->eMotorId]);
			WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].uSpeed_pos.sScalarParam.fltScalarControlVHzGain, cmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain, g_p.bWriteInProgress[cmd->eMotorId]);
			break;
		case kMC_FOC_SpeedControl:
			WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].uSpeed_pos.fltSpeed, cmd->uSpeed_pos.fltSpeed, g_p.bWriteInProgress[cmd->eMotorId]);
			break;
		case kMC_FOC_PositionControl:
			WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].uSpeed_pos.sPosParam.uPosition.i32Raw, cmd->uSpeed_pos.sPosParam.uPosition.i32Raw, g_p.bWriteInProgress[cmd->eMotorId]);
			WRITE_PARAMETER_WITH_CHECK(g_p.sCommands[cmd->eMotorId].uSpeed_pos.sPosParam.bIsRandomPosition, cmd->uSpeed_pos.sPosParam.bIsRandomPosition, g_p.bWriteInProgress[cmd->eMotorId]);
			break;
		default:
			g_p.bWriteInProgress[cmd->eMotorId] = false; /* unlock memory */
			return kStatus_QMC_ErrRange;
		}

		/* unlock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = false;
	}

	return kStatus_QMC_Ok;
}

void MC_GetMotorStatus(mc_motor_id_t motorId, mc_motor_status_t* status)
{
	/* input sanitation and limit checks */
	if(NULL == status)
		return;
	if(IS_MOTORID_INVALID(motorId))
		return;

	/* lock memory */
	g_p.bReadInProgress_slow[motorId] = true;
	g_p.bReadInProgress_fast[motorId] = true;

	/* copy status values */
	*status = g_p.sStatus[motorId];

	/* unlock memory */
	g_p.bReadInProgress_fast[motorId] = false;
	g_p.bReadInProgress_slow[motorId] = false;
}

RAM_FUNC_CRITICAL void MC_SetFastMotorStatus_fromISR(mc_motor_id_t motorId, const mc_motor_status_fast_t* status)
{
	/* input sanitation and limit checks */
	if(NULL == status)
		return;
	if(IS_MOTORID_INVALID(motorId))
		return;

    #if (FEATURE_ANOMALY_DETECTION != 0)
	{
	    static unsigned int counter[MC_MAX_MOTORS] = {0};

	    if(g_p.sSyncStatus[motorId] == &(g_p.sSyncStatusA[motorId][0])){
	    	g_p.sSyncStatusB[motorId][counter[motorId]] = *status;
            #if FEATURE_MC_PSB_TEMPERATURE_FAULTS
	    	    g_p.sSyncStatusB[motorId][counter[motorId]].eFaultStatus |= g_psbFaults[motorId] & (kMC_PsbOverTemperature1 | kMC_PsbOverTemperature2 | kMC_GD3000_OverTemperature | kMC_GD3000_Desaturation | kMC_GD3000_LowVLS | kMC_GD3000_OverCurrent |	kMC_GD3000_PhaseError |	kMC_GD3000_Reset);
            #endif
	    	counter[motorId]++;
	    	if(AD_CURRENT_BLK_SIZE == counter[motorId])
	    	{
	    		g_p.sSyncStatus[motorId] = &(g_p.sSyncStatusB[motorId][0]);
	    		counter[motorId] = 0;
	    	}
	    } else {
	    	g_p.sSyncStatusA[motorId][counter[motorId]] = *status;
            #if FEATURE_MC_PSB_TEMPERATURE_FAULTS
                g_p.sSyncStatusA[motorId][counter[motorId]].eFaultStatus |= g_psbFaults[motorId] & (kMC_PsbOverTemperature1 | kMC_PsbOverTemperature2 | kMC_GD3000_OverTemperature | kMC_GD3000_Desaturation | kMC_GD3000_LowVLS | kMC_GD3000_OverCurrent |	kMC_GD3000_PhaseError |	kMC_GD3000_Reset);
            #endif
            counter[motorId]++;
	    	if(AD_CURRENT_BLK_SIZE == counter[motorId])
	    	{
	    		g_p.sSyncStatus[motorId] = &(g_p.sSyncStatusA[motorId][0]);
	    		counter[motorId] = 0;
	    	}
	    }
	}
    #endif

	/* copy status values */
	if(!g_p.bReadInProgress_fast[motorId])
	{
	    g_p.sStatus[motorId].sFast = *status;
        #if FEATURE_MC_PSB_TEMPERATURE_FAULTS
            g_p.sStatus[motorId].sFast.eFaultStatus |= g_psbFaults[motorId] & (kMC_PsbOverTemperature1 | kMC_PsbOverTemperature2 | kMC_GD3000_OverTemperature | kMC_GD3000_Desaturation | kMC_GD3000_LowVLS | kMC_GD3000_OverCurrent |	kMC_GD3000_PhaseError |	kMC_GD3000_Reset);
        #endif
	}
	return;
}

RAM_FUNC_CRITICAL void MC_SetSlowMotorStatus_fromISR(mc_motor_id_t motorId, const mc_motor_status_slow_t* status)
{
	/* input sanitation and limit checks */
	if(NULL == status)
		return;
	if(IS_MOTORID_INVALID(motorId))
		return;

	/* copy status values */
	if(!g_p.bReadInProgress_slow[motorId])
	    g_p.sStatus[motorId].sSlow = *status;

	return;
}

RAM_FUNC_CRITICAL mc_motor_command_t* MC_GetMotorCommand_fromISR(mc_motor_id_t motorId)
{
	/* input sanitation and limit checks */
	if(IS_MOTORID_INVALID(motorId))
		return NULL;

	/* update internal command buffer */
	if(!g_p.bWriteInProgress[motorId])
		g_p.sCommands_int[motorId] = g_p.sCommands[motorId];

	/* Cast removes "volatile" modifier. The calling motor control loop
	 * cannot be interrupted by any code modifying this memory location */
	return (mc_motor_command_t*)&(g_p.sCommands_int[motorId]);
}

#if (FEATURE_ANOMALY_DETECTION != 0)
qmc_status_t MC_GetFastMotorStatusSync_fromISR(mc_motor_id_t motorId, mc_motor_status_fast_t** status)
{
	static mc_motor_status_fast_t *oldSyncStatus[MC_MAX_MOTORS];

	/* input sanitation and limit checks */
	if(NULL == status)
		return kStatus_QMC_ErrArgInvalid;
	if(IS_MOTORID_INVALID(motorId))
		return kStatus_QMC_ErrRange;

	/* Double buffering synchronicity check */
    #if (FEATURE_ANOMALY_DETECTION_SYNC_CHECK != 0)
	    if(oldSyncStatus[motorId] == g_p.sSyncStatus[motorId])
		    return kStatus_QMC_ErrSync;
    #endif

    /* Cast removes "volatile" modifier. The motor control loop will update the other buffer */
	oldSyncStatus[motorId] = (mc_motor_status_fast_t*) g_p.sSyncStatus[motorId];
	memcpy(*status, oldSyncStatus[motorId], (AD_CURRENT_BLK_SIZE * sizeof(mc_motor_status_fast_t)));
	return kStatus_QMC_Ok;
}
#endif

qmc_status_t MC_QueueMotorCommand(const mc_motor_command_t* cmd)
{
	BaseType_t retval;

	/* input sanitation and limit checks */
	if(NULL == cmd)
		return kStatus_QMC_ErrArgInvalid;
	if(!isMotorCommandInRange(cmd))
		return kStatus_QMC_ErrArgInvalid;
	if(!g_isInitialized_DataHub)
		return kStatus_QMC_Err;

	if (g_p.bIsFrozen[cmd->eMotorId])
	{
		return kStatus_QMC_ErrBusy;
	}

	if ((cmd->eAppSwitch == kMC_App_Freeze) || (cmd->eAppSwitch == kMC_App_FreezeAndStop))
	{
		MC_SetTsnCommandInjectionById(false, cmd->eMotorId); /* prevent TSN commands for this motor */
	}

	/* queue motor command item (xQueueSendToBack copies the value) */
	retval = xQueueSendToBack(g_motorCommandQueue, cmd, 0);
	if(pdTRUE != retval)
		return kStatus_QMC_Err;

	/* trigger corresponding new-message-event */
	xEventGroupSetBits(g_motorQueueEventGroupHandle, g_motorCommandQueueEventBit);

	return kStatus_QMC_Ok;
}

qmc_status_t MC_DequeueMotorStatus(const qmc_msg_queue_handle_t* handle, uint32_t timeout, mc_motor_status_t* status)
{
	BaseType_t retval;

	/* input sanitation and limit checks */
	if((NULL == handle) || (NULL == handle->queueHandle))
		return kStatus_QMC_ErrArgInvalid;
	if(NULL == status)
		return kStatus_QMC_ErrArgInvalid;
	if(!g_isInitialized_DataHub)
		return kStatus_QMC_Err;

	/* dequeue status item (xQueueReceive copies the value) */
	retval = xQueueReceive(*(handle->queueHandle), status, pdMS_TO_TICKS(timeout));
	if(pdTRUE != retval)
		return kStatus_QMC_ErrNoMsg;

	/* clear event if all elements have been dequeued */
	if( 0 == uxQueueMessagesWaiting(*(handle->queueHandle)))
		xEventGroupClearBits(*(handle->eventHandle), handle->eventMask);

	return kStatus_QMC_Ok;
}

void MC_SetTsnCommandInjection(bool isOn)
{
	taskENTER_CRITICAL();

	for (mc_motor_id_t motorId = kMC_Motor1; motorId < MC_MAX_MOTORS; motorId++)
	{
		g_p.bIsTsnCommandInjectionOn[motorId] = isOn;
	}

	taskEXIT_CRITICAL();
}

qmc_status_t MC_SetTsnCommandInjectionById(bool isOn, mc_motor_id_t motorId)
{
	if (IS_MOTORID_INVALID(motorId))
	{
		return kStatus_QMC_ErrArgInvalid;
	}
	else
	{
		g_p.bIsTsnCommandInjectionOn[motorId] = isOn;
		return kStatus_QMC_Ok;
	}
}

qmc_status_t MC_GetNewStatusQueueHandle(qmc_msg_queue_handle_t** handle, uint32_t prescaler)
{
	unsigned int i;

	/* input sanitation and limit checks */
	if(NULL == handle)
		return kStatus_QMC_ErrArgInvalid;
	if(0 == prescaler)
		return kStatus_QMC_ErrArgInvalid;
	if(!g_isInitialized_DataHub)
		return kStatus_QMC_Err;

	if(pdTRUE != xSemaphoreTake(g_statusQueueMutexHandle, portMAX_DELAY))
		return kStatus_QMC_Err;
	for(i=0; i<DATAHUB_MAX_STATUS_QUEUES; i++)
	{
		if(NULL == g_motorStatusQueueHandles[i].queueHandle)
		{
			g_motorStatusQueueHandles[i].queueHandle = &(g_motorStatusQueues[i]);
			g_motorStatusQueuePrescalers[i]          = prescaler;
			g_motorStatusQueuePrescalerCounters[i]   = prescaler;
			*handle                                  = &(g_motorStatusQueueHandles[i]);
			if(!gs_isStatusSamplingTimerStarted) /* check required as starting an already running timer is equivalent to a reset */
			{
				xTimerStart(g_statusSamplingTimerHandle, 0);
				gs_isStatusSamplingTimerStarted = true;
			}
			xSemaphoreGive(g_statusQueueMutexHandle);
			return kStatus_QMC_Ok;
		}
	}
	xSemaphoreGive(g_statusQueueMutexHandle);
	return kStatus_QMC_ErrMem; /* No free queue handle available */
}

qmc_status_t MC_ReturnStatusQueueHandle(const qmc_msg_queue_handle_t* handle)
{
	unsigned int i;
	bool         areAllHandlesReturned = true;
	qmc_status_t retval = kStatus_QMC_ErrRange;

	/* input sanitation and limit checks */
	if(NULL == handle)
		return kStatus_QMC_ErrArgInvalid;
	if(!g_isInitialized_DataHub)
		return kStatus_QMC_Err;

	if(pdTRUE != xSemaphoreTake(g_statusQueueMutexHandle, portMAX_DELAY))
		return kStatus_QMC_Err;
	for(i=0; i<DATAHUB_MAX_STATUS_QUEUES; i++)
	{
		/* search handle and mark it as available again */
		if(handle == &(g_motorStatusQueueHandles[i]))
		{
			g_motorStatusQueueHandles[i].queueHandle = NULL;
			retval =  kStatus_QMC_Ok;
		}

		/* check, if all handles are returned */
		if(NULL != g_motorStatusQueueHandles[i].queueHandle)
			areAllHandlesReturned = false;
	}

	/* stop timer, if no handles are in use */
	if(areAllHandlesReturned)
	{
		xTimerStop(g_statusSamplingTimerHandle, 0);
		gs_isStatusSamplingTimerStarted = false;
	}

	xSemaphoreGive(g_statusQueueMutexHandle);
    return retval;
}

qmc_status_t MC_ExecuteMotorCommandFromTsn(const mc_motor_command_t* cmd)
{
	/* input sanitation and limit checks */
	if(NULL == cmd)
		return kStatus_QMC_ErrArgInvalid;
	if(IS_MOTORID_INVALID(cmd->eMotorId))
		return kStatus_QMC_ErrRange;
	if(!g_p.bIsTsnCommandInjectionOn[cmd->eMotorId])
		return kStatus_QMC_Err;
	if(!isMotorCommandInRange(cmd))
		return kStatus_QMC_ErrArgInvalid;

	if (g_p.bIsFrozen[cmd->eMotorId])
	{
		return kStatus_QMC_ErrBusy;
	}

	if (cmd->eAppSwitch == kMC_App_Freeze)
	{
		g_p.bIsFrozen[cmd->eMotorId] = true;
	}
	else if (cmd->eAppSwitch == kMC_App_FreezeAndStop)
	{
		g_p.bIsFrozen[cmd->eMotorId] = true;

		mc_motor_command_t cmd_stop = {0};
		cmd_stop.eMotorId = cmd->eMotorId;
		cmd_stop.eAppSwitch = kMC_App_Off;

		/* shared memory access must not be interrupted by other tasks (e.g. DataHub) */
		taskENTER_CRITICAL();

		/* lock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = true;

		/* copy parameters */
		g_p.sCommands[cmd->eMotorId] = cmd_stop;

		/* unlock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = false;

		taskEXIT_CRITICAL();
	}
	else
	{
		/* shared memory access must not be interrupted by other tasks (e.g. DataHub) */
		taskENTER_CRITICAL();

		/* lock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = true;

		/* copy parameters */
		g_p.sCommands[cmd->eMotorId] = *cmd;

		/* unlock memory */
		g_p.bWriteInProgress[cmd->eMotorId] = false;

		taskEXIT_CRITICAL();
	}

	return kStatus_QMC_Ok;
}

qmc_status_t MC_GetMotorStatusforTsn(mc_motor_id_t motorId, mc_motor_status_t* status)
{
	/* input sanitation and limit checks */
	if(NULL == status)
		return kStatus_QMC_ErrArgInvalid;
	if(IS_MOTORID_INVALID(motorId))
		return kStatus_QMC_ErrRange;

	/* shared memory access must not be interrupted by other tasks (e.g. DataHub) */
	taskENTER_CRITICAL();

	/* copy fast loop status values */
	if(g_p.bReadInProgress_fast[motorId])
	{ /* locking not required */
		status->sFast = g_p.sStatus[motorId].sFast;
	}
	else
	{ /* locking required */
		g_p.bReadInProgress_fast[motorId] = true;
		status->sFast = g_p.sStatus[motorId].sFast;
		g_p.bReadInProgress_fast[motorId] = false;
	}

	/* copy slow loop status values */
	if(g_p.bReadInProgress_slow[motorId])
	{ /* locking not required */
		status->sSlow = g_p.sStatus[motorId].sSlow;
	}
	else
	{ /* locking required */
		g_p.bReadInProgress_fast[motorId] = true;
		status->sSlow = g_p.sStatus[motorId].sSlow;
		g_p.bReadInProgress_fast[motorId] = false;
	}

	taskEXIT_CRITICAL();

	return kStatus_QMC_Ok;
}

/*!
* @brief Unfreeze a motor and tell the motor control API that it can stop ignoring further commands to the motor.
*
* @param[in] motor_id Id of the motor to be unfrozen
*/
qmc_status_t MC_UnfreezeMotor(mc_motor_id_t motor_id)
{
	if (IS_MOTORID_INVALID(motor_id))
	{
		return kStatus_QMC_ErrRange;
	}

	g_p.bIsFrozen[motor_id] = false;
	MC_SetTsnCommandInjectionById(true, motor_id); /* enable TSN commands for this motor */

	return kStatus_QMC_Ok;
}

static bool isMotorCommandInRange(const mc_motor_command_t* cmd)
{
	if(IS_MOTORID_INVALID(cmd->eMotorId))
		return false;
	if((cmd->eAppSwitch != kMC_App_Off) && (cmd->eAppSwitch != kMC_App_On) && (cmd->eAppSwitch != kMC_App_Freeze) && (cmd->eAppSwitch != kMC_App_FreezeAndStop))
		return false;
	switch(cmd->eControlMethodSel)
	{
	case kMC_ScalarControl:
		if((cmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain < MC_LIMIT_L_GAIN) || (cmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain > MC_LIMIT_H_GAIN))
			return false;
		if((cmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency < MC_LIMIT_L_FREQUENCY) || (cmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency > MC_LIMIT_H_FREQUENCY))
			return false;
		break;
	case kMC_FOC_SpeedControl:
		if((cmd->uSpeed_pos.fltSpeed < MC_LIMIT_L_SPEED) || (cmd->uSpeed_pos.fltSpeed > MC_LIMIT_H_SPEED))
			return false;
		break;
	case kMC_FOC_PositionControl:
		if((cmd->uSpeed_pos.sPosParam.uPosition.i32Raw < MC_LIMIT_L_POSITION) || (cmd->uSpeed_pos.sPosParam.uPosition.i32Raw > MC_LIMIT_H_POSITION))
			return false;
		break;
	default: return false;
	}
	return true;
}

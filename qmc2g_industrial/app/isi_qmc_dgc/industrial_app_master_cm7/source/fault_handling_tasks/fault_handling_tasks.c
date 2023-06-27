/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "fault_handling_tasks.h"
#include "fsl_debug_console.h"
#include "qmc_features_config.h"

#include "api_logging.h"
#include "api_motorcontrol.h"
#include "api_board.h"
#include "api_usermanagement.h"
#include "queue.h"
#include "timers.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ERROR_LOG_PERIOD_IN_SECONDS 		300u
#define MOTOR_QUEUE_TIMEOUT_ATTEMPTS        20u
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Stops motors per configuration.
 *
 * @param[in] faultMotorId The ID of the faulty motor.
 */
static void StopMotorsPerConfiguration(mc_motor_id_t faultMotorId);

/*!
 * @brief Stops all motors.
 */
static void StopAllMotors(void);

/*!
 * @brief Submits logs for all faults included in maskedSrc, one log per fault.
 *
 * @param[in] maskedSrc The faults that should be reported
 * @param[in] motor_id Id of the motor that was affected by the logged faults, if applicable
 */
static void SubmitLogs(fault_source_t maskedSrc, mc_motor_id_t motor_id);

/*!
 * @brief Periodically resets gs_AlreadyReportedAFECommunicationErrorForMotor to allow
 * another log to be created for the AFE communication error
 *
 * @param[in] xTimer Timer handle
 */
static void errorLogTimerCallback(TimerHandle_t xTimer);
/*******************************************************************************
 * Globals
 ******************************************************************************/
TaskHandle_t g_fault_handling_task_handle;
static fault_system_fault_t gs_AlreadyReportedFaultAPIErrorFlags;
static bool gs_AlreadyReportedAFECommunicationErrorForMotor[4] = { false, false, false, false };
extern EventGroupHandle_t g_systemStatusEventGroupHandle;
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Initiates the fault handling task, then starts an infinite loop that
 * reads incoming faults and handles and logs them.
 *
 * @param[in] pvParameters Unused.
 */
void FaultHandlingTask(void *pvParameters)
{
	TimerHandle_t errorLogTimerHandle;
	StaticTimer_t errorLogTimer;
	fault_source_t src = 0;
	gs_AlreadyReportedFaultAPIErrorFlags = 0;

	bool MCNoFault[4] = { true, true, true, true };
	bool BSNoFault[4] = { true, true, true, true };
	bool systemNoFault = true;

	errorLogTimerHandle = xTimerCreateStatic("errorLogTimer",
											  pdMS_TO_TICKS(ERROR_LOG_PERIOD_IN_SECONDS * 1000),
											  pdFALSE, NULL,
											  errorLogTimerCallback,
											  &errorLogTimer);

	xTaskNotifyWait(0, 0, &src, portMAX_DELAY);

	for( ;; )
	{
		mc_motor_id_t motor_id = FAULT_GetMotorIdFromSource(src);

		/* PSB faults reported by the motor control task. */
		if ((SRC_WITHOUT_MOTOR_ID(src) == kMC_NoFaultMC) || (src & MC_PSB_FAULTS_MASK))
		{
			if (SRC_WITHOUT_MOTOR_ID(src) == kMC_NoFaultMC)
			{
				/* Only clear the event bit if neither a board service task fault nor a motor control task fault is active */
				if (BSNoFault[motor_id])
				{
					switch(motor_id)
					{
						case kMC_Motor1:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor1);
							break;

						case kMC_Motor2:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor2);
							break;

						case kMC_Motor3:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor3);
							break;

						case kMC_Motor4:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor4);
							break;

						default:
							/* Impossible to reach unless the mc_motor_id_t enum gets extended. */
							assert(false);
							break;
					}
				}

				MCNoFault[motor_id] = true;
			}
			else
			{
				StopMotorsPerConfiguration(motor_id);
				switch(motor_id)
				{
					case kMC_Motor1:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor1);
						break;

					case kMC_Motor2:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor2);
						break;

					case kMC_Motor3:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor3);
						break;

					case kMC_Motor4:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor4);
						break;

					default:
						/* Impossible to reach unless the mc_motor_id_t enum gets extended. */
						assert(false);
						break;
				}

				MCNoFault[motor_id] = false;
				BOARD_SetLifecycle(kQMC_LcError);
			}

			SubmitLogs(src & ALL_MC_PSB_FAULTS_BITS_MASK, motor_id);
		}

		/* PSB faults reported by the board service or a fault handler, stopping motors per configuration. */
		if ((SRC_WITHOUT_MOTOR_ID(src) & kMC_NoFaultBS) || (src & BS_PSB_FAULTS_MASK))
		{
			if (SRC_WITHOUT_MOTOR_ID(src) & kMC_NoFaultBS)
			{
				/* Only clear the event bit if neither a board service task nor a motor control task fault is active */
				if (MCNoFault[motor_id])
				{
					switch(motor_id)
					{
						case kMC_Motor1:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor1);
							break;

						case kMC_Motor2:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor2);
							break;

						case kMC_Motor3:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor3);
							break;

						case kMC_Motor4:
							xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor4);
							break;

						default:
							/* Impossible to reach unless the mc_motor_id_t enum gets extended. */
							assert(false);
							break;
					}
				}

				BSNoFault[motor_id] = true;
			}
			else
			{
				StopMotorsPerConfiguration(motor_id);
				switch(motor_id)
				{
					case kMC_Motor1:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor1);
						break;

					case kMC_Motor2:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor2);
						break;

					case kMC_Motor3:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor3);
						break;

					case kMC_Motor4:
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_Motor4);
						break;

					default:
						/* Impossible to reach unless the mc_motor_id_t enum gets extended. */
						assert(false);
						break;
				}

				BSNoFault[motor_id] = false;
				BOARD_SetLifecycle(kQMC_LcError);
			}

			SubmitLogs(src & ALL_BS_PSB_FAULTS_BITS_MASK, motor_id);
		}

		/* System faults reported by the board service or a fault handler, stopping all motors. */
		if ((SRC_WITHOUT_MOTOR_ID(src) & kFAULT_NoFault) || (src & SYSTEM_FAULTS_MASK))
		{
			/* Only clear the System fault bit and log the NoFault if the buffer and queue aren't overflowed. */
			if (SRC_WITHOUT_MOTOR_ID(src) & kFAULT_NoFault)
			{
				if (!(g_systemFaultStatus & FAULT_OVERFLOW_ERRORS_MASK))
				{
					xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_System);
					g_systemFaultStatus = kFAULT_NoFault;
				}
				systemNoFault = true;
			}
			else
			{
				StopAllMotors();
				xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_System);
				g_systemFaultStatus |= src & SYSTEM_FAULTS_MASK;
				g_systemFaultStatus &= ~(kFAULT_NoFault);
				systemNoFault = false;
				BOARD_SetLifecycle(kQMC_LcError);
			}

			SubmitLogs(src & ALL_SYSTEM_FAULT_BITS_MASK, 0);
		}

		/* Fault handling errors related to communication with peripherals reported by the board service.
		 * These will only be reported once per a specified amount of time to prevent log flooding.
		 * g_AlreadyReportedFaultAPIErrorFlags is automatically cleared after the gs_errorLogTimer expires. */
		if (src & FAULT_HANDLING_ERRORS_MASK)
		{
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_System);
			g_systemFaultStatus |= src & FAULT_HANDLING_ERRORS_MASK;
			g_systemFaultStatus &= ~(kFAULT_NoFault);
			systemNoFault = false;


			if (((!gs_AlreadyReportedAFECommunicationErrorForMotor[motor_id]) && (SRC_WITHOUT_MOTOR_ID(src) == kFAULT_AfePsbCommunicationError)) ||\
					!(gs_AlreadyReportedFaultAPIErrorFlags & FAULT_HANDLING_ERRORS_MASK))
			{
				xTimerStart(errorLogTimerHandle, 0);
				if (SRC_WITHOUT_MOTOR_ID(src) == kFAULT_AfePsbCommunicationError)
				{
					SubmitLogs(src & FAULT_HANDLING_ERRORS_MASK, motor_id);

					gs_AlreadyReportedAFECommunicationErrorForMotor[motor_id] = true;
				}
				else
				{
					SubmitLogs(src & FAULT_HANDLING_ERRORS_MASK, 0);
				}

				gs_AlreadyReportedFaultAPIErrorFlags |= src & FAULT_HANDLING_ERRORS_MASK;
			}
		}

		/* Checking if the fault buffer or fault queue overflowed. */
		if (g_FaultHandlingErrorFlags & FAULT_OVERFLOW_ERRORS_MASK)
		{
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_System);
			g_systemFaultStatus |= g_FaultHandlingErrorFlags & FAULT_OVERFLOW_ERRORS_MASK;
			g_systemFaultStatus &= ~(kFAULT_NoFault);

			/* These will only be reported once per a specified amount of time to prevent log flooding.
			 * g_AlreadyReportedFaultAPIErrorFlags is automatically cleared after the gs_errorLogTimer expires. */
			if (CONTAINS_BUFFER_OVERFLOW_FLAG(g_FaultHandlingErrorFlags) && !(CONTAINS_BUFFER_OVERFLOW_FLAG(gs_AlreadyReportedFaultAPIErrorFlags)))
			{
				xTimerStart(errorLogTimerHandle, 0);
				SubmitLogs(kFAULT_FaultBufferOverflow, 0);
				gs_AlreadyReportedFaultAPIErrorFlags |= kFAULT_FaultBufferOverflow;
			}

			if (CONTAINS_QUEUE_OVERFLOW_FLAG(g_FaultHandlingErrorFlags) && !(CONTAINS_QUEUE_OVERFLOW_FLAG(gs_AlreadyReportedFaultAPIErrorFlags)))
			{
				xTimerStart(errorLogTimerHandle, 0);
				SubmitLogs(kFAULT_FaultQueueOverflow, 0);
				gs_AlreadyReportedFaultAPIErrorFlags |= kFAULT_FaultQueueOverflow;
			}
		}

		/* src contains invalid bits */
		if (src & INVALID_FAULT_BITS)
		{
			SubmitLogs(kFAULT_InvalidFaultSource, 0);
		}

		/* Check if the buffer and queue are empty before waiting for a new notification. */
		if (!FAULT_BufferEmpty())
		{
			/* Buffer is not empty. Read the next src from it and continue with the next iteration of the task. */
			src = FAULT_ReadBuffer();
		}
		else
		{
			/* Buffer is empty. Clear the overflow flag and check the queue. */
			g_FaultHandlingErrorFlags &= ~(kFAULT_FaultBufferOverflow);

			while (g_FaultQueue == NULL)
			{
				vTaskDelay(10);
			}

			/* If the queue is not empty, read the src and continue with the next iteration of the task. */
			if (xQueueReceive(g_FaultQueue, &src, 0) == errQUEUE_EMPTY)
			{
				/* Queue is empty. Clear the overflow flag and wait for a new notification */
				g_FaultHandlingErrorFlags &= ~(kFAULT_FaultQueueOverflow);

				/* If there are no system faults and an overflow error was reported previously, log a Nofault */
				if (systemNoFault && (g_systemFaultStatus & FAULT_OVERFLOW_ERRORS_MASK))
				{
					xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_FAULT_System);
					g_systemFaultStatus = kFAULT_NoFault;

					SubmitLogs(kFAULT_NoFault, 0);
				}

				xTaskNotifyWait(0, 0, &src, portMAX_DELAY);
			}
		}
	}
}

/*!
 * @brief A generic fault handler.
 *
 * @param[in] fault The fault that should be written into the buffer.
 *
 * @detail  Generic fault handler. Not used by the QMC2G demo application.
 * 			All interrupts calling the FAULT_RaiseFaultEvent_fromISR must
 * 			have the same priority as the fast motor control loop interrupts
 * 			or issues with the fault buffer may occur. Other solutions include
 *			using a critical section, mutexes etc. However, it is recommended
 * 			to always use polling instead of interrupts because causing any
 * 			delays to the fast motor control interrupt handling will result
 * 			in undefined behavior.
 */
void faulthandler_x_callback(fault_source_t src)
{
	StopMotorsPerConfiguration(FAULT_GetMotorIdFromSource(src));
	FAULT_RaiseFaultEvent_fromISR(src);
}

/*!
 * @brief Stops motors per configuration.
 *
 * @param[in] faultMotorId The ID of the faulty motor.
 */
static void StopMotorsPerConfiguration(mc_motor_id_t faultMotorId)
{
	for (mc_motor_id_t stopMotorId = kMC_Motor1; stopMotorId < MC_MAX_MOTORS; stopMotorId++)
	{
		if (faultMotorId == stopMotorId || FAULT_GetImmediateStopConfiguration_fromISR(faultMotorId, stopMotorId))
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

			if (motorQueueTimeout <= 0)
			{
				log_record_t logEntryError = {0};
				logEntryError.type = kLOG_FaultDataWithoutID;
				logEntryError.data.faultDataWithoutID.source = LOG_SRC_FaultHandling;
				logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
				logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_QueueingCommandFailedQueue;
				LOG_QueueLogEntry(&logEntryError, true);

				BOARD_SetLifecycle(kQMC_LcError);
				return;
			}
		}
	}
}

/*!
 * @brief Stops all motors.
 */
static void StopAllMotors(void)
{
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

		if (motorQueueTimeout <= 0)
		{
			log_record_t logEntryError = {0};
			logEntryError.type = kLOG_FaultDataWithoutID;
			logEntryError.data.faultDataWithoutID.source = LOG_SRC_FaultHandling;
			logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
			logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_QueueingCommandFailedQueue;
			LOG_QueueLogEntry(&logEntryError, true);

			BOARD_SetLifecycle(kQMC_LcError);
			return;
		}
	}
}

/*!
 * @brief Submits logs for all faults included in maskedSrc, one log per fault.
 *
 * @param[in] maskedSrc The faults that should be reported. Should be a masked src to make sure only the relevant faults are included.
 * @param[in] motor_id Id of the motor that was affected by the logged faults, if applicable
 */
static void SubmitLogs(fault_source_t maskedSrc, mc_motor_id_t motor_id)
{
	log_record_t logEntryWithId = {
			.rhead = {
					.chksum				= 0,
					.uuid				= 0,
					.ts = {
						.seconds		= 0,
						.milliseconds	= 0
					}
			},
			.type = kLOG_FaultDataWithID,
			.data.faultDataWithID.source = LOG_SRC_FaultHandling,
			.data.faultDataWithID.category = LOG_CAT_Fault,
			.data.faultDataWithID.motorId = motor_id
	};

	log_record_t logEntryWithoutId = {
			.rhead = {
					.chksum				= 0,
					.uuid				= 0,
					.ts = {
						.seconds		= 0,
						.milliseconds	= 0
					}
			},
			.type = kLOG_FaultDataWithoutID,
			.data.faultDataWithoutID.source = LOG_SRC_FaultHandling,
			.data.faultDataWithoutID.category = LOG_CAT_Fault
	};

	if (maskedSrc == kMC_NoFaultMC)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_NoFaultMC;
		LOG_QueueLogEntry(&logEntryWithId, true);
		return; /* Not necessary to look for other faults, since maskedSrc is all 0s. */
	}

	if (maskedSrc & kMC_NoFaultBS)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_NoFaultBS;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_OverCurrent)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_OverCurrent;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_UnderDcBusVoltage)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_UnderDcBusVoltage;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_OverDcBusVoltage)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_OverDcBusVoltage;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_OverLoad)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_OverLoad;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_OverSpeed)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_OverSpeed;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_RotorBlocked)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_RotorBlocked;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_GD3000_OverTemperature)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_GD3000_OverTemperature;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_GD3000_Desaturation)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_GD3000_Desaturation;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_GD3000_LowVLS)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_GD3000_LowVLS;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_GD3000_OverCurrent)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_GD3000_OverCurrent;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_GD3000_PhaseError)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_GD3000_PhaseError;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_GD3000_Reset)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_GD3000_Reset;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_PsbOverTemperature1)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_PsbOverTemperature1;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kMC_PsbOverTemperature2)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_PsbOverTemperature2;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kFAULT_AfePsbCommunicationError)
	{
		logEntryWithId.data.faultDataWithID.eventCode = LOG_EVENT_AfePsbCommunicationError;
		LOG_QueueLogEntry(&logEntryWithId, true);
	}

	if (maskedSrc & kFAULT_NoFault)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_NoFault;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_DbOverTemperature)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_DbOverTemperature;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_McuOverTemperature)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_McuOverTemperature;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_PmicUnderVoltage1)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_PmicUnderVoltage1;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_PmicUnderVoltage2)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_PmicUnderVoltage2;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_PmicUnderVoltage3)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_PmicUnderVoltage3;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_PmicUnderVoltage4)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_PmicUnderVoltage4;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_PmicOverTemperature)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_PmicOverTemperature;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_EmergencyStop)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_EmergencyStop;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_AfeDbCommunicationError)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_AfeDbCommunicationError;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_DBTempSensCommunicationError)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_DBTempSensCommunicationError;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_FaultBufferOverflow)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_FaultBufferOverflow;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_FaultQueueOverflow)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_FaultQueueOverflow;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}

	if (maskedSrc & kFAULT_InvalidFaultSource)
	{
		logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_InvalidFaultSource;
		LOG_QueueLogEntry(&logEntryWithoutId, true);
	}
}

/*!
 * @brief Periodically resets gs_AlreadyReportedAFECommunicationErrorForMotor to allow
 * another log to be created for the AFE communication error
 *
 * @param[in] xTimer Timer handle
 */
static void errorLogTimerCallback(TimerHandle_t xTimer)
{
	gs_AlreadyReportedFaultAPIErrorFlags = 0;

	for (int i = 0; i < MC_MAX_MOTORS; i++)
	{
		gs_AlreadyReportedAFECommunicationErrorForMotor[i] = false;
	}
}

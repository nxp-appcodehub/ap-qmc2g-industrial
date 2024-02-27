/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_FAULT_H_
#define _API_FAULT_H_

#include <cr_section_macros.h>
#include "api_qmc_common.h"
#include "api_motorcontrol.h"
#include "api_motorcontrol_internal.h"
#include "stdbool.h"
#include "queue.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ID_BITS								(uint32_t) 0x00000003
#define FAULT_GetIdFromSource(x) 			((uint8_t) ((x) & (uint32_t) ID_BITS))
#define SRC_WITHOUT_ID(x) 					(x & ~((uint32_t) ID_BITS))

#define SYSTEM_FAULTS_MASK 					(uint32_t) (kFAULT_DbOverTemperature | kFAULT_McuOverTemperature | kFAULT_EmergencyStop | kFAULT_PmicUnderVoltage |\
											kFAULT_RpcCallFailed | kFAULT_PmicOverTemperature | kFAULT_FunctionalWatchdogInitFail)

#define FAULT_HANDLING_ERRORS_MASK 			(uint32_t) (kFAULT_AfePsbCommunicationError | kFAULT_AfeDbCommunicationError | kFAULT_DBTempSensCommunicationError)

#define FAULT_OVERFLOW_ERRORS_MASK 			(uint32_t) (kFAULT_FaultBufferOverflow | kFAULT_FaultQueueOverflow)

#define ALL_SYSTEM_FAULT_BITS_MASK 			(uint32_t) (SYSTEM_FAULTS_MASK | FAULT_HANDLING_ERRORS_MASK | FAULT_OVERFLOW_ERRORS_MASK | kFAULT_NoFault)

#define INVALID_FAULT_BITS 					(~ (uint32_t) (ALL_SYSTEM_FAULT_BITS_MASK | ALL_PSB_FAULTS_BITS_MASK | ID_BITS | kFAULT_KickWatchdogNotification | kFAULT_FunctionalWatchdogInitFail))

#define CONTAINS_BUFFER_OVERFLOW_FLAG(x) 	(x & (uint32_t) kFAULT_FaultBufferOverflow)

#define CONTAINS_QUEUE_OVERFLOW_FLAG(x) 	(x & (uint32_t) kFAULT_FaultQueueOverflow)

#define IS_MOTORID_INVALID(x)				(((x) < kMC_Motor1) || ((x) >= (MC_MAX_MOTORS)))

 /*!
 * @brief Enumeration that list system-wide (not motor specific) faults.
 *
 * Upon detection of a fault listed in this enumeration, all motors must be stopped immediately.
 */
typedef enum _fault_system_fault
{
    kFAULT_NoFault						= 0x00020000U, /*!< Normal operation; no fault occurred. */
    kFAULT_DbOverTemperature			= 0x00040000U, /*!< Over-temperature on digital board */
    kFAULT_McuOverTemperature			= 0x00080000U, /*!< MCU over-temperature */
    kFAULT_PmicUnderVoltage				= 0x00100000U, /*!< PMIC Under-voltage */
	kFAULT_PmicOverTemperature			= 0x00200000U, /*!< PMIC (daughter card) over-temperature */
	kFAULT_EmergencyStop				= 0x00400000U, /*!< Emergency stop was triggered */
	kFAULT_RpcCallFailed				= 0x00800000U, /*!< RPC call failed */
	kFAULT_AfePsbCommunicationError		= 0x01000000U, /*!< Failed to communicate with an AFE on a PSB */
	kFAULT_AfeDbCommunicationError		= 0x02000000U, /*!< Failed to communicate with the AFE on the DB */
	kFAULT_DBTempSensCommunicationError	= 0x04000000U, /*!< Failed to communicate with the temperature sensor on the DB */
	kFAULT_FaultBufferOverflow			= 0x08000000U, /*!< Circular fault buffer overflow */
	kFAULT_FaultQueueOverflow			= 0x10000000U, /*!< Fault queue overflow */
	kFAULT_InvalidFaultSource			= 0x20000000U, /*!< Invalid fault source */
	kFAULT_KickWatchdogNotification		= 0x40000000U, /*!< Notification for the fault handling task to kick its functional watchdog */
	kFAULT_FunctionalWatchdogInitFail	= 0x80000000U, /*!< The initial kick for a functional watchdog failed */
} fault_system_fault_t;

 /*!
 * @brief 32-bit enumeration type that is a bit-packed composition of mc_motor_id_t, mc_fault_t and fault_system_fault_t.
 */
typedef uint32_t fault_source_t;
typedef uint32_t fault_handling_error_flags_t;

extern mc_control_shm g_p; /* Global shared memory variable */
extern TaskHandle_t g_fault_handling_task_handle; /* Fault handling task handle */
extern fault_system_fault_t g_systemFaultStatus;
extern fault_system_fault_t g_FaultHandlingErrorFlags;
extern mc_fault_t g_psbFaults[4];
extern QueueHandle_t g_FaultQueue;


/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Writes a fault into the circular fault buffer.
 *
 * @param[in] fault The fault that should be written into the buffer.
 *
 * @retval Returns QMC status.
 */
__RAMFUNC(SRAM_ITC_cm7) qmc_status_t FAULT_WriteBuffer(fault_source_t fault);

 /*!
 * @brief Sends a FAULT signal to the Fault Handling service indicating the given fault_source_t. For valid fault sources see referenced enumeration type.
 *
 * This function should be used within the MOTOR CONTROL ISRs only; for calling from a task use FAULT_RaiseFaultEvent(src : fault_source_t) : void from FaultAPI.
 *
 * @param[in] src Source that triggered this fault
 */
__attribute__((always_inline)) static inline void FAULT_RaiseFaultEvent_fromISR(fault_source_t src)
{
	if (xTaskNotifyFromISR(g_fault_handling_task_handle, src, eSetValueWithoutOverwrite, 0) == pdFAIL)
	{
		/* If the fault buffer overflows, set the flag in g_FaultHandlingErrorFlags. If the error has already been reported and is marked in
		 * the g_systemFaultStatus and the buffer is no longer overflowed, clear the flag. This ensures that the flag won't be cleared before
		 * the error is reported. */
		if (FAULT_WriteBuffer(src) == kStatus_QMC_Err)
		{
			g_FaultHandlingErrorFlags |= kFAULT_FaultBufferOverflow;
		}
		else if (CONTAINS_BUFFER_OVERFLOW_FLAG(g_systemFaultStatus))
		{
			g_FaultHandlingErrorFlags &= (fault_system_fault_t) ~((uint32_t) kFAULT_FaultBufferOverflow);
		}
		else
		{
			;
		}
	}
}

 /*!
 * @brief Set the immediate motor stop behaviour for the motor defined by stopMotorId that should be applied in case the motor defined by faultMotorId experiences a fault. If stopMotorId equals faultMotorId, the input is ignored and the configuration remains unchanged.
 *
 * @param[in] faultMotorId Motor to set the stop behaviour for, in case it experiences a fault
 * @param[in] stopMotorId Motor to apply the stop behaviour to
 * @param[in] doStop Action to be performed; true = stop, false = do not stop
 */

__attribute__((always_inline)) static inline void FAULT_SetImmediateStopConfiguration_fromISR(mc_motor_id_t faultMotorId, mc_motor_id_t stopMotorId, bool doStop)
{
	if (faultMotorId == stopMotorId)
	{
		return;
	}

	if (IS_MOTORID_INVALID(faultMotorId) || IS_MOTORID_INVALID(stopMotorId))
	{
		return;
	}

	uint16_t mask = (1 << ((4 * faultMotorId) + stopMotorId));

	if (doStop)
	{
		g_p.ui16MotorFaultConfiguration |= mask;
	}
	else
	{
		g_p.ui16MotorFaultConfiguration &= ~mask;
	}
}

 /*!
 * @brief Get the immediate motor stop behaviour for the motor defined by stopMotorId that should be applied in case the motor defined by faultMotorId experiences a fault. If stopMotorId equals faultMotorId true is returned, regardless of the configuration.
 *
 * @param[in] faultMotorId Motor that experienced a fault
 * @param[in] stopMotorId Motor to get the stop behaviour for
 * @return The action to be performed; true = stop, false = do not stop
 */
__attribute__((always_inline)) static inline bool FAULT_GetImmediateStopConfiguration_fromISR(mc_motor_id_t faultMotorId, mc_motor_id_t stopMotorId)
{
	if (faultMotorId == stopMotorId)
	{
		return true;
	}

	if (IS_MOTORID_INVALID(faultMotorId) || IS_MOTORID_INVALID(stopMotorId))
	{
		return false;
	}

	uint16_t mask = (1 << ((4 * faultMotorId) + stopMotorId));
	return g_p.ui16MotorFaultConfiguration & mask;
}

/*!
 * @brief Reads from the circular fault buffer.
 *
 * @retval Returns the stored fault.
 */
fault_source_t FAULT_ReadBuffer(void);

/*!
 * @brief Checks if the circular fault buffer is empty.
 *
 * @retval True if buffer is empty, false if it isn't.
 */
bool FAULT_BufferEmpty(void);

/*!
 * @brief Initializes the fault queue
 *
 * @retval Returns QMC status.
 */
qmc_status_t FAULT_InitFaultQueue(void);

/*!
* @brief Sends a FAULT signal to the Fault Handling service indicating the given fault_source_t. For valid fault sources see referenced enumeration type. (Not ISR safe version)
*
* @param src Source that triggered this fault
*/
void FAULT_RaiseFaultEvent(fault_source_t src);

/*
* @brief Get the current system fault status.
*/
__attribute__((always_inline)) static inline fault_system_fault_t FAULT_GetSystemFault_fromISR()
{
    return g_systemFaultStatus;
}

/*!
* @brief Get the current motor fault status for the given motor ID. If motor ID is invalid, kMC_NoFaultMC shall be returned.
*
* @param[in] motorId Motor to get the fault information from
*/
__attribute__((always_inline)) static inline mc_fault_t FAULT_GetMotorFault_fromISR(mc_motor_id_t motorId)
{
	/* input sanitation and limit checks */
    if(IS_MOTORID_INVALID(motorId))
        return kMC_NoFaultMC;
    return g_p.sStatus[motorId].sFast.eFaultStatus;
}

#endif /* _API_FAULT_H_ */

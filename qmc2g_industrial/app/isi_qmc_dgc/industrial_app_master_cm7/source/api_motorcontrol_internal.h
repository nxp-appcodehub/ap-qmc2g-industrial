/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _API_MOTORCONTROL_INTERNAL_H_
#define _API_MOTORCONTROL_INTERNAL_H_

#include "qmc_features_config.h"
#include "api_motorcontrol.h"
#include "stdatomic.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief This structure is meant to be instantiated once as a global variable. It acts as a the shared memory interface between the Data Hub task and the motor control loops.
 */
typedef struct _mc_control_shm {
	volatile _Atomic bool       bIsTsnCommandInjectionOn;            /*!< Defines, whether the TSN Receive interrupt service routine is allowed to inject a MotorCommand for execution by Slow Motor Control Loop. */
	volatile mc_motor_command_t sCommands[MC_MAX_MOTORS];            /*!< Array of MotorCommand that holds the motor commands for each of the four motors. Commands are read by the Slow Motor Control Loop and written by the Data Hub task; TSN Receive can also write them. */
	volatile _Atomic bool       bWriteInProgress[MC_MAX_MOTORS];     /*!< Array of booleans that act as mutexes for the corresponding entries in the commands : MotorCommand[] array. */
	volatile mc_motor_status_t  sStatus[MC_MAX_MOTORS];              /*!< Array of MotorStatus that holds the motor status for each of the four motors. The status is written by the Fast Motor Control Loop and  Slow Motor Control Loop; it is read by the Data Hub task. */
	volatile _Atomic bool       bReadInProgress_fast[MC_MAX_MOTORS]; /*!< Array of booleans that act as mutexes for the ms_fast section of the corresponding entries in the status : MotorStatus[] array. */
	volatile _Atomic bool       bReadInProgress_slow[MC_MAX_MOTORS]; /*!< Array of booleans that act as mutexes for the ms_slow section of the corresponding entries in the status : MotorStatus[] array. */
	volatile _Atomic bool 		bIsFrozen[MC_MAX_MOTORS];			 /*!< Array of booleans that tell the motor control API to ignore incoming commands. */
	volatile uint16_t           ui16MotorFaultConfiguration;         /*!< Describes for each of the four motors which other motors need to be stopped immediately in case of a fault. */
	volatile mc_motor_command_t sCommands_int[MC_MAX_MOTORS];        /*!< Array of MotorCommand that holds the last valid motor command for each motor and acts as an internal buffer for the Slow Motor Control Loop. */
    #if (FEATURE_ANOMALY_DETECTION != 0)
	    volatile mc_motor_status_fast_t sSyncStatusA[MC_MAX_MOTORS][AD_CURRENT_BLK_SIZE]; /*!< First buffer for motor status values from the Fast Motor Control Loop. It is used exclusively by the Anomaly Detection interrupt service routine. */
	    volatile mc_motor_status_fast_t sSyncStatusB[MC_MAX_MOTORS][AD_CURRENT_BLK_SIZE]; /*!< Second buffer for motor status values from the Fast Motor Control Loop. It is used exclusively by the Anomaly Detection interrupt service routine. */
	    volatile mc_motor_status_fast_t *sSyncStatus[MC_MAX_MOTORS]; /*!< Pointer to the next buffer to read motor status values. Access to this double-buffered data is provided through the getFastMotorStatusSync_fromISR(motorId : MotorId, status : ms_fast**) : Status function of MotorAPI_ISR. It is used exclusively by the AD Capture interrupt service routine. */
    #endif
} mc_control_shm;



/*******************************************************************************
 * API
 ******************************************************************************/

 /*!
 * @brief Internal function used by the Data Hub task to write a mc_motor_command_t to the shared memory.
 *
 * @param[in] cmd Motor command to be set for execution
 */
qmc_status_t MC_SetMotorCommand(const mc_motor_command_t* cmd);

 /*!
 * @brief Internal function used by the Data Hub task to read a mc_motor_status_t from the shared memory.
 *
 * @param[in]  motorId Motor of which to retrieve the status
 * @param[out] status Pointer to write the retrieved status to
 */
void MC_GetMotorStatus(mc_motor_id_t motorId, mc_motor_status_t* status);

 /*!
 * @brief Internal function used by the Fast Motor Control Loop to write part of a mc_motor_status_t to the shared memory.
 *
 * @param[in] motorId Motor ID of the motor this status applies to
 * @param[in] status Status to be written to the shared memory
 */
void MC_SetFastMotorStatus_fromISR(mc_motor_id_t motorId, const mc_motor_status_fast_t* status);

 /*!
 * @brief Internal function used by the Slow Motor Control Loop to write part of a mc_motor_status_t to the shared memory.
 *
 * @param[in] motorId Motor ID of the motor this status applies to
 * @param[in] status Status to be written to the shared memory
 */
void MC_SetSlowMotorStatus_fromISR(mc_motor_id_t motorId, const mc_motor_status_slow_t* status);

 /*!
 * @brief Internal function used by the Slow Motor Control Loop to get the current mc_motor_command_t for execution.
 *
 * @param[in] motorId Motor for which the command shall be fetched
 * @return Pointer to a motor command for the selected motor. NULL, if motorId is invalid.
 */
mc_motor_command_t* MC_GetMotorCommand_fromISR(mc_motor_id_t motorId);

 /*!
 * @brief Internal function used by the AD Capture handler to get the part of the mc_motor_status_t written by the Fast Motor Control Loop (phase current values) synchronously. Access to this data uses double buffering.
 *
 * @param[in]  motorId Motor of which to retrieve the status
 * @param[out] status Pointer to write the block of retrieved status values to
 */
qmc_status_t MC_GetFastMotorStatusSync_fromISR(mc_motor_id_t motorId, mc_motor_status_fast_t** status);

/*!
* @brief Function to be used by the TSN main loop to set a mc_motor_command_t for execution.
*
* @param[in] cmd Motor command to be set for execution
*/
qmc_status_t MC_ExecuteMotorCommandFromTsn(const mc_motor_command_t* cmd);

/*!
* @brief Function to be used by the TSN main loop to retrieve a mc_motor_status_t.
*
* @param[in]  motorId Motor of which to retrieve the status
* @param[out] status Pointer to write the retrieved status to
*/
qmc_status_t MC_GetMotorStatusforTsn(mc_motor_id_t motorId, mc_motor_status_t* status);


#endif /* _API_MOTORCONTROL_INTERNAL_H_ */

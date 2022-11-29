/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef TSN_TASKS_TSN_TASKS_H_
#define TSN_TASKS_TSN_TASKS_H_

#include "main_cm7.h"
#include "api_motorcontrol.h"

typedef struct mc_command
{
	uint8_t eAppSwitch;        /*!< Defines whether motor control is enabled */
	uint8_t eControlMethodSel; /*!< Control method to be used */
	union {
	        float                   fltSpeed;                         /*!< Speed parameter for FOC speed control method */
	        struct {
	            float               fltScalarControlVHzGain;      /*!< V/F gain for scalar control method */
	            float               fltScalarControlFrequency; /*!< Frequency parameter for scalar control method */
	        };
	        struct {
	        	int32_t             i32Raw;                 /*!< Actual motor position; signed 32-bit (fractional Q15.16 value) */
	        	uint8_t             bIsRandomPosition;                /*!< Indicates that the position is not on a trajectory and may need filtering */
	        };
	    };
}mc_command_t;


typedef struct _mc_motor_command_frame
{
	uint8_t dirty_flags;
	mc_command_t commands[4];

}mc_motor_command_frame_t;


typedef struct mc_status
{
	mc_motor_status_fast_t sFast;    /*!< Part of the status that is written by the Fast Motor Control Loop */
	mc_motor_status_slow_t sSlow;    /*!< Part of the status that is written by the Slow Motor Control Loop */

}mc_status_t;



typedef struct system_status_frame
{
	uint32_t system_status;
	uint32_t system_fault;
	mc_status_t motor_status[4];
	uint16_t MC_ExecuteMotorCommand_status[4];

}system_status_frame_t;



typedef struct qmc2g_tsn_ctx
{
	struct cyclic_task *c_task;
	uint8_t lost_frames_counter;
	uint8_t network_status_ok;
	uint16_t MC_ExecuteMotorCommand_status[4];

}qmc2g_tsn_ctx_t;

qmc_status_t TsnInit(void);


#endif /* TSN_TASKS_TSN_TASKS_H_ */

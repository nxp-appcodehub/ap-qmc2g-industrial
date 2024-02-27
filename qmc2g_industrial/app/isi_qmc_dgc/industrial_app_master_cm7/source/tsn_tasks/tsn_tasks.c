/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "fsl_device_registers.h"

#include "stats_task.h"
#include "lwip.h"
#include "genavb.h"
#include "log.h"
#include "cyclic_task.h"
#include "tsn_tasks_config.h"
#include "system_config.h"
#include "tsn_tasks.h"

#include "api_motorcontrol.h"
#include "api_fault.h"
#include "api_qmc_common.h"
#include "api_rpc.h"
#include "main_cm7.h"
#include "api_motorcontrol_internal.h"
#include "api_logging.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STATS_PERIOD_MS 2000
#define LOST_FRAMES 10

/*******************************************************************************
 * Variables
 ******************************************************************************/
static qmc2g_tsn_ctx_t gs_tsn_ctx;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*	TSN_main_loop
 *
 *  place of execution: ITCM
 *
 *  description: Check network status, read motors statuses, sysstem status and fault status.
 *               Create frame with all the information and send the data to TSN controller.
 *
 *  params:     *data - pass the context into the function
 *              timer_status - not used in the function
 *
 */
static void TSN_main_loop(void *data, int timer_status)
{

	qmc2g_tsn_ctx_t *context = data;
	system_status_frame_t system_status_frame;
    mc_motor_status_t motor_status;

	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogTSN) != kStatus_QMC_Ok)
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
				.data.systemData.source = LOG_SRC_TSN,
				.data.systemData.category = LOG_CAT_General,
				.data.systemData.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed
		};

		LOG_QueueLogEntry(&logEntryWithoutId, false);
	}

    /*Check, if receive callback was executed (callback set network_status_ok),
     * which means network is OK*/
    if(!context->network_status_ok)
    {
    	/*Increment lost frames counter */
    	context->lost_frames_counter +=1;

    	/*if 10 continuous frames are lost, we are out of synch and QMC_SYSEVENT_NETWORK_TsnSyncLost is set.
    	 * Number of frames can be configured via LOST_FRAMES macro.
    	 */
    	if(context->lost_frames_counter >= LOST_FRAMES)
    	{
    		xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_NETWORK_TsnSyncLost);
    	}

    	/*Check the link status, which is passed via task context.
    	 *If link status is not set, QMC_SYSEVENT_NETWORK_NoLink is set*/
    	if(!context->c_task->rx_socket[0].stats.link_status)
    	{
    		xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_NETWORK_NoLink);
    	}

    }
    else
    {
    	/*This branch signalizes network is ok*/

    	/*Clear network_status_ok for this round. network_status_ok must be set byt received
    	 * callback function in next period*/
    	context->network_status_ok = 0;

    	/*Clear lost frames counter*/
    	context->lost_frames_counter = 0;
    }

    /*Check the link status, which is passed via task context.
     *If link status is set, QMC_SYSEVENT_NETWORK_NoLink is cleared*/
    if(context->c_task->rx_socket[0].stats.link_status)
    {
        xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_NETWORK_NoLink);
    }

    /*Call MC_GetMotorStatusforTsn function for all four motors. Save motor statuses including
     * MC_GetMotorStatusforTsn function return status into the prepared system status frame */
    for(mc_motor_id_t i = 0; i < MC_MAX_MOTORS; i++)
    {
    	MC_GetMotorStatusforTsn(i, &motor_status);
    	system_status_frame.motor_status[i].sFast = motor_status.sFast;
    	system_status_frame.motor_status[i].sSlow = motor_status.sSlow;

    	system_status_frame.MC_ExecuteMotorCommand_status[i] = context->MC_ExecuteMotorCommand_status[i];
    }



    /*Read system status and save in into the prepared system status frame */
    system_status_frame.system_status = xEventGroupGetBits(g_systemStatusEventGroupHandle);


    /* Read System fault status from SHM*/
    system_status_frame.system_fault = FAULT_GetSystemFault_fromISR();


    /*Send information to TSN controller*/
    cyclic_net_transmit(context->c_task, 0, &system_status_frame, sizeof(system_status_frame));

}


/*	TSN_receive_MC_command
 *
 *  place of execution: ITCM
 *
 *  description: Receive callback function. When frame is received with correct timestamp,
 *               this function is called. Function clears TSN related system statuses and
 *               checks received frame.
 *               If received frame contains new data for motors, motor control API is called
 *               and the data are passed to motor control task using shared memory.
 *
 *
 *
 *  params:     *ctx  		task context
 *              msg_id  	message id, used to identify the message
 *              src_id  	source id
 *              *buf  		buffer with the received frame
 *              len  		length of the buffer
 */
static void TSN_receive_MC_command(void *ctx, int msg_id, int src_id, void *buf, int len)
{

	mc_motor_command_frame_t * mccf = buf;
	qmc2g_tsn_ctx_t *context = ctx;
	mc_motor_command_t mc_command;

    /*Signalizes correct frame receive to TSN_main_loop*/
    context->network_status_ok = 1;

    /*If this callback is executed, network works correct inlcuding synchronization,
     * bits below can be cleared*/
    xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_NETWORK_TsnSyncLost | QMC_SYSEVENT_NETWORK_NoLink);


    /*Going through received data...always data for all 4 motors are send together*/
	for(uint8_t i = 0; i<MC_MAX_MOTORS; i++)
	{
	    /*	First byte determines, if data are new and will be send to the motors.
	     *  If dirty flag is not set, no new data will be applied to the motors and
	     *  the block below is skipped.
	     */
		if(mccf->dirty_flags & (1<<i))
		{
            /*Set motor ID. Each passing through the for loop increments the ID by 1 up to 3 */
   			mc_command.eMotorId = i;

   			/*Read data about motor control enabled from received frame and set it to prepared motor
   			 *command*/
   			mc_command.eAppSwitch = mccf->commands[i].eAppSwitch;
   			/*Read control method from received framer and set it to prepared motor command*/
   			mc_command.eControlMethodSel = mccf->commands[i].eControlMethodSel;

   			/*According to the received motor control method, one of the branch below is executed
   			 * and appropriate data are read from received frame and set to prepared motor command*/

   			/*Scalar control*/
   			if(mc_command.eControlMethodSel == kMC_ScalarControl)
   			{
   				mc_command.uSpeed_pos.sScalarParam.fltScalarControlFrequency = mccf->commands[i].fltScalarControlFrequency;
   				mc_command.uSpeed_pos.sScalarParam.fltScalarControlVHzGain = mccf->commands[i].fltScalarControlVHzGain;
   			}
   			/*Direct speed control*/
   			else if(mc_command.eControlMethodSel == kMC_FOC_SpeedControl )
   			{
   				mc_command.uSpeed_pos.fltSpeed = mccf->commands[i].fltSpeed;
   			}
   			/*Direct position control*/
   			else
   			{
   				mc_command.uSpeed_pos.sPosParam.uPosition.i32Raw = mccf->commands[i].i32Raw;
   				mc_command.uSpeed_pos.sPosParam.bIsRandomPosition = mccf->commands[i].bIsRandomPosition;
   			}

   			/*Read return status from MC_ExecuteMotorCommandFromTsn and save it to the task context.
   			 * This status is then send back to TSN controller*/
   			context->MC_ExecuteMotorCommand_status[i] = MC_ExecuteMotorCommandFromTsn(&mc_command);
		}
	}
}


/*	TsnInitTask
 *
 *  place of execution: flash
 *
 *  description: Basic task which initializes all TSN related thing.
 *               When initialization is finished, task is deleted
 *
 *  params:     *pvParameters - not used in the task
 *
 */
qmc_status_t TsnInit(void)
{
	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogTSN) != kStatus_QMC_Ok)
	{
		FAULT_RaiseFaultEvent(kFAULT_FunctionalWatchdogInitFail);
	}

    struct tsn_app_config *config = NULL;

    /*Initialize network event bits*/
    xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_NETWORK_TsnSyncLost | QMC_SYSEVENT_NETWORK_NoLink);


    /*Initialize PHY and ENET_QoS clock*/
	BOARD_InitNetInterfaces();

	/*Enable interrupts related to the TSN*/
	BOARD_InitNVIC();

    /*Statistics task initialization - creates TASK called "stats" with PRIO=5*/
	if (STATS_TaskInit(NULL, NULL, STATS_PERIOD_MS) < 0){
#if PRINT_LEVEL == VERBOSE_DEBUG
	        ERR("STATS_TaskInit() failed\n");
#endif
	    goto exit;
	}

	/*Get default tsn application configuration from system_cfg structure defined in tsn_app/configs.c */
	config = system_config_get_tsn_app();

	/*Check, if configuration exists*/
	if (!config) {
#if PRINT_LEVEL == VERBOSE_DEBUG
	     ERR("system_config_get_tsn_app() failed\n");
#endif
	    goto exit;
	}

	/*Genavb stack initialization. Provides stack init call API call + some other initialization*/
	if (gavb_stack_init()) {
#if PRINT_LEVEL == VERBOSE_DEBUG
	     ERR("gavb_stack_init() failed\n");
#endif
	     goto exit;
	}

    /*port statistics initialization*/
	if (gavb_port_stats_init(0)) {
#if PRINT_LEVEL == VERBOSE_DEBUG
	     ERR("gavb_port_stats_init() failed\n");
#endif
	     goto exit;
	}

	/*lwip stack initialization*/
	lwip_stack_init();


	/*Read cyclic task parameters according to the selected role (CONTROLLER/IO_DEVICE)*/
	gs_tsn_ctx.c_task = tsn_conf_get_cyclic_task(config->role);
	    if (!gs_tsn_ctx.c_task) {
#if PRINT_LEVEL == VERBOSE_DEBUG
	         ERR("tsn_conf_get_cyclic_task() failed\n");
#endif
	         goto exit;
	    }

    /*Set cyclic task period. We use 1ms period*/
	cyclic_task_set_period(gs_tsn_ctx.c_task, config->period_ns);

	/*Check, if application period is configured in correct range*/
		if (config->period_ns < APP_PERIOD_MIN) {
#if PRINT_LEVEL == VERBOSE_DEBUG
		     ERR("invalid application period, minimum is %u ns\n", APP_PERIOD_MIN);
#endif
		     goto exit;
		}

	/*Set scheduled traffic and frame preemption for the cyclic task according to the configuration*/
	gs_tsn_ctx.c_task->params.use_st = config->use_st;
	gs_tsn_ctx.c_task->params.use_fp = config->use_fp;

	//TODO Remove this line as soon as web service is available
	MC_SetTsnCommandInjection(true);


    /*Cyclic task initialization*/
	cyclic_task_init(gs_tsn_ctx.c_task, TSN_receive_MC_command, TSN_main_loop, &gs_tsn_ctx);

	/*Clear network status. At the beginning, node is not synchronized*/
	gs_tsn_ctx.network_status_ok = 0;
	gs_tsn_ctx.lost_frames_counter = 0;

	/*Start cyclic task*/
	cyclic_task_start(gs_tsn_ctx.c_task);

	if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogTSN) != kStatus_QMC_Ok)
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
				.data.systemData.source = LOG_SRC_TSN,
				.data.systemData.category = LOG_CAT_General,
				.data.systemData.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed
		};

		LOG_QueueLogEntry(&logEntryWithoutId, false);
	}

	return kStatus_QMC_Ok;

exit:
	return kStatus_QMC_Err;

}


/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "api_qmc_common.h"
#include "qmc_features_config.h"
#include "api_board.h"
#include "api_rpc_internal.h"
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

extern qmc_status_t DataHubInit();

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern EventGroupHandle_t g_inputButtonEventGroupHandle;
extern mc_motor_command_t g_sMotorCmdTst; /* Motor commands from command queue */

extern TaskHandle_t g_fault_handling_task_handle;
extern TaskHandle_t g_board_service_task_handle;
extern TaskHandle_t g_datahub_task_handle;
extern TaskHandle_t g_local_service_task_handle;
extern TaskHandle_t g_getMotorStatus_task_handle;

/*******************************************************************************
 * Code
 ******************************************************************************/

void StartupTask(void *pvParameters)
{
	//TODO: Put initialization code here

	/* Initialize GPIO/button input handlers */
	xEventGroupSetBits(g_inputButtonEventGroupHandle, QMC_IOEVENT_BTN1_RELEASED | QMC_IOEVENT_BTN2_RELEASED | QMC_IOEVENT_BTN3_RELEASED  | QMC_IOEVENT_BTN4_RELEASED | QMC_IOEVENT_INPUT0_LOW | QMC_IOEVENT_INPUT1_LOW | QMC_IOEVENT_INPUT2_LOW | QMC_IOEVENT_INPUT3_LOW | QMC_IOEVENT_INPUT4_LOW | QMC_IOEVENT_INPUT5_LOW | QMC_IOEVENT_INPUT6_LOW | QMC_IOEVENT_INPUT7_LOW);
	NVIC_SetPriority(BOARD_USER_BUTTON1_IRQ, 15); //TODO: adjust priority
    EnableIRQ(BOARD_USER_BUTTON1_IRQ);
	NVIC_SetPriority(BOARD_DIG_IN0_IRQ, 15);      //TODO: adjust priority
    EnableIRQ(BOARD_DIG_IN0_IRQ);
    NVIC_SetPriority(BOARD_DIG_IN2_IRQ, 15);      //TODO: adjust priority
    EnableIRQ(BOARD_DIG_IN2_IRQ);
    NVIC_SetPriority(BOARD_DIG_IN3_IRQ, 15);      //TODO: adjust priority
    EnableIRQ(BOARD_DIG_IN3_IRQ);

    DataloggerInit();

    /*Initialize network addresses*/
    if(kStatus_QMC_Ok != Init_network_addresses())
    {
    	goto error_task_init_failed;
    }

    /* Initialize RPC interface */
    RPC_Init();

#if FEATURE_FREEMASTER_ENABLE
    /* Initialize FreeMASTER */
	init_freemaster_lpuart();
	FMSTR_Init();
	if (pdPASS != xTaskCreate(FreemasterTask, "FreemasterTask", (8*configMINIMAL_STACK_SIZE), NULL,
			                  (configMAX_PRIORITIES-1), NULL))
	{
	  PRINTF("Task create failed! \r\n");
	  while (1); /* For debug only */
	}
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

	/* Create secure element session */
    if(kStatus_QMC_Ok != SE_Initialize())
		goto error_task_init_failed;
		
	/* Initialize IOT SDK with SE enablement */
    if(kStatus_QMC_Ok != SE_InitSecureSockets())
        goto error_task_init_failed;
	
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

    g_sMotorCmdTst.uSpeed_pos.sPosParam.bIsRandomPosition = false;
    g_sMotorCmdTst.eAppSwitch = kMC_App_Off;
    g_sMotorCmdTst.eMotorId = kMC_Motor3;
    g_sMotorCmdTst.eControlMethodSel = kMC_ScalarControl;
    g_sMotorCmdTst.uSpeed_pos.sScalarParam.fltScalarControlFrequency = 20;
    g_sMotorCmdTst.uSpeed_pos.sScalarParam.fltScalarControlVHzGain = 0.08;

    vTaskResume(g_fault_handling_task_handle);
    vTaskResume(g_board_service_task_handle);
    vTaskResume(g_datahub_task_handle);
    vTaskResume(g_local_service_task_handle);
#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
    vTaskResume(g_getMotorStatus_task_handle);
#endif

    vTaskDelete(NULL);
    return;

	error_task_init_failed:
	return; //TODO: add proper error handling
}

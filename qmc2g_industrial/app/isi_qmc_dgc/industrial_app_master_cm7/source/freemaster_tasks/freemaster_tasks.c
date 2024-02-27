/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
#include "stdint.h"
#include "fsl_lpuart.h"
#include "freemaster_tasks.h"
#include "api_fault.h"
#include "task.h"
#include "api_qmc_common.h"
#include "board.h"
#include "mlib_types.h"

fault_system_fault_t eSysFaultsRegistered;
mc_fault_t eMotor1FaultsRegistered;
mc_fault_t eMotor2FaultsRegistered;
mc_fault_t eMotor3FaultsRegistered;
mc_fault_t eMotor4FaultsRegistered;
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#if FEATURE_FREEMASTER_ENABLE
/*******************************************************************************
 * Prototypes
 ******************************************************************************/


/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint8_t u8FmstrTsaTemp = 0;
static bool_t bSendCmd;
TaskHandle_t g_freemaster_task_handle;

FMSTR_TSA_TABLE_BEGIN(first_table)
	FMSTR_TSA_RW_VAR(u8FmstrTsaTemp, FMSTR_TSA_UINT8)
FMSTR_TSA_TABLE_END()

FMSTR_TSA_TABLE_LIST_BEGIN()
  FMSTR_TSA_TABLE(first_table)
FMSTR_TSA_TABLE_LIST_END()

/*******************************************************************************
 * Code
 ******************************************************************************/


/*!
 * @brief FreemasterTask function definition
     FreeMASTER runs in short-interrupt mode. This task executes FMSTR_Poll() regularly and sends the motor command
     to command queue.

    MC_QueueMotorCommand() is invoked in FreemasterTask() to send motor commands to a command queue.
    (a) DataHubTask() is unblocked when command queue is not empty. Motor commands are fetched from the command queue, and then written into an internal memory.
    (b) Motor slow loop ISRs read this internal memory by MC_GetMotorCommand_fromISR(). Commands won't be updated (stay as previous one) if DataHubTask() is writing this internal memory.
 */
void FreemasterTask(void *pvParameters)
{
	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();
	while (1)
	{
	  u8FmstrTsaTemp++;

	  /* Set motor command test  - begin */
	  if(bSendCmd == TRUE)
	  {
		  eSetCmdStatus = MC_QueueMotorCommand(&g_sMotorCmdTst);
		  bSendCmd = FALSE;
	  }

	  /* Set motor command test  - end */

	  FMSTR_Poll();

	  eSysFaultsRegistered = FAULT_GetSystemFault_fromISR();
	  eMotor1FaultsRegistered = FAULT_GetMotorFault_fromISR(kMC_Motor1);
	  eMotor2FaultsRegistered = FAULT_GetMotorFault_fromISR(kMC_Motor2);
	  eMotor3FaultsRegistered = FAULT_GetMotorFault_fromISR(kMC_Motor3);
	  eMotor4FaultsRegistered = FAULT_GetMotorFault_fromISR(kMC_Motor4);

	  vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
	}
}

/*!
 * @brief LPUART Module initialization for FreeMASTER
 */
qmc_status_t init_freemaster_lpuart(void)
{
    lpuart_config_t config;
    qmc_status_t eStatus = kStatus_QMC_Err;

    /*
     * config.baudRate_Bps = 115200U;
     * config.parityMode = kUART_ParityDisabled;
     * config.stopBitCount = kUART_OneStopBit;
     * config.txFifoWatermark = 0;
     * config.rxFifoWatermark = 1;
     * config.enableTx = false;
     * config.enableRx = false;
     */
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = 115200U;
    config.enableTx = false;
    config.enableRx = false;

    if(kStatus_Success == LPUART_Init((LPUART_Type*)BOARD_FMSTR_UART_BASEADDR, &config, BOARD_FMSTR_UART_CLK_FREQ))
    {
    	eStatus = kStatus_QMC_Ok;
    }
    /* Register communication module used by FreeMASTER driver. */
    FMSTR_SerialSetBaseAddress((LPUART_Type*)BOARD_FMSTR_UART_BASEADDR);

#if FMSTR_SHORT_INTR || FMSTR_LONG_INTR
    /* Enable UART interrupts. */
    EnableIRQ(BOARD_FMSTR_UART_IRQ);
    NVIC_SetPriority(BOARD_FMSTR_UART_IRQ, 6);
#endif
    return eStatus;
}


/*!
 * @brief LPUART handler for FreeMASTER communication
 */
void BOARD_FMSTR_UART_IRQ_HANDLER (void)
{
#if FMSTR_SHORT_INTR || FMSTR_LONG_INTR
    /* Call FreeMASTER Interrupt routine handler */
    FMSTR_SerialIsr();
#endif
}
#endif


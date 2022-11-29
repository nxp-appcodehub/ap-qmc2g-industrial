/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/**
 * @file    qmc2g_industrial.c
 * @brief   Application entry point.
 */
#include "main_cm7.h"

/* TODO: insert other include files here. */
#include "FreeRTOS.h"
#include "fsl_lpi2c.h"

#include "fsl_soc_src.h"

/* TODO: insert other definitions and declarations here. */
__RAMFUNC(SRAM_ITC_cm7) void SoftwareHandler(void);
static void cm4Release(void);
void flexspi1_octal_bus_init();

static StaticEventGroup_t gs_inputButtonEventGroup;
static StaticEventGroup_t gs_systemStatusEventGroup;
EventGroupHandle_t g_inputButtonEventGroupHandle;
EventGroupHandle_t g_systemStatusEventGroupHandle;

/* mutex for shared access to the I2C bus */
SemaphoreHandle_t gSmComlock; /* name defined by hostlib / SMCOM layer */
static StaticSemaphore_t gs_i2cMutex;

/* I2C configuration for secure element and temperature sensor */
const lpi2c_master_config_t LPI2C3_masterConfig = {
  .enableMaster = true,
  .enableDoze = true,
  .debugEnable = false,
  .ignoreAck = false,
  .pinConfig = kLPI2C_2PinOpenDrain,
  .baudRate_Hz = 1000000UL, /* 1MHz is the maximum speed supported by PCT2075 */
  .busIdleTimeout_ns = 0UL,
  .pinLowTimeout_ns = 0UL,
  .sdaGlitchFilterWidth_ns = 0U,
  .sclGlitchFilterWidth_ns = 0U,
  .hostRequest = {
    .enable = false,
    .source = kLPI2C_HostRequestExternalPin,
    .polarity = kLPI2C_HostRequestPinActiveHigh
  }
};


volatile uint32_t ui32ClkADC;


/* declare UART handler for FreeMASTER */
#if FMSTR_UART_INDEX == 1
extern void LPUART1_IRQHandler(void);
#elif FMSTR_UART_INDEX == 6
extern void LPUART6_IRQHandler(void);
#elif FMSTR_UART_INDEX == 8
extern void LPUART8_IRQHandler(void);
#endif

extern TaskHandle_t g_fault_handling_task_handle;
extern TaskHandle_t g_board_service_task_handle;
extern TaskHandle_t g_datahub_task_handle;
extern TaskHandle_t g_local_service_task_handle;
extern TaskHandle_t g_getMotorStatus_task_handle;

/*
 * @brief   Application entry point.
 */
int main(void) {
	//TODO: cm4Release();
	/* Init board hardware. */
	BOARD_ConfigMPU();
	InstallIRQHandler(Reserved177_IRQn, (uint32_t)SoftwareHandler);
//	InstallIRQHandler(M1_fastloop_irq, (uint32_t)M1_fastloop_handler);
//	InstallIRQHandler(M2_fastloop_irq, (uint32_t)M2_fastloop_handler);
//	InstallIRQHandler(M3_fastloop_irq, (uint32_t)M3_fastloop_handler);
//	InstallIRQHandler(M4_fastloop_irq, (uint32_t)M4_fastloop_handler);
//	InstallIRQHandler(M1_slowloop_irq, (uint32_t)M1_slowloop_handler);
//	InstallIRQHandler(M2_slowloop_irq, (uint32_t)M2_slowloop_handler);
//	InstallIRQHandler(M3_slowloop_irq, (uint32_t)M3_slowloop_handler);
//	InstallIRQHandler(M4_slowloop_irq, (uint32_t)M4_slowloop_handler);
//	InstallIRQHandler(BOARD_FMSTR_UART_IRQ, (uint32_t)BOARD_FMSTR_UART_IRQ_HANDLER);

    /*
     * Reset the displaymix, otherwise during debugging, the
     * debugger may not reset the display, then the behavior
     * is not right.
     */
    SRC_AssertSliceSoftwareReset(SRC, kSRC_DisplaySlice);
    while (kSRC_SliceResetInProcess == SRC_GetSliceResetState(SRC, kSRC_DisplaySlice))
    {
    }

    BOARD_InitBootPins(); // TODO: exclude BOARD_InitLogFlexSPI1Pins after SBL integrated
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    ui32ClkADC = CLOCK_GetFreqFromObs(CCM_OBS_ADC1_CLK_ROOT);

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("QMC2G code started \r\n");

    /* Initialize Flexspi1 interface to enable access to OctalFlash and OctalRam. */
    /* Must be executed after the Debug console initialization. */
    flexspi1_octal_bus_init();
	
	/* Initialize I2C interface for secure element and temperature sensor */
	LPI2C_MasterInit(BOARD_GENERAL_I2C_BASEADDR, &LPI2C3_masterConfig, BOARD_GENERAL_I2C_CLOCK_FREQ);

	/* initialize event groups for system status and input events */
	g_systemStatusEventGroupHandle = xEventGroupCreateStatic(&gs_systemStatusEventGroup);
	g_inputButtonEventGroupHandle  = xEventGroupCreateStatic(&gs_inputButtonEventGroup);
	if( (NULL == g_systemStatusEventGroupHandle) || (NULL == g_inputButtonEventGroupHandle) )
	{
		PRINTF("Event group creation failed!.\r\n");
		return -1;
	}

	/* initialize mutex for shared access to the I2C bus */
	gSmComlock = xSemaphoreCreateMutexStatic(&gs_i2cMutex);
    if(NULL == gSmComlock)
    {
    	PRINTF("Failed to create mutex for I2C access.\r\n");
    	return -1;
    }

    if (pdPASS != xTaskCreate(StartupTask, "StartupTask", (configMINIMAL_STACK_SIZE+1024), NULL,
    		                  ((configMAX_PRIORITIES-3)), NULL))
    {
    	PRINTF("Failed to create task: StartupTask.\r\n");
    	return -1;
    }


#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
	#if( (FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB != 0) && (FEATURE_FREEMASTER_ENABLE == 0) )
		#error "The motor status update task feature requires FreeMASTER to be enabled."
	#endif
    // TODO: adjust stack size and priority. The priority of DataHubTask must be larger than getMotorStatusTask, otherwise getMotorStatusTask may fail at registering the queue.
    if (pdPASS != xTaskCreate(getMotorStatusTask, "getMotorStatusTask", (8*configMINIMAL_STACK_SIZE), NULL,
    		                  ((tskIDLE_PRIORITY+1)), &g_getMotorStatus_task_handle))
    {
    	PRINTF("Get motor status Task create failed!\r\n");
    	return -1;
    }
#endif


    if (pdPASS != xTaskCreate(FaultHandlingTask, "FaultHandlingTask", (3 * configMINIMAL_STACK_SIZE), NULL,
        		              ((tskIDLE_PRIORITY+2)), &g_fault_handling_task_handle))
	{
		PRINTF("Task create failed!.\r\n");
		return -1;
	}

    if (pdPASS != xTaskCreate(BoardServiceTask, "BoardServiceTask", (2 * configMINIMAL_STACK_SIZE), NULL,
            		          ((tskIDLE_PRIORITY+1)), &g_board_service_task_handle))
	{
		PRINTF("Task create failed!.\r\n");
		return -1;
	}

    // TODO: adjust stack size and priority
    if (pdPASS != xTaskCreate(DataHubTask, "DataHub", (3 * configMINIMAL_STACK_SIZE), NULL,
    		                  ((tskIDLE_PRIORITY+2)), &g_datahub_task_handle))
    {
    	PRINTF("Failed to create task: DataHub.\r\n");
    	return -1;
    }

    if (pdPASS != xTaskCreate(LocalServiceTask, "LocalServiceTask", (8 * configMINIMAL_STACK_SIZE), NULL,
    		                  ((tskIDLE_PRIORITY+1)), &g_local_service_task_handle))
    {
    	PRINTF("Failed to create task: LocalServiceTask.\r\n");
    	return -1;
    }
	
	vTaskSuspend(g_fault_handling_task_handle);
	vTaskSuspend(g_board_service_task_handle);
	vTaskSuspend(g_datahub_task_handle);
	vTaskSuspend(g_local_service_task_handle);
#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
	vTaskSuspend(g_getMotorStatus_task_handle);
#endif

    vTaskStartScheduler();

    return 0;
}

void SoftwareHandler(void)
{
}



static void cm4Release(void)
{
  PRINTF("Releasing CM4\r\n");
  IOMUXC_LPSR_GPR->GPR0 = ((uint32_t)(0x0000FFFF&(uint32_t)CORE1_REAL_BOOT_ADDRESS));
  IOMUXC_LPSR_GPR->GPR1 = IOMUXC_LPSR_GPR_GPR1_CM4_INIT_VTOR_HIGH((uint32_t)CORE1_REAL_BOOT_ADDRESS >> 16);

  SRC->CTRL_M4CORE = SRC_CTRL_M4CORE_SW_RESET_MASK;
  SRC->SCR |= SRC_SCR_BT_RELEASE_M4_MASK;
}

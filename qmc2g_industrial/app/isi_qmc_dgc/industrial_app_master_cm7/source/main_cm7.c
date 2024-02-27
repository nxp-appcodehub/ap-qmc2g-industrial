/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/**
 * @file    qmc2g_industrial.c
 * @brief   Application entry point.
 */
#include "main_cm7.h"

#include "FreeRTOS.h"
#include "fsl_lpi2c.h"

#include "fsl_soc_src.h"
#include "webservice/webservice_logging_task.h"

#if defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".fw_hdr"), used))
#elif defined(__ICCARM__)
#pragma location = ".fw_hdr"
#endif
/*************************************
 *  FW's Header Data
 *************************************/
const header_t fw_hdr = {
    MAIN_FW_HEADER_MAGIC,
    (uint32_t)MAIN_FW_VERSION,
    FW_DATA_ADDRESS,
    FW_DATA_SIZE,
    FW_SIGN_ADDRESS,
    FW_DATA_CM4_ADDRESS,
    FW_DATA_CM4_SIZE,
    CM4_CORE_BOOT_ADDRESS,
    CM7_VECTOR_TABLE_ADDRESS,
    FW_CFGDATA_ADDRESS,
    FW_CFGDATA_SIZE,
};

__RAMFUNC(SRAM_ITC_cm7) void SoftwareHandler(void);
extern void flexspi1_octal_bus_init();

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

/* QMC task priorities */
#define QMC_TASK_PRIO_NORMAL   (tskIDLE_PRIORITY+1)
#define QMC_TASK_PRIO_ELEVATED (tskIDLE_PRIORITY+2)
#define QMC_TASK_PRIO_HIGH     (configMAX_PRIORITIES-1)

/* QMC task stack sizes */
#define QMC_TASK_STACKSIZE_STARTUP               (21 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_DATALOGGER            (19 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_FREEMASTER            ( 1 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_GETMOTORSTATUS        ( 1 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_FAULTHANDLING         ( 2 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_BOARDSERVICE          ( 2 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_DATAHUB               ( 1 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_LOCALSERVICE          ( 6 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_JSONMOTORAPISERVICE   ( 1 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_WEBSERVICELOGGING     ( 2 * configMINIMAL_STACK_SIZE)
#define QMC_TASK_STACKSIZE_CLOUDSERVICE          (27 * configMINIMAL_STACK_SIZE)
/* around 3016 bytes of stack are used, 440 stay unused */
#define QMC_TASK_STACKSIZE_AWDGCONNECTIONSERVICE (27 * configMINIMAL_STACK_SIZE)

extern TaskHandle_t g_fault_handling_task_handle;
extern TaskHandle_t g_board_service_task_handle;
extern TaskHandle_t g_datahub_task_handle;
extern TaskHandle_t g_datalogger_task_handle;
extern TaskHandle_t g_local_service_task_handle;
extern TaskHandle_t g_freemaster_task_handle;
extern TaskHandle_t g_getMotorStatus_task_handle;
extern TaskHandle_t g_webservice_logging_task_handle;
extern TaskHandle_t g_json_motor_api_service_task_handle;
extern TaskHandle_t g_cloud_service_task_handle;
extern TaskHandle_t g_awdg_connection_service_task_handle;

/* static task TCBs and stacks */
static StaticTask_t gs_fault_handling_task;
static StaticTask_t gs_board_service_task;
static StaticTask_t gs_datahub_task;
static StaticTask_t gs_datalogger_task;
static StaticTask_t gs_local_service_task;
static StaticTask_t gs_json_motor_api_service_task;
static StaticTask_t gs_webservice_logging_task;
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_fault_handling_task_stack[QMC_TASK_STACKSIZE_FAULTHANDLING];
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_board_service_task_stack[QMC_TASK_STACKSIZE_BOARDSERVICE];
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_datahub_task_stack[QMC_TASK_STACKSIZE_DATAHUB];
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_datalogger_task_stack[QMC_TASK_STACKSIZE_DATALOGGER];
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_local_service_task_stack[QMC_TASK_STACKSIZE_LOCALSERVICE];
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_json_motor_api_service_task_stack[QMC_TASK_STACKSIZE_JSONMOTORAPISERVICE];
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_json_webservice_logging_task_stack[QMC_TASK_STACKSIZE_WEBSERVICELOGGING];
#if FEATURE_FREEMASTER_ENABLE
static StaticTask_t gs_freemaster_task;
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_freemaster_task_stack[QMC_TASK_STACKSIZE_FREEMASTER];
#endif
#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
static StaticTask_t gs_getMotorStatus_task;
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_getMotorStatus_task_stack[QMC_TASK_STACKSIZE_GETMOTORSTATUS];
#endif
#if FEATURE_CLOUD_AZURE_IOTHUB || FEATURE_CLOUD_GENERIC_MQTT
static StaticTask_t gs_cloud_service_task;
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_cloud_service_task_stack[QMC_TASK_STACKSIZE_CLOUDSERVICE];
#endif
#if FEATURE_SECURE_WATCHDOG
static StaticTask_t gs_awdg_connection_service_task;
__attribute__((section(".bss.$SRAM_OC1"))) static StackType_t  gs_awdg_connection_service_task_stack[QMC_TASK_STACKSIZE_AWDGCONNECTIONSERVICE];
#endif

/*
 * @brief   Application entry point.
 */
int main(void) {
	/* Init board hardware. */
	BOARD_ConfigMPU();
	InstallIRQHandler(Reserved177_IRQn, (uint32_t)SoftwareHandler);

    /*
     * Reset the displaymix, otherwise during debugging, the
     * debugger may not reset the display, then the behavior
     * is not right.
     */
#ifdef NO_SBL
    SRC_AssertSliceSoftwareReset(SRC, kSRC_DisplaySlice);
    while (kSRC_SliceResetInProcess == SRC_GetSliceResetState(SRC, kSRC_DisplaySlice))
    {
    }
#endif
    BOARD_InitBootPins();
#ifdef NO_SBL
    /* If this function must be used along with SBL, be aware that access to the DCDC is blocked by RDC configuration done in SBL.
     * Any access to DCDC peripheral must be avoided.
     *  */
    BOARD_InitBootClocks();
#else
    /* if the SBL is used, then the SystemCoreClock variable must be set (normally done in BOARD_InitBootClocks()) */
    SystemCoreClock = CLOCK_GetRootClockFreq(kCLOCK_Root_M7);
#endif
    BOARD_InitBootPeripherals();

    ui32ClkADC = CLOCK_GetFreqFromObs(CCM_OBS_ADC1_CLK_ROOT);

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    PRINTF("QMC2G code started \r\n");

#ifdef NO_SBL
    /* Initialize Flexspi1 interface to enable access to OctalFlash and OctalRam. */
    /* Must be executed after the Debug console initialization. */
    flexspi1_octal_bus_init();
#endif
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

    /* create tasks */
    if (pdPASS != xTaskCreate(StartupTask, "StartupTask", (QMC_TASK_STACKSIZE_STARTUP), NULL,
    		                  (QMC_TASK_PRIO_HIGH), NULL))
    {
    	PRINTF("Failed to create task: StartupTask.\r\n");
    	return -1;
    }

    /* return values are not checked as static creation can not fail if buffers are not NULL 
     * see www.freertos.org/xTaskCreateStatic.html */
	g_datalogger_task_handle = xTaskCreateStatic(DataloggerTask, "DataloggerTask", QMC_TASK_STACKSIZE_DATALOGGER, NULL,
												QMC_TASK_PRIO_ELEVATED, gs_datalogger_task_stack, &gs_datalogger_task);
	g_fault_handling_task_handle = xTaskCreateStatic(FaultHandlingTask, "FaultHandlingTask", QMC_TASK_STACKSIZE_FAULTHANDLING, NULL,
			                                         QMC_TASK_PRIO_ELEVATED, gs_fault_handling_task_stack, &gs_fault_handling_task);
	g_board_service_task_handle = xTaskCreateStatic(BoardServiceTask, "BoardServiceTask", QMC_TASK_STACKSIZE_BOARDSERVICE, NULL,
			                                        QMC_TASK_PRIO_NORMAL, gs_board_service_task_stack, &gs_board_service_task);
	g_datahub_task_handle = xTaskCreateStatic(DataHubTask, "DataHub", QMC_TASK_STACKSIZE_DATAHUB, NULL,
			                                  QMC_TASK_PRIO_ELEVATED, gs_datahub_task_stack, &gs_datahub_task);
    g_local_service_task_handle = xTaskCreateStatic(LocalServiceTask, "LocalServiceTask", QMC_TASK_STACKSIZE_LOCALSERVICE, NULL,
    		                                        QMC_TASK_PRIO_NORMAL, gs_local_service_task_stack, &gs_local_service_task);
    g_json_motor_api_service_task_handle = xTaskCreateStatic(JsonMotorAPIServiceTask, "JSONMotorAPIServiceTask", QMC_TASK_STACKSIZE_JSONMOTORAPISERVICE, NULL,
    		                                                 QMC_TASK_PRIO_NORMAL, gs_json_motor_api_service_task_stack, &gs_json_motor_api_service_task);
    g_webservice_logging_task_handle = xTaskCreateStatic(WebserviceLoggingTask, "WebserviceLoggingTask", QMC_TASK_STACKSIZE_WEBSERVICELOGGING, NULL,
    		                                                 QMC_TASK_PRIO_NORMAL, gs_json_webservice_logging_task_stack, &gs_webservice_logging_task);
#if FEATURE_FREEMASTER_ENABLE
	g_freemaster_task_handle = xTaskCreateStatic(FreemasterTask, "FreemasterTask", QMC_TASK_STACKSIZE_FREEMASTER, NULL,
			                                     QMC_TASK_PRIO_HIGH, gs_freemaster_task_stack, &gs_freemaster_task);
#endif
#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
	#if( (FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB != 0) && (FEATURE_FREEMASTER_ENABLE == 0) )
		#error "The motor status update task feature requires FreeMASTER to be enabled."
	#endif
    /* The priority of DataHubTask must be higher than getMotorStatusTask, otherwise getMotorStatusTask may fail at registering the queue. */
	g_getMotorStatus_task_handle = xTaskCreateStatic(getMotorStatusTask, "getMotorStatusTask", QMC_TASK_STACKSIZE_GETMOTORSTATUS, NULL,
			                                         QMC_TASK_PRIO_NORMAL, gs_getMotorStatus_task_stack, &gs_getMotorStatus_task);
#endif
#if FEATURE_CLOUD_AZURE_IOTHUB || FEATURE_CLOUD_GENERIC_MQTT
    g_cloud_service_task_handle = xTaskCreateStatic(CloudServiceTask, "CloudServiceTask", QMC_TASK_STACKSIZE_CLOUDSERVICE, NULL,
    		                                        QMC_TASK_PRIO_NORMAL, gs_cloud_service_task_stack, &gs_cloud_service_task);
#endif
#if FEATURE_SECURE_WATCHDOG
    g_awdg_connection_service_task_handle = xTaskCreateStatic(AwdgConnectionServiceTask, "AwdgConnectionServiceTask", QMC_TASK_STACKSIZE_AWDGCONNECTIONSERVICE, NULL,
    		                                                  QMC_TASK_PRIO_NORMAL, gs_awdg_connection_service_task_stack, &gs_awdg_connection_service_task);
#endif

	vTaskSuspend(g_fault_handling_task_handle);
	vTaskSuspend(g_board_service_task_handle);
	vTaskSuspend(g_datahub_task_handle);
	vTaskSuspend(g_datalogger_task_handle);
	vTaskSuspend(g_local_service_task_handle);
	vTaskSuspend(g_webservice_logging_task_handle);
	vTaskSuspend(g_json_motor_api_service_task_handle);
#if FEATURE_FREEMASTER_ENABLE
	vTaskSuspend(g_freemaster_task_handle);
#endif
#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
	vTaskSuspend(g_getMotorStatus_task_handle);
#endif
#if FEATURE_CLOUD_AZURE_IOTHUB || FEATURE_CLOUD_GENERIC_MQTT
    vTaskSuspend(g_cloud_service_task_handle);
#endif
#if FEATURE_SECURE_WATCHDOG
    vTaskSuspend(g_awdg_connection_service_task_handle);
#endif

    vTaskStartScheduler();

    return 0;
}

void SoftwareHandler(void)
{
}

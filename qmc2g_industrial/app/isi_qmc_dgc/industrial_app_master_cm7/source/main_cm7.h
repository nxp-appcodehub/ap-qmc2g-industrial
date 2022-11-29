/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#ifndef __main_cm7_h_
#define __main_cm7_h_

#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------------------------
// Linker macros placing include file
//-----------------------------------------------------------------------
#include <cr_section_macros.h>

//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1176_cm7.h"
#include "fsl_debug_console.h"
#include "fsl_lpuart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"

#include "fault_handling_tasks.h"
#include "board_service_tasks.h"
#include "local_service_tasks.h"
#include "datalogger_tasks.h"
#include "freemaster.h"
#include "freemaster_tsa.h"
#include "motor_control_task.h"
#include "mcinit.h"

//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Typedef
//------------------------------------------------------------------------------
 
//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define CORE1_REAL_BOOT_ADDRESS (void *)0x20200000

//------------------------------------------------------------------------------     
// Function Prototypes                                                          
//------------------------------------------------------------------------------

extern void DataHubTask(void *pvParameters);

// Init task definition
extern void StartupTask(void *pvParameters);

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
extern EventGroupHandle_t g_inputButtonEventGroupHandle;
extern EventGroupHandle_t g_systemStatusEventGroupHandle;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __main_cm7_h_ */

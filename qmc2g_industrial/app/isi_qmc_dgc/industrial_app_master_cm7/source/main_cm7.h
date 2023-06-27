/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
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
// Extern
//------------------------------------------------------------------------------
extern uint32_t  __FW_CM4_START[];
extern uint32_t  __FW_CM4_SIZE[];
extern uint32_t  __VECTOR_TABLE[];
extern uint32_t  __FW_HEADER_START[];
extern uint32_t  __FW_SIZE[];
extern uint32_t  __FW_SIGN_START[];
extern uint32_t  __FW_CFGDATA_START[];
extern uint32_t  __FW_CFGDATA_SIZE[];
extern uint32_t  __LOG_STORAGE_STAR[];
extern uint32_t  __LOG_STORAGE_SIZE[];
extern uint32_t  __FWU_STORAGE_START[];
extern uint32_t  __FWU_STORAGE_SIZE[];
//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Typedef
//------------------------------------------------------------------------------
/*
 *  The main Firmware header.
 *  Must be paced at the beginning of the image with size 256 Bytes.
 *  */
typedef struct _header
{
	uint32_t magic;				/* Magic number 0x456D1BE8U */
	uint32_t version;			/* FW version */
	uint32_t fwDataAddr;		/* FW start address - from FW header */
	uint32_t fwDataLength;		/* FW data length - up to signature*/
	uint32_t signDataAddr;		/* FW signature start */
	uint32_t cm4FwDataAddr;		/* CM4 image start address */
	uint32_t cm4FwDataLength; 	/* CM4 image length */
	uint32_t cm4BootAddr;		/* CM4 boot address */
	uint32_t cm7VectorTableAddr;/* CM7 Vector Table address */
	uint32_t cfgDataAddr;		/* Configuration Data start address in Octal Flash */
	uint32_t cfgDataLength;		/* Configuration Data length */
} header_t;

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
/* Main FW header MAGIC. */
#define MAIN_FW_HEADER_MAGIC				0x456D1BE8U
/* Main FW Version. */
#define MAIN_FW_VERSION						MAKE_VERSION(1,0,0)
/* CM4 Boot project. */
#define CM4_CORE_BOOT_ADDRESS 				(uint32_t)0x20200000
/* Address of the CM4 image. */
#define FW_DATA_CM4_ADDRESS					(uint32_t)__FW_CM4_START
/* CM4 image size. */
#define FW_DATA_CM4_SIZE					(uint32_t)__FW_CM4_SIZE
/* CM7 Vector Table address. */
#define CM7_VECTOR_TABLE_ADDRESS			(uint32_t)__VECTOR_TABLE
/* Main FW data address in the external memory. */
#define FW_DATA_ADDRESS						(uint32_t)__FW_HEADER_START
/* Main FW data size including FW header, CM7 and CM4 images. */
#define FW_DATA_SIZE						(uint32_t)__FW_SIZE
/* Main FW signature pointer. */
#define FW_SIGN_ADDRESS						(uint32_t)__FW_SIGN_START
/* Configuration data address. */
#define FW_CFGDATA_ADDRESS					(uint32_t)__FW_CFGDATA_START
/* Configuration data size. */
#define FW_CFGDATA_SIZE					    (uint32_t)__FW_CFGDATA_SIZE
/* Log storage address. */
#define LOG_STORAGE_ADDRESS					(uint32_t)__LOG_STORAGE_STAR
/* Log storage size. */
#define LOG_STORAGE_SIZE					(uint32_t)__LOG_STORAGE_SIZE
/* Firmware Update storage address. */
#define FWU_STORAGE_ADDRESS					(uint32_t)__FWU_STORAGE_START
/* Firmware Update storage size. */
#define FWU_STORAGE_SIZE					(uint32_t)__FWU_STORAGE_SIZE
//------------------------------------------------------------------------------     
// Function Prototypes                                                          
//------------------------------------------------------------------------------

extern void DataHubTask(void *pvParameters);

// Init task definition
extern void StartupTask(void *pvParameters);

extern void AwdgConnectionServiceTask(void *pvParameters);

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
extern EventGroupHandle_t g_inputButtonEventGroupHandle;
extern EventGroupHandle_t g_systemStatusEventGroupHandle;

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __main_cm7_h_ */

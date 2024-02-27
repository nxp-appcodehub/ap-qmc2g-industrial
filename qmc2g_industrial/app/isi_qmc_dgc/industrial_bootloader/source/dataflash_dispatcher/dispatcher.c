/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
 
#include "fsl_flexspi.h"
#include "app.h"
#include "fsl_debug_console.h"
//#include "semphr.h"

//#include "api_qmc_common.h"
#include "qmc2_types.h"
#include "qmc2_flash.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
qmc_status_t dispatcher_read_memory( void *pdst, void *psrc, size_t size, TickType_t ticks)
{
	//Need to validate the address range of flash!
	memcpy( pdst, psrc, size);
	return kStatus_QMC_Ok;
}

qmc_status_t dispatcher_write_memory( void *pdst, void *psrc, size_t size, TickType_t ticks)
{
	//Size of data must be even. Octalflash does not support odd sized data.
	if( ( size & 1))
	{
		dbgDispPRINTF("DispatcherWriteMemory. Err, try to write odd size data to dataflash. %d\n\r", size);
		return kStatus_QMC_ErrArgInvalid;
	}

	if (QMC2_FLASH_ProgramPages((uint32_t) pdst, psrc, size) != kStatus_SSS_Success)
	{
		return kStatus_QMC_Err;
	}

	return kStatus_QMC_Ok;
}

qmc_status_t dispatcher_erase_sectors(void *pdst, uint16_t sect_cn, TickType_t ticks)
{
	if (QMC2_FLASH_Erase((uint32_t) pdst, (uint32_t)(sect_cn * OCTAL_FLASH_SECTOR_SIZE)) != kStatus_SSS_Success)
	{
		return kStatus_QMC_Err;
	}

	return kStatus_QMC_Ok;
}

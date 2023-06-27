/*
 * Copyright 2022 NXP 
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
#include "semphr.h"

#include "api_qmc_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
status_t flexspi_nor_octalflash_write_data(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer, size_t size);
status_t flexspi_nor_octalflash_erase_sector(FLEXSPI_Type *base, uint32_t addr);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static StaticSemaphore_t gs_flashMutex;
SemaphoreHandle_t g_flash_xSemaphore = NULL;

/*******************************************************************************
 * Code
 ******************************************************************************/

qmc_status_t dispatcher_init()
{
	if( g_flash_xSemaphore == NULL)
	{
		g_flash_xSemaphore = xSemaphoreCreateMutexStatic( &gs_flashMutex);
		if( g_flash_xSemaphore == NULL)
		{
			return kStatus_QMC_Err;
		}
	}
	return kStatus_QMC_Ok;
}

//Blocking function to get dedicated access to flash
qmc_status_t dispatcher_get_flash_lock( TickType_t ticks)
{
	if( g_flash_xSemaphore != NULL)
	if( xSemaphoreTake( g_flash_xSemaphore, ticks ) == pdTRUE )
	{
		return kStatus_QMC_Ok;
	}

	return kStatus_QMC_ErrBusy;
}

qmc_status_t dispatcher_release_flash_lock()
{
	if( xSemaphoreGive( g_flash_xSemaphore ) != pdTRUE )
	{
		return kStatus_QMC_Err;
	}

	return kStatus_QMC_Ok;
}

qmc_status_t dispatcher_read_memory( void *pdst, void *psrc, size_t size, TickType_t ticks)
{
	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		//Need to validate the address range of flash!
		memcpy( pdst, psrc, size);
		return dispatcher_release_flash_lock();
	}
	return kStatus_QMC_ErrBusy;
}

qmc_status_t dispatcher_write_memory( void *pdst, void *psrc, size_t size, TickType_t ticks)
{
	//Size of data must be even. Octalflash does not support odd sized data.
	if( ( size & 1))
	{
		dbgDispPRINTF("DispatcherWriteMemory. Err, try to write odd size data to dataflash. %d\n\r", size);
		return kStatus_QMC_ErrArgInvalid;
	}

	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		//Need to validate the address range of flash!
		uint32_t flashAddr = (uint32_t)pdst - FlexSPI1_AMBA_BASE;
		uint8_t *flashSrc = (uint8_t *)psrc;

		while( size)
		{
			size_t s = OCTAL_FLASH_PAGE_SIZE - flashAddr % OCTAL_FLASH_PAGE_SIZE;
			if( s > size )
			{
				s = size;
			}
			if( flexspi_nor_octalflash_write_data( FLEXSPI1, flashAddr, (void *)flashSrc, s) != kStatus_Success)
			{
				dispatcher_release_flash_lock();
				return kStatus_QMC_Err;
			}
			flashAddr+=s;
			flashSrc+=s;
			size-=s;
		}
		return dispatcher_release_flash_lock();
	}
	return kStatus_QMC_ErrBusy;
}

qmc_status_t dispatcher_erase_sectors( void *pdst, uint16_t sect_cn, TickType_t ticks)
{
	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		//Need to validate the address range of flash!
		uint32_t flashAddr = (uint32_t)pdst - FlexSPI1_AMBA_BASE;
		for(; sect_cn; sect_cn--)
		{
			if( flexspi_nor_octalflash_erase_sector( FLEXSPI1, flashAddr) != kStatus_Success)
			{
				dispatcher_release_flash_lock();
				return kStatus_QMC_Err;
			}
			flashAddr+=OCTAL_FLASH_SECTOR_SIZE;
		}
		return dispatcher_release_flash_lock();
	}
	return kStatus_QMC_ErrBusy;
}

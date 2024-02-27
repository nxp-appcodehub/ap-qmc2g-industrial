/*
 * Copyright 2022-2023 NXP 
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
		memcpy( pdst, psrc, size);
		return dispatcher_release_flash_lock();
	}
	return kStatus_QMC_ErrBusy;
}

qmc_status_t dispatcher_write_memory( void *pdst, void *psrc, size_t size, TickType_t ticks)
{
	//Size of data must be even. Octalflash does not support odd sized data.
	if( ( size & 1U))
	{
		dbgDispPRINTF("DispatcherWriteMemory1. Err, try to write odd size data to dataflash. %d\n\r", size);
		return kStatus_QMC_ErrArgInvalid;
	}

	if( (uint32_t)psrc & 1U)
	{
		dbgDispPRINTF("DispatcherWriteMemory2. Err, try to write from the psrc not aligned to uint32_t to dataflash. %X\n\r", (uint32_t)psrc);
		return kStatus_QMC_ErrArgInvalid;
	}

	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		status_t retv = kStatus_Fail;
		uint32_t flashAddr = (uint32_t)pdst - FlexSPI1_AMBA_BASE;
		if( flashAddr >= OCTAL_FLASH_SIZE)
		{
			dbgDispPRINTF("DispatcherWriteMemory5. Err invalid address. FA:0x%x pdst:0x%x", flashAddr, (uint32_t)pdst);
			retv = dispatcher_release_flash_lock();
			if( retv != kStatus_Success)
			{
				dbgDispPRINTF("DispatcherWriteMemory5. Err release lock:%d", retv);
				retv = MAKE_STATUS( 196, retv);
			}
			return retv;
		}
		uint8_t *flashSrc = (uint8_t *)psrc;

		//To be sure we can safely retype flasSrc from uint8_t* to uint32_t* we do this test:
		if( MAKE_NUMBER_ALIGN( (uint32_t)flashSrc, 2) != (uint32_t)flashSrc)
		{
			dbgDispPRINTF("DispatcherWriteMemory6. Err invalid address. flashSrc:0x%x", (uint32_t)flashSrc);
			retv = dispatcher_release_flash_lock();
			if( retv != kStatus_Success)
			{
				dbgDispPRINTF("DispatcherWriteMemory6. Err release lock:%d", retv);
				retv = MAKE_STATUS( 197, retv);
			}
			return retv;
		}

		while( size)
		{
			size_t s = OCTAL_FLASH_PAGE_SIZE - flashAddr % OCTAL_FLASH_PAGE_SIZE;
			if( s > size )
			{
				s = size;
			}
			//We can safely retype flashSrc pointer to uint32_t* because it was already tested to be uint32_t* aligned
			retv = flexspi_nor_octalflash_write_data( FLEXSPI1, flashAddr, (uint32_t *)flashSrc, s);
			if( retv != kStatus_Success)
			{
				dbgDispPRINTF("DispatcherWriteMemory3. Err write data:%d FA:0x%x SR:0x%x S:0x%x", retv, flashAddr, flashSrc, s);
				retv = dispatcher_release_flash_lock();
				if( retv != kStatus_Success)
				{
					dbgDispPRINTF("DispatcherWriteMemory4. Err release lock:%d", retv);
					retv = MAKE_STATUS( 190, retv);
				}
				return retv;
			}
			flashAddr+=s;
			flashSrc+=s;
			size-=s;
		}
		retv = dispatcher_release_flash_lock();
		if( retv != kStatus_Success)
		{
			dbgDispPRINTF("DispatcherWriteMemory4. Err release lock:%d", retv);
			retv = MAKE_STATUS( 191, retv);
		}
		return retv;
	}
	return kStatus_QMC_ErrBusy;
}

qmc_status_t dispatcher_erase_sectors( void *pdst, uint16_t sect_cn, TickType_t ticks)
{
	status_t retv = kStatus_Fail;
	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		uint32_t flashAddr = (uint32_t)pdst - FlexSPI1_AMBA_BASE;
		if( flashAddr >= OCTAL_FLASH_SIZE)
		{
			dbgDispPRINTF("DispatcherEraseSector4. Err erase sector. FA:0x%x pdst:0x%x", flashAddr, (uint32_t)pdst);
			retv = dispatcher_release_flash_lock();
			if( retv != kStatus_Success)
			{
				dbgDispPRINTF("DispatcherEraseSector4. Err release lock:%d", retv);
				retv = MAKE_STATUS( 194, retv);
			}
			return retv;
		}
		for(; sect_cn; sect_cn--)
		{
			retv = flexspi_nor_octalflash_erase_sector( FLEXSPI1, flashAddr);
			if( retv != kStatus_Success)
			{
				dbgDispPRINTF("DispatcherEraseSector1. Err erase sector:%d FA:0x%x", retv, flashAddr);
				retv = dispatcher_release_flash_lock();
				if( retv != kStatus_Success)
				{
					dbgDispPRINTF("DispatcherEraseSector2. Err release lock:%d", retv);
					retv = MAKE_STATUS( 192, retv);
				}
				return retv;
			}
			flashAddr+=OCTAL_FLASH_SECTOR_SIZE;
		}
		retv = dispatcher_release_flash_lock();
		if( retv != kStatus_Success)
		{
			dbgDispPRINTF("DispatcherEraseSector3. Err release lock:%d", retv);
			retv = MAKE_STATUS( 193, retv);
		}
		return retv;
	}
	return kStatus_QMC_ErrBusy;
}

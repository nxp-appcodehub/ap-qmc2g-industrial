/*
 * Copyright 2019-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "fsl_sd.h"
#include "fsl_debug_console.h"
#include "ff.h"
#include "diskio.h"
#include "fsl_sd_disk.h"
#include "sdmmc_config.h"
#include "app.h"
#include "fsl_common.h"
#include "api_qmc_common.h"
#include "api_board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
//#define SDCARD_POSITIVE_DEBUG

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
bool BOARD_SDCardGetDetectStatus(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static FATFS g_fileSystem; /* File system object */
static FIL g_fileObject1;  /* File object */

/*******************************************************************************
 * Code
 ******************************************************************************/

qmc_status_t SDCard_MountVolume(void)
{
    FRESULT error;
    const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};

#ifdef DATALOGGER_SDCARD_FATFS_DELAYED_MOUNT
    if (f_mount(&g_fileSystem, driverNumberBuffer, 0U))
#else
    if (f_mount(&g_fileSystem, driverNumberBuffer, 1U))
#endif
    {
        dbgSDcPRINTF("Mount volume failed.\r\n");
        return kStatus_QMC_Err;
    }

#if (FF_FS_RPATH >= 2U)
    error = f_chdrive((char const *)&driverNumberBuffer[0U]);
    if (error)
    {
        dbgSDcPRINTF("Change drive failed.\r\n");
        return kStatus_QMC_Err;
    }
#endif

    return kStatus_QMC_Ok;
}

qmc_status_t SDCard_UnMountVolume(void)
{
    const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};
    if (f_mount( NULL, driverNumberBuffer, 1U))
    {
        dbgSDcPRINTF("UnMount volume failed.\r\n");
        return kStatus_QMC_Err;
    }

    return kStatus_QMC_Ok;
}

qmc_status_t SDCard_WriteRecord( const char *dir_path, const char *file_path, uint8_t *buf, size_t buf_len)
{
    UINT bytesWritten   = 0U;
    FRESULT error;

	error = f_open(&g_fileObject1, _T( file_path), FA_WRITE);
	if( error)
	{
		error = f_mkdir(_T( dir_path));
		if (error)
		{
			if (error != FR_EXIST)
			{
				dbgSDcPRINTF("Make directory failed.\r\n");
				return kStatus_QMC_Err;
			}
		}
		error = f_open(&g_fileObject1, _T( file_path), FA_WRITE);
	}

	if (error)
	{
		if (error == FR_EXIST)
		{
			dbgSDcPRINTF("File exists.\r\n");
		}
		/* if file not exist, creat a new file */
		else if (error == FR_NO_FILE)
		{
			if (f_open(&g_fileObject1, _T( file_path), (FA_WRITE | FA_CREATE_NEW)) != FR_OK)
			{
				dbgSDcPRINTF("Create file failed.\r\n");
				return kStatus_QMC_Err;
			}
		}
		else
		{
			dbgSDcPRINTF("Open file failed.\r\n");
			return kStatus_QMC_Err;
		}
	}
	/* write append */
	if (f_lseek(&g_fileObject1, g_fileObject1.obj.objsize) != FR_OK)
	{
		dbgSDcPRINTF("lseek file failed.\r\n");
		return kStatus_QMC_Err;
	}

	error = f_write(&g_fileObject1, buf, buf_len, &bytesWritten);
	if ((error) || (bytesWritten != buf_len))
	{
		dbgSDcPRINTF("Write file failed.\r\n");
		return kStatus_QMC_Err;
	}
	f_close(&g_fileObject1);
	return kStatus_QMC_Ok;
}

qmc_status_t Handle_file( const char * dir_path, const char *file_path)
{
	FILINFO fno;
	char buf[32];
	qmc_timestamp_t	ts = {0,0};
	qmc_datetime_t dt;

	FRESULT fresult = f_stat( _T( file_path), &fno);
	if( fresult != FR_OK)
	{
		dbgSDcPRINTF("fstat file failed.\r\n");
		return kStatus_QMC_Err;
	}
#ifdef SDCARD_POSITIVE_DEBUG
	dbgSDcPRINTF("fsize:%d\n\r", fno.fsize);
#endif
	if( fno.fsize >= DATALOGGER_SDCARD_MAX_FILESIZE)
	{
		memset( &dt, 0, sizeof(qmc_datetime_t));
		if( BOARD_GetTime( &ts) == kStatus_QMC_Ok)
			BOARD_ConvertTimestamp2Datetime( &ts, &dt);	//return status currently doesn't care
		sprintf( buf, "%s/%02d%02d%02d%02d.bin", dir_path, dt.year%100, dt.month%100, dt.day%100, dt.hour%100 );
		if( f_rename( _T( file_path), buf) != FR_OK)
		{
			if( f_unlink ( _T( buf)) != FR_OK)
			{
				dbgSDcPRINTF("Cannot unlink file %s\n\r", buf);
				return kStatus_QMC_Err;
			}
			if( f_rename( _T( file_path), buf) != FR_OK)
			{
				dbgSDcPRINTF("Cannot rename file %s to %s\n\r", file_path, buf);
				return kStatus_QMC_Err;
			}
		}
#ifdef SDCARD_POSITIVE_DEBUG
		dbgSDcPRINTF("Renamed file %s to %s\n\r", file_path, buf);
#endif
	}
	return kStatus_QMC_Ok;
}

qmc_status_t Get_SD_FSTAT( uint32_t *total_sect, uint32_t *free_sect, const char *file_path)
{
	FATFS *fs;
	uint32_t free_clust;
	FRESULT fresult = f_getfree( _T( file_path), &free_clust, &fs);
	if( fresult != FR_OK)
	{
		dbgSDcPRINTF("fstat file failed.\r\n");
		return kStatus_QMC_Err;
	}
	*total_sect = (fs->n_fatent - 2) * fs->csize;
	*free_sect = free_clust * fs->csize;
#ifdef SDCARD_POSITIVE_DEBUG
	dbgSDcPRINTF("SDCARD total_sect:%d rfee_sect:%d\n\r", *total_sect, *free_sect);
#endif

	return kStatus_QMC_Ok;
}

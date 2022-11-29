/*
 * Copyright 2019-2022 NXP
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

    if (f_mount(&g_fileSystem, driverNumberBuffer, 1U))
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

//void FileAccessTask1(void *pvParameters)
qmc_status_t SDCard_WriteRecord( const char *dir_path, const char *file_path, char *buf)
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

	int cnt = strlen( buf);
	error = f_write(&g_fileObject1, buf, cnt, &bytesWritten);
	if ((error) || (bytesWritten != cnt))
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

	if (f_stat( _T( file_path), &fno) != FR_OK)
	{
		dbgSDcPRINTF("fstat file failed.\r\n");
		return kStatus_QMC_Err;
	}
	dbgSDcPRINTF("fsize:%d\n\r", fno.fsize);
	if( fno.fsize >= DATALOGGER_SDCARD_MAX_FILESIZE)
	{
		memset( &dt, 0, sizeof(qmc_datetime_t));
		if( BOARD_GetTime( &ts) == kStatus_QMC_Ok)
			BOARD_ConvertTimestamp2Datetime( &ts, &dt);	//return status currently doesn't care
		sprintf( buf, "%s/%02d%02d%02d%02d.txt", dir_path, dt.year&0xff, dt.month, dt.day, dt.hour );
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
		dbgSDcPRINTF("Renamed file %s to %s\n\r", file_path, buf);
	}
	return kStatus_QMC_Ok;
}

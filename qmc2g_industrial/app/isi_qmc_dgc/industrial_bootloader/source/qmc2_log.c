/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/**
 * @file    sbl_puf.c
 * @brief   PUF API.
 */

#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include <qmc2_log.h>
#include "datalogger_tasks.h"
#include "flash_recorder.h"

volatile bool isFlashDriverInitialized = false;
volatile bool isLogKeyCtrAvailable = false;
volatile bool isDataLoggerInitialized = false;

extern recorder_t g_LogRecorder;
/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/
sss_status_t QMC2_LOG_CreateLogEntry(log_event_code_t *log)
{
	qmc_status_t status = kStatus_QMC_Err;
	log_record_t  logEntry = {0};
	assert(log != NULL);

	if(isFlashDriverInitialized && isLogKeyCtrAvailable && isDataLoggerInitialized)
	{
		if (*log == LOG_EVENT_DeviceDecommissioned || *log == LOG_EVENT_NewFWReverted
				|| *log == LOG_EVENT_NewFWCommitted || *log == LOG_EVENT_KeyRevocation
				|| *log == LOG_EVENT_CfgDataBackedUp || *log == LOG_EVENT_NoLogEntry)
		{
			logEntry.type = kLOG_SystemData;
			logEntry.data.systemData.source = LOG_SRC_SecureBootloader;
			logEntry.data.systemData.category = LOG_CAT_General;
			logEntry.data.systemData.eventCode = *log;
		}
		else if (*log == LOG_EVENT_StackError || *log == LOG_EVENT_UnknownFWReturnStatus)
		{
			logEntry.type = kLOG_FaultDataWithoutID;
			logEntry.data.faultDataWithoutID.source = LOG_SRC_SecureBootloader;
			logEntry.data.faultDataWithoutID.category = LOG_CAT_General;
			logEntry.data.faultDataWithoutID.eventCode = *log;
		}
		else{
			logEntry.type = kLOG_FaultDataWithoutID;
			logEntry.data.faultDataWithoutID.source = LOG_SRC_SecureBootloader;
			logEntry.data.faultDataWithoutID.category = LOG_CAT_Fault;
			logEntry.data.faultDataWithoutID.eventCode = *log;
		}

		status = FlashWriteRecord(&logEntry, &g_LogRecorder);
		if (status == kStatus_QMC_Ok)
		{
			PRINTF("\r\nLOG: Successful!\r\n");
			return kStatus_SSS_Success;
		}
	}

	PRINTF("\r\nLOG: failed!\r\n");
	return kStatus_SSS_Fail;
}

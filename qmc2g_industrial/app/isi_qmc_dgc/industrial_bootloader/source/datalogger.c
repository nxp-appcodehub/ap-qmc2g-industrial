/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
 

#include "app.h"

#include "api_logging.h"

#include "dispatcher.h"


/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Globals
 ******************************************************************************/

/*
 * Flash LogRecorder pro datalogger.
 */
recorder_t g_InfRecorder={
	0,                                      //Idr
	RECORDER_REC_INF_DATALOGGER_AREABEGIN,  //Pt=AreaBegin
	0,                                      //RotNumber
	NULL,                                   //InfRec
	RECORDER_REC_INF_DATALOGGER_AREABEGIN,      //AreaBegin
	RECORDER_REC_INF_DATALOGGER_AREALENGTH,     //AreaLength
	OCTAL_FLASH_SECTOR_SIZE,                    //PageSize
	MAKE_EVEN( sizeof( recorder_info_t)),       //RecordSize fixed length
	0											//Flags No Crypto
};

recorder_t g_LogRecorder={
	0,                                      //Idr
	RECORDER_REC_DATALOGGER_AREABEGIN,      //Pt=AreaBegin
	0,                                      //RotNumber
	&g_InfRecorder,                           //InfRec
	RECORDER_REC_DATALOGGER_AREABEGIN,      //AreaBegin
	RECORDER_REC_DATALOGGER_AREALENGTH,     //AreaLength
	OCTAL_FLASH_SECTOR_SIZE,                //PageSize
	MAKE_EVEN( sizeof( log_record_t)),      //RecordSize fixed length
	1										//Flags Crypt this log
};


AT_NONCACHEABLE_SECTION_ALIGN(lcrypto_aes_ctx_t g_export_aes_ctx, 16);
extern mbedtls_sha256_context g_flash_recorder_sha256_ctx;



/*******************************************************************************
 * Code
 ******************************************************************************/

/*
 * @brief Init routine for datalogger service.
 *
 */
sss_status_t DataloggerInit()
{
	qmc_status_t retv = kStatus_QMC_Err;

	retv = FlashRecorderInit( &g_InfRecorder);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("InfFlashRecorderInit fail. Datalogger.\r\n");
		dbgRecPRINTF("Format inf recorder.\r\n");
		retv = FlashRecorderFormat( &g_InfRecorder);
		if( retv != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("InfFlashRecorderFormat fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}

	retv = FlashRecorderInit( &g_LogRecorder);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("FlashRecorderInit fail. Datalogger.\r\n");
		dbgRecPRINTF("Format Datalogger.\r\n");

		retv = FlashRecorderFormat( &g_LogRecorder);

		if( retv != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("FlashRecorderFormat fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}

	return kStatus_SSS_Success;

datalogger_init_failed:
	return kStatus_SSS_Fail;
}




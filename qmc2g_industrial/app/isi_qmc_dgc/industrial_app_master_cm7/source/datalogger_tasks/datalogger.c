/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#include "main_cm7.h"
#include "app.h"
#include "api_board.h"
#include "api_logging.h"
#include "api_configuration.h"
#include "dispatcher.h"
#include "sdmmc_config.h"
#include "fsl_sd_disk.h"

//#include "lcrypto.h"

/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/

#define MAKE_EVEN( size) ( ( (size)&1) == 1 ? (size+1) : (size) )

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
qmc_status_t CONFIG_Init( void);

#ifdef FEATURE_DATALOGGER_SDCARD
qmc_status_t SDCard_MountVolume(void);
qmc_status_t SDCard_WriteRecord( const char *dir_path, const char *file_path, char *buf);
qmc_status_t Handle_file( const char * dir_path, const char *file_path);
#endif

/*******************************************************************************
 * Globals
 ******************************************************************************/

/*
 * Flash LogRecorder pro datalogger.
 */
__attribute__((section(".data.$SRAM_OC2")))  recorder_t g_InfRecorder={
	0,                                      //Idr
	RECORDER_REC_INF_DATALOGGER_AREABEGIN,  //Pt=AreaBegin
	NULL,                                   //InfRec
#ifdef DATALOGGER_FLASH_RECORDER_USE_HASH
	NULL,
#endif
	RECORDER_REC_INF_DATALOGGER_AREABEGIN,      //AreaBegin
	RECORDER_REC_INF_DATALOGGER_AREALENGTH,     //AreaLength
	OCTAL_FLASH_SECTOR_SIZE,                    //PageSize
	MAKE_EVEN( sizeof( recorder_info_t))       //RecordSize fixed length
};

__attribute__((section(".data.$SRAM_OC2")))  recorder_t g_LogRecorder={
	0,                                      //Idr
	RECORDER_REC_DATALOGGER_AREABEGIN,      //Pt=AreaBegin
	&g_InfRecorder,                           //InfRec
#ifdef DATALOGGER_FLASH_RECORDER_USE_HASH
	NULL,
#endif
	RECORDER_REC_DATALOGGER_AREABEGIN,      //AreaBegin
	RECORDER_REC_DATALOGGER_AREALENGTH,     //AreaLength
	OCTAL_FLASH_SECTOR_SIZE,                //PageSize
	MAKE_EVEN( sizeof( log_record_t))       //RecordSize fixed length
};


static StaticQueue_t gs_DataloggerQueue;
static uint8_t       gs_DataloggerQueueBuffer[DATALOGGER_RCV_QUEUE_DEPTH * sizeof( log_record_t)];
static QueueHandle_t gs_DataloggerQueueHandler = NULL;
static log_record_t  gs_datalogger_rcv_record;

static bool gs_DataloggerDqInitialized = false;
static bool gs_DataloggerDqAlloc = false;
static log_static_queue_t gs_DataloggerDynamicQueue[DATALOGGER_RCV_QUEUE_CN];
static StaticSemaphore_t gs_DataloggerDqMutex;
SemaphoreHandle_t g_Datalogger_Dq_xSemaphore = NULL;

#ifdef FEATURE_DATALOGGER_DQUEUE_EVENT_BITS
EventGroupHandle_t  g_DataloggerDqEventGroupHandle;
const EventBits_t   g_DataloggerDqEventBit = DATALOGGER_EVENTBIT_DQUEUE_QUEUE;
static StaticEventGroup_t gs_DataloggerDqEventGroup;
#endif

#ifdef FEATURE_DATALOGGER_SDCARD
extern EventGroupHandle_t g_systemStatusEventGroupHandle;
#endif

static bool gs_sdcard_inserted;
static char gs_ExportBuf[256];
/*******************************************************************************
 * Code
 ******************************************************************************/

static void ConvertLogRecordToString( log_record_t *lr, char *buf)	//function will be removed when crypto comes on.
{
	if( buf == NULL)
		return;

	if( lr == NULL)
	{
		sprintf( buf, "NULL\n\r");
		return;
	}

	qmc_timestamp_t ts = lr->rhead.ts;
	qmc_datetime_t dt;
	memset( &dt, 0, sizeof(qmc_datetime_t) );
	BOARD_ConvertTimestamp2Datetime( &ts, &dt);	//return status currently doesn't care

	switch( lr->type)
	{
		case kLOG_DefaultData:
		{
			sprintf( buf, "DATE:%d/%d/%d %d:%d:%d.%d UUID:%ld RecType:%ld Source:%d Category:%d EventCode:%d User:%d\n\r",
					dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.millisecond,
					lr->rhead.uuid,
					lr->type,
					lr->data.defaultData.source,
					lr->data.defaultData.category,
					lr->data.defaultData.eventCode,
					lr->data.defaultData.user
			);
			break;
		}
		case kLOG_FaultDataWithID:
		{
			sprintf( buf, "DATE:%d/%d/%d %d:%d:%d.%d UUID:%ld RecType:%ld Source:%d Category:%d EventCode:%d MotorId:%d\n\r",
					dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.millisecond,
					lr->rhead.uuid,
					lr->type,
					lr->data.faultDataWithID.source,
					lr->data.faultDataWithID.category,
					lr->data.faultDataWithID.eventCode,
					lr->data.faultDataWithID.motorId
			);
			break;
		}
		case kLOG_FaultDataWithoutID:
		{
			sprintf( buf, "DATE:%d/%d/%d %d:%d:%d.%d UUID:%ld RecType:%ld Source:%d Category:%d EventCode:%d\n\r",
					dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.millisecond,
					lr->rhead.uuid,
					lr->type,
					lr->data.faultDataWithoutID.source,
					lr->data.faultDataWithoutID.category,
					lr->data.faultDataWithoutID.eventCode
			);
			break;
		}
		default:
		{
			sprintf( buf, "DATE:%d/%d/%d %d:%d:%d.%d UUID:%ld RecType:%ld Unsupported\n\r",
					dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, dt.millisecond,
					lr->rhead.uuid,
					lr->type
			);
			break;
		}
	}
}

static void DataloggerTask(void *pvParameters)
{
	const TickType_t xDelayms = pdMS_TO_TICKS(100);
	BaseType_t ret = pdFALSE;
	int i;

	while (1)
	{
		if( xQueueReceive( gs_DataloggerQueueHandler, &gs_datalogger_rcv_record, xDelayms ) == pdTRUE)
		{

			if( FlashWriteRecord( &gs_datalogger_rcv_record, &g_LogRecorder) != kStatus_QMC_Ok)
			{
				xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LOG_FlashError);
				dbgRecPRINTF("Cannot write record. Datalogger.\r\n");
			}

			if( gs_sdcard_inserted || gs_DataloggerDqAlloc)
			{
				ConvertLogRecordToString( &gs_datalogger_rcv_record, gs_ExportBuf);	//Encrypt log_record here.
			}

#ifdef FEATURE_DATALOGGER_SDCARD
			if( gs_sdcard_inserted)
			{
				SDCard_WriteRecord( DATALOGGER_SDCARD_DIRPATH, DATALOGGER_SDCARD_FILEPATH, gs_ExportBuf);
				Handle_file( DATALOGGER_SDCARD_DIRPATH, DATALOGGER_SDCARD_FILEPATH);
			}
#endif

#ifdef FEATURE_DATALOGGER_DQUEUE
			xSemaphoreTake(g_Datalogger_Dq_xSemaphore, portMAX_DELAY);
			if( gs_DataloggerDqAlloc)
			{
				for( i=0; i<DATALOGGER_RCV_QUEUE_CN; i++)
				{
					if(NULL != gs_DataloggerDynamicQueue[i].MsgQueueHandle.queueHandle)
					{
						ret = xQueueSend( gs_DataloggerDynamicQueue[i].QueueHandle, gs_ExportBuf, 0);
						dbgRecPRINTF("DQueue: Send frame to DQueue %d:%s\n\r", i, gs_ExportBuf);

#ifdef FEATURE_DATALOGGER_DQUEUE_EVENT_BITS
						xEventGroupSetBits( g_DataloggerDqEventGroupHandle, g_DataloggerDqEventBit);
#endif
						if( ret != pdTRUE)
						{
							dbgRecPRINTF("DQueue: Cannot send frame to DQueue %d\n\r", i);
						}
					}
				}
			}
			xSemaphoreGive(g_Datalogger_Dq_xSemaphore);
#endif
		}

#ifdef FEATURE_DATALOGGER_SDCARD
		bool sdcard_status = BOARD_SDCardGetDetectStatus();
		if( sdcard_status != gs_sdcard_inserted)
		{
			gs_sdcard_inserted = sdcard_status;
			dbgSDcPRINTF("SDCARD CD:%d\n\r", gs_sdcard_inserted);

			if( gs_sdcard_inserted)
			{
				dbgSDcPRINTF("Card inserted.\r\n");

				SDCard_MountVolume();
				xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_MEMORY_SdCardAvailable);
			}
			else
			{
				xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_MEMORY_SdCardAvailable);
				dbgSDcPRINTF("Card removed.\r\n");
			}
		}
#endif
	}
}

void DataloggerInit()
{
	int i;
    //CRYPTO_InitHardware();
    if( CONFIG_Init() != kStatus_QMC_Ok)
    {
    	dbgCnfPRINTF("Init failed!. Configuration.\r\n");
    	//Config init issue! We need to report it to FaultHandling and LocalServices service. System should not run without valid config.
    	//Another option, when octal flash is out of order, is to let the system run with defaults each time it starts. But this is no case now.
    	xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LOG_FlashError);
    	goto datalogger_init_failed;
    }

    //There is no need to init dispatcher again (already done in CONFIG_Init()), but it can be here close to FlashRecorderInit.
	if( dispatcher_init() != kStatus_QMC_Ok)
	{
		dbgDispPRINTF("Dispatcher_init fail. Datalogger.\r\n");
		goto datalogger_init_failed;
	}

	if( FlashRecorderInit( &g_InfRecorder) != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("InfFlashRecorderInit fail. Datalogger.\r\n");
		dbgRecPRINTF("Format inf recorder.\r\n");
		if( FlashRecorderFormat( &g_InfRecorder) != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("InfFlashRecorderFormat fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}
	if( FlashRecorderInit( &g_LogRecorder) != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("FlashRecorderInit fail. Datalogger.\r\n");
		dbgRecPRINTF("Format Datalogger.\r\n");
		if( FlashRecorderFormat( &g_LogRecorder) != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("FlashRecorderFormat fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}

#ifdef FEATURE_DATALOGGER_SDCARD
    BOARD_SD_Config(&g_sd, NULL, BOARD_SDMMC_SD_HOST_IRQ_PRIORITY, NULL);

    /* SD host init function */
    if (SD_HostInit(&g_sd) != kStatus_Success)
    {
    	dbgSDcPRINTF("Cannot init SD_Host interface. Datalogger.\n\r");
    }

	gs_sdcard_inserted = BOARD_SDCardGetDetectStatus();

	if( gs_sdcard_inserted)
	{
		SDCard_MountVolume();
		xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_MEMORY_SdCardAvailable);
	}
	else
	{
		xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_MEMORY_SdCardAvailable);
	}
#endif

	gs_DataloggerQueueHandler = xQueueCreateStatic( DATALOGGER_RCV_QUEUE_DEPTH, sizeof( log_record_t), gs_DataloggerQueueBuffer, &gs_DataloggerQueue);
	if( gs_DataloggerQueueHandler == NULL)
	{
		dbgRecPRINTF("xQueueCreate fail. Datalogger.\r\n");
		goto datalogger_init_failed;
	}

	if( g_Datalogger_Dq_xSemaphore == NULL)
	{
		g_Datalogger_Dq_xSemaphore = xSemaphoreCreateMutexStatic( &gs_DataloggerDqMutex);
		if( g_Datalogger_Dq_xSemaphore == NULL)
		{
			dbgRecPRINTF("DataloggerTask Datalogger_Dq_xSemaphore fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}
	for( i=0; i<DATALOGGER_RCV_QUEUE_CN; i++)
	{
		gs_DataloggerDynamicQueue[i].QueueHandle = xQueueCreateStatic( DATALOGGER_DYNAMIC_RCV_QUEUE_DEPTH, sizeof( log_encrypted_record_t), gs_DataloggerDynamicQueue[i].QueueBuffer, &gs_DataloggerDynamicQueue[i].Queue);
		if(	gs_DataloggerDynamicQueue[i].QueueHandle == NULL)
		{
			dbgRecPRINTF("xQueueCreate dynamic %d fail. Datalogger.\r\n", i);
			goto datalogger_init_failed;
		}
		gs_DataloggerDynamicQueue[i].MsgQueueHandle.queueHandle = NULL;
		gs_DataloggerDynamicQueue[i].MsgQueueHandle.eventHandle = NULL;
		gs_DataloggerDynamicQueue[i].MsgQueueHandle.eventMask	= (DATALOGGER_EVENTBIT_FIRST_STATUS_QUEUE << i);
		vQueueAddToRegistry(gs_DataloggerDynamicQueue[i].QueueHandle, "datalogger dqueue");
	}

	gs_DataloggerDqAlloc = false;

#ifdef FEATURE_DATALOGGER_DQUEUE_EVENT_BITS
	/* initialize event group for queue events */
	g_DataloggerDqEventGroupHandle = xEventGroupCreateStatic(&gs_DataloggerDqEventGroup);
	if( NULL == g_DataloggerDqEventGroupHandle )
		goto datalogger_init_failed;
#endif

	gs_DataloggerDqInitialized = true;

	if (pdPASS != xTaskCreate(DataloggerTask, "DataloggerTask", (4*configMINIMAL_STACK_SIZE), NULL, (tskIDLE_PRIORITY+1), NULL))
	{
		dbgRecPRINTF("DataloggerTask create fail. Datalogger.\r\n");
		goto datalogger_init_failed;
	}

	dbgRecPRINTF("DataloggerTask create OK. Datalogger.\r\n");

datalogger_init_failed:
	return;
}

qmc_status_t LOG_QueueLogEntry(const log_record_t* entry, bool hasPriority)
{
    BaseType_t ret = pdFALSE;

	if( gs_DataloggerDqInitialized == false)
		return kStatus_QMC_Err;

    if( entry == NULL)
    	return kStatus_QMC_ErrArgInvalid;

    if( gs_DataloggerQueueHandler == NULL)
    	return kStatus_QMC_Err;

    if( hasPriority)
    {
    	ret = xQueueSendToFront( gs_DataloggerQueueHandler, entry, 0);
    }
    else
    {
    	ret = xQueueSend( gs_DataloggerQueueHandler, entry, 0);
    }

	if( ret != pdTRUE)
		return kStatus_QMC_Err;

	return kStatus_QMC_Ok;
}

qmc_status_t LOG_GetNewLoggingQueueHandle(qmc_msg_queue_handle_t** handle)
{
	int i;

	if( gs_DataloggerDqInitialized == false)
		return kStatus_QMC_Err;

	if( handle == NULL)
		return kStatus_QMC_ErrArgInvalid;

	xSemaphoreTake(g_Datalogger_Dq_xSemaphore, portMAX_DELAY);
	for( i=0; i<DATALOGGER_RCV_QUEUE_CN; i++)
	{
		if(NULL == gs_DataloggerDynamicQueue[i].MsgQueueHandle.queueHandle)
		{
			gs_DataloggerDynamicQueue[i].MsgQueueHandle.queueHandle = &(gs_DataloggerDynamicQueue[i].QueueHandle);
			*handle = &(gs_DataloggerDynamicQueue[i].MsgQueueHandle);
			gs_DataloggerDqAlloc = true;
			xSemaphoreGive(g_Datalogger_Dq_xSemaphore);
			return kStatus_QMC_Ok;
		}
	}
	xSemaphoreGive(g_Datalogger_Dq_xSemaphore);
	dbgRecPRINTF("xQueueCreate dynamic fail. Datalogger.\r\n");
	return kStatus_QMC_Err;
}

qmc_status_t LOG_ReturnLoggingQueueHandle(const qmc_msg_queue_handle_t* handle)
{
	int i;
	qmc_status_t retval = kStatus_QMC_Err;

	if( gs_DataloggerDqInitialized == false)
		return kStatus_QMC_Err;

	if( handle == NULL)
		return kStatus_QMC_ErrArgInvalid;

	xSemaphoreTake(g_Datalogger_Dq_xSemaphore, portMAX_DELAY);

	xQueueReset( *handle->queueHandle);

	gs_DataloggerDqAlloc = false;
	for( i=0; i<DATALOGGER_RCV_QUEUE_CN; i++)
	{
		/* search handle and mark it as available again */
		if(handle == &(gs_DataloggerDynamicQueue[i].MsgQueueHandle))
		{
			gs_DataloggerDynamicQueue[i].MsgQueueHandle.queueHandle = NULL;
			retval = kStatus_QMC_Ok;
		}
		if( gs_DataloggerDynamicQueue[i].MsgQueueHandle.queueHandle != NULL)
			gs_DataloggerDqAlloc = true;
	}
	xSemaphoreGive(g_Datalogger_Dq_xSemaphore);
	return retval;
}

qmc_status_t LOG_DequeueEncryptedLogEntry(const qmc_msg_queue_handle_t* handle, uint32_t timeout, log_encrypted_record_t* entry)
{
	if( gs_DataloggerDqInitialized == false)
		return kStatus_QMC_Err;

	if( handle == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( handle->queueHandle == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( xQueueReceive( *handle->queueHandle, entry->data, timeout ) == pdTRUE)
	{
		entry->length = sizeof( log_encrypted_record_t);
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_Timeout;
}

qmc_status_t LOG_GetLogRecord(uint32_t id, log_record_t* record)
{
	if( FlashGetRecord( id, &g_LogRecorder, record, portMAX_DELAY) == NULL)
	{
		return kStatus_QMC_Err;
	}
	return kStatus_QMC_Ok;
}

qmc_status_t LOG_GetLogRecordEncrypted(uint32_t id, log_encrypted_record_t* record)
{
	log_record_t lrec;
	if( FlashGetRecord( id, &g_LogRecorder, &lrec, portMAX_DELAY) == NULL)
	{
		return kStatus_QMC_Err;
	}

	//Encrypt record here. As crypto is not implemented yet just copy log_record.
	memcpy( record->data, &lrec, sizeof( log_record_t));
	record->length = sizeof( log_record_t);

	return kStatus_QMC_Ok;
}

uint32_t LOG_GetLastLogId()
{
	return FlashGetLastIdr( &g_LogRecorder);
}

qmc_status_t LOG_InitDatalogger()
{
	qmc_status_t retv=FlashRecorderInit( &g_LogRecorder);
	return retv;
}

qmc_status_t LOG_FormatDatalogger()
{
	qmc_status_t retv=FlashRecorderFormat( &g_LogRecorder);
	return retv;
}

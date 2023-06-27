/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 *
 */
 
#include "main_cm7.h"
#include "app.h"
#include "api_board.h"
#include "api_logging.h"
#include "api_configuration.h"
#include "api_motorcontrol.h"
#include "api_rpc.h"
#include "dispatcher.h"
#include "sdmmc_config.h"
#include "fsl_sd_disk.h"

/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/
//#define DATALOGGER_POSITIVE_DEBUG

typedef struct __attribute__((__packed__)) {
	uint8_t key[LCRYPTO_EX_AES_KEY_SIZE];
	uint8_t iv[LCRYPTO_EX_AES_IV_SIZE];
} lcrypt_keyiv_t;

#define MOTOR_QUEUE_TIMEOUT_ATTEMPTS        20u

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
qmc_status_t CONFIG_Init( void);
qmc_status_t Datalogger_encrypt_log_entry( log_record_t *psrc, log_encrypted_record_t *pdst, TickType_t ticks);

#ifdef FEATURE_DATALOGGER_SDCARD
qmc_status_t SDCard_MountVolume(void);
qmc_status_t SDCard_WriteRecord( const char *dir_path, const char *file_path, uint8_t *buf, size_t buf_len);
qmc_status_t Handle_file( const char * dir_path, const char *file_path);
#endif

static void StopAllMotors(void);
static void DisableMotorInterrupts(void);

/*******************************************************************************
 * Globals
 ******************************************************************************/

/*
 * Flash LogRecorder pro datalogger.
 */
__attribute__((section(".data.$SRAM_OC2")))  recorder_t g_InfRecorder={
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

__attribute__((section(".data.$SRAM_OC2")))  recorder_t g_LogRecorder={
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

static StaticSemaphore_t gs_DataloggerCtrlMutex;
SemaphoreHandle_t g_Datalogger_Ctrl_xSemaphore = NULL;

#ifdef FEATURE_DATALOGGER_SDCARD
extern EventGroupHandle_t g_systemStatusEventGroupHandle;
#endif

static bool gs_sdcard_inserted;
static log_encrypted_record_t gs_enc_export_data;


AT_NONCACHEABLE_SECTION_ALIGN(lcrypto_aes_ctx_t g_export_aes_ctx, 16);
extern mbedtls_sha256_context g_flash_recorder_sha256_ctx;

TaskHandle_t g_datalogger_task_handle;


/*******************************************************************************
 * Code
 ******************************************************************************/


/*
 * @brief RTOS task responsible for receiving, encrypting, signing and storing into the NOR flash, SD Card or Cloud service.
 *
 */

static void DataloggerTask(void *pvParameters)
{
	const TickType_t xDelayms = pdMS_TO_TICKS(100);
	BaseType_t ret = pdFALSE;
	int i;
	uint32_t wakeupEvent = 0;
	bool queueEmpty = true;

	while (1)
	{
		xTaskNotifyWait(0, 0, &wakeupEvent, xDelayms);

		if ((wakeupEvent & kDLG_SHUTDOWN_PowerLoss) || (wakeupEvent & kDLG_SHUTDOWN_WatchdogReset))
		{
			vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
	        StopAllMotors();
	        MC_SetTsnCommandInjection(false);
	        vTaskDelay(pdMS_TO_TICKS(10));
	        DisableMotorInterrupts();
		}

		wakeupEvent = wakeupEvent & (~(kDLG_LOG_Queued));

		do
		{
			if ( xQueueReceive( gs_DataloggerQueueHandler, &gs_datalogger_rcv_record, 0 ) == pdTRUE)
			{
				queueEmpty = false;

				xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
				qmc_status_t retv = FlashWriteRecord( &gs_datalogger_rcv_record, &g_LogRecorder);
				xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
				if( retv != kStatus_QMC_Ok)
				{
					xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LOG_FlashError);
					dbgRecPRINTF("Cannot write record. Datalogger. %d\r\n", gs_datalogger_rcv_record.type);
				}
				else
				{
	#ifdef DATALOGGER_POSITIVE_DEBUG
					dbgRecPRINTF("Write record. Datalogger. %d\r\n", gs_datalogger_rcv_record.type);
	#endif
				}

				if (!(wakeupEvent & kDLG_SHUTDOWN_PowerLoss))
				{
#if defined(FEATURE_DATALOGGER_SDCARD) || defined(FEATURE_DATALOGGER_DQUEUE)
					if( gs_sdcard_inserted || gs_DataloggerDqAlloc)
					{
						retv = Datalogger_encrypt_log_entry( &gs_datalogger_rcv_record, &gs_enc_export_data, portMAX_DELAY);
						if( retv != kStatus_QMC_Ok)
						{
							xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LOG_FlashError);
							dbgRecPRINTF("Cannot encrypt log_entry for export. Datalogger.\r\n");
						}
					}
					else
					{
						//Record is not going to be recorded on SD card nor Cloud service -> set the QMC_SYSEVENT_LOG_MessageLost
						xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LOG_MessageLost);
						dbgRecPRINTF("Message is not sent to SD card nor to Cloud service. Datalogger.\r\n");
					}
#endif

#ifdef FEATURE_DATALOGGER_SDCARD
					if( gs_sdcard_inserted)
					{
						retv = SDCard_WriteRecord( DATALOGGER_SDCARD_DIRPATH, DATALOGGER_SDCARD_FILEPATH, (uint8_t *)&gs_enc_export_data, sizeof( gs_enc_export_data));
						if( retv != kStatus_QMC_Ok)
							dbgRecPRINTF("Cannot export log_entry to SDCard. Datalogger.\r\n");
						else
						{
#ifdef DATALOGGER_POSITIVE_DEBUG
							dbgRecPRINTF("Export log_entry to SDCard. Datalogger.\r\n");
#endif
						}

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
								ret = xQueueSend( gs_DataloggerDynamicQueue[i].QueueHandle, &gs_enc_export_data, 0);
#ifdef DATALOGGER_POSITIVE_DEBUG
								dbgRecPRINTF("DQueue: Send frame to DQueue %d:%s\n\r", i, sizeof(gs_enc_export_data));
#endif

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
			}
			else
			{
				queueEmpty = true;
			}
		} while (!queueEmpty);

	    if (wakeupEvent & kDLG_SHUTDOWN_PowerLoss)
	    {
	    	/* Reset the device to prevent it from continuing any further */
	    	while (RPC_Reset(kQMC_ResetRequest) != kStatus_QMC_Ok){}
	    }
	    else if (wakeupEvent & kDLG_SHUTDOWN_WatchdogReset)
	    {
	    	/* Reset the device to prevent it from continuing any further */
	    	while (RPC_Reset(kQMC_ResetSecureWd) != kStatus_QMC_Ok){}
	    }
	    else
	    {
	    	;
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

/*
 * @brief Init routine for datalogger service.
 *
 */
qmc_status_t DataloggerInit()
{
	int i;
	qmc_status_t retv = kStatus_QMC_Err;

	LCRYPTO_init();

	if( dispatcher_init() != kStatus_QMC_Ok)
	{
		dbgDispPRINTF("Dispatcher_init fail. Datalogger.\r\n");
		goto datalogger_init_failed;
	}

    if( CONFIG_Init() != kStatus_QMC_Ok)
    {
    	dbgCnfPRINTF("Init failed!. Configuration.\r\n");
    	xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LOG_FlashError);
    	goto datalogger_init_failed;
    }

	if( g_Datalogger_Ctrl_xSemaphore == NULL)
	{
		g_Datalogger_Ctrl_xSemaphore = xSemaphoreCreateMutexStatic( &gs_DataloggerCtrlMutex);
		if( g_Datalogger_Ctrl_xSemaphore == NULL)
		{
			dbgRecPRINTF("DataloggerTask Datalogger_Ctrl_xSemaphore fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}

	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	retv = FlashRecorderInit( &g_InfRecorder);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("InfFlashRecorderInit fail. Datalogger.\r\n");
		dbgRecPRINTF("Format inf recorder.\r\n");
		xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
		retv = FlashRecorderFormat( &g_InfRecorder);
		xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
		if( retv != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("InfFlashRecorderFormat fail. Datalogger.\r\n");
			goto datalogger_init_failed;
		}
	}
	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	retv = FlashRecorderInit( &g_LogRecorder);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("FlashRecorderInit fail. Datalogger.\r\n");
		dbgRecPRINTF("Format Datalogger.\r\n");
		xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
		retv = FlashRecorderFormat( &g_LogRecorder);
		xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
		if( retv != kStatus_QMC_Ok)
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

	if (pdPASS != xTaskCreate(DataloggerTask, "DataloggerTask", (20*configMINIMAL_STACK_SIZE), NULL, (tskIDLE_PRIORITY+1), &g_datalogger_task_handle))
	{
		dbgRecPRINTF("DataloggerTask create fail. Datalogger.\r\n");
		goto datalogger_init_failed;
	}


#ifdef DATALOGGER_POSITIVE_DEBUG
	dbgRecPRINTF("DataloggerTask create OK. Datalogger.\r\n");
#endif

	return kStatus_QMC_Ok;

datalogger_init_failed:
	return kStatus_QMC_Err;
}

/*!
 * @brief Put one LogRecord into the message queue and notify the Logging Service task. If the hasPriority flag is set the log record is put at the beginning of the queue instead of its end.
 *
 * @param[in] entry Pointer to the entry to be queued
 * @param[in] hasPriority If true, the entry is queue with priority.
 */
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

	xTaskNotify(g_datalogger_task_handle, kDLG_LOG_Queued, eSetBits);

	return kStatus_QMC_Ok;
}

/*!
 * @brief Request a new message queue handle to receive logging messages. Operation may return kStatus_QMC_ErrMem if queue cannot be created or all statically created queues are in use.
 *
 * @param[out] handle Address of the pointer to write the retrieved handle to
 */
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

/*!
 * @brief Hand back a message queue handle obtained by LOG_GetNewLoggingQueueHandle(handle : qmc_msg_queue_handle_t*) : qmc_status_t.
 *
 * @param[in] handle Pointer to the handle that is no longer in use
 */
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

/*!
 * @brief Get one log_encrypted_record_t element from the previously registered queue, if available.
 *
 * @param[in]  handle Handle of the queue to receive the log entry from
 * @param[in]  timeout Timeout in milliseconds
 * @param[out] entry Pointer to write the retrieved log entry to
 * @return kStatus_QMC_Ok = A log entry was successfully retrieved; kStatus_QMC_ErrArgInvalid = Invalid input parameters, e.g. NULL pointer; kStatus_QMC_Timeout = No log entry was available until the timeout expired
 */
qmc_status_t LOG_DequeueEncryptedLogEntry(const qmc_msg_queue_handle_t* handle, uint32_t timeout, log_encrypted_record_t* entry)
{
	if( gs_DataloggerDqInitialized == false)
		return kStatus_QMC_Err;

	if( handle == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( handle->queueHandle == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( xQueueReceive( *handle->queueHandle, entry, timeout ) == pdTRUE)
		return kStatus_QMC_Ok;

	return kStatus_QMC_Timeout;
}

/*!
 * @brief Get the log_record_t with the given ID.
 *
 * @param[in]  id ID of the record to be retrieved
 * @param[out] record Pointer to write the retrieved log record to
 * @return kStatus_QMC_Ok = The log entry was successfully retrieved and stored at the given location; kStatus_QMC_ErrArgInvalid = An invalid log ID or a NULL pointer was passed
 */
qmc_status_t LOG_GetLogRecord(uint32_t id, log_record_t* record)
{
	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	void *pt = FlashGetRecord( id, &g_LogRecorder, record, portMAX_DELAY);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	if( pt == NULL)
	{
		return kStatus_QMC_Err;
	}
	return kStatus_QMC_Ok;
}

/*!
 * @brief Get the log record with the given ID and encrypt it for the external log reader.
 *
 * @param[in]  id ID of the record to be retrieved
 * @param[out] record Pointer to write the encrypted log record to
 * @return kStatus_QMC_Ok = The log entry was successfully retrieved and stored at the given location; kStatus_QMC_ErrArgInvalid = An invalid log ID or a NULL pointer was passed
 */
qmc_status_t LOG_GetLogRecordEncrypted(uint32_t id, log_encrypted_record_t* record)
{
	log_record_t lrec;
	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	void * pt = FlashGetRecord( id, &g_LogRecorder, &lrec, portMAX_DELAY);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	if( pt == NULL)
	{
		return kStatus_QMC_Err;
	}

	if( Datalogger_encrypt_log_entry( &lrec, record, portMAX_DELAY) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	return kStatus_QMC_Ok;
}

/*!
 * @brief Returns the ID of the latest log entry.
 */
uint32_t LOG_GetLastLogId()
{
	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	qmc_status_t retv = FlashGetLastIdr( &g_LogRecorder);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	return retv;
}

/*!
 * @brief Initialization of Flash Recorder. Need to be executed before the first usage.
 */
qmc_status_t LOG_InitDatalogger()
{
	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	qmc_status_t retv = FlashRecorderInit( &g_LogRecorder);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	return retv;
}

/*!
 * @brief Format (erase) space of Flash Recorder.
 */
qmc_status_t LOG_FormatDatalogger()
{
	xSemaphoreTake(g_Datalogger_Ctrl_xSemaphore, portMAX_DELAY);
	qmc_status_t retv = FlashRecorderFormat( &g_LogRecorder);
	xSemaphoreGive(g_Datalogger_Ctrl_xSemaphore);
	return retv;
}

/*!
 * @brief Encrypt log_record_t using symmetric (CAAM) and asymmetric (SE05x) cryptography.
 */
qmc_status_t Datalogger_encrypt_log_entry( log_record_t *psrc, log_encrypted_record_t *pdst, TickType_t ticks)
{
	qmc_status_t retv = kStatus_QMC_Err;
	lcrypt_keyiv_t keyiv;

	//Get rnd number and use it as SYMKEY and IV
	retv = LCRYPTO_SE_get_RND( (uint8_t *)&keyiv, sizeof( keyiv));
	if( retv != kStatus_QMC_Ok)
		return retv;

	memcpy( g_export_aes_ctx.iv, keyiv.iv, sizeof( keyiv.iv));
	memcpy( g_export_aes_ctx.key, keyiv.key, sizeof( keyiv.key));

	const size_t rsize16 = MAKE_NUMBER_ALIGN( sizeof(log_record_t), 16);
	uint8_t *rbuff = pvPortMalloc( 2*rsize16 + 16);
	if( rbuff == NULL)
	{
		dbgRecPRINTF("LCRYPTO Cannot alloc mem16.\n\r");
		return kStatus_QMC_ErrMem;
	}
	uint8_t *dst16 = (uint8_t *)MAKE_NUMBER_ALIGN( (uint32_t)rbuff, 16);
	uint8_t *src16 = dst16 + rsize16;
	memcpy( src16, (uint8_t *)psrc, sizeof(log_record_t));

	//Encrypt log_record data using keyiv.iv (IV) and keyiv.key (SYMKEY)
	//Input (src) and output (dst) buffers must be alligned(16)!!!
	retv = LCRYPTO_encrypt_aes256_cbc( dst16, src16, sizeof(log_record_t), &g_export_aes_ctx, ticks);
	if( retv != kStatus_QMC_Ok)
	{
		vPortFree( rbuff);
		dbgRecPRINTF("LCRYPTO Encrypt AES CBC#2 Err:%d\r\n", retv);
		return retv;
	}
	memcpy( pdst->data.lr_enc, dst16, sizeof(pdst->data.lr_enc));
	vPortFree( rbuff);

	//Encrypt IV + SYMKEY using RSA (SE05x)
	size_t dst_len = sizeof( pdst->data.keyiv_enc);
	retv = LCRYPTO_SE_crypt_RSA( (uint8_t*)pdst->data.keyiv_enc, &dst_len, (uint8_t *)&keyiv, sizeof( keyiv));
	if( retv == kStatus_QMC_Ok)
	{
#ifdef DATALOGGER_POSITIVE_DEBUG
		dbgRecPRINTF("RSA Encryption Successful. %d/%d\n\r", dst_len, sizeof( keyiv));
#endif

	}
	else
	{
		dbgRecPRINTF("RSA Encryption Error !!!\n\r");
		return kStatus_QMC_Err;
	}

	//Compute Hash (SHA2-384) of encrypted data
	uint8_t hash[LCRYPTO_EX_HASH384_SIZE];
	dst_len = sizeof( hash);
	retv = LCRYPTO_SE_get_sha384( hash, &dst_len, (uint8_t *)&pdst->data, sizeof( pdst->data));
	if( retv == kStatus_QMC_Ok)
	{
#ifdef DATALOGGER_POSITIVE_DEBUG
		dbgRecPRINTF("SHA2-384 Successful. %d/%d\n\r", dst_len, sizeof( pdst->data));
#endif
	}
	else
	{
		dbgRecPRINTF("SHA2-384 Error %d!!!\n\r", retv);
		return kStatus_QMC_Err;
	}

	//Sign ECDSA_SHA2-384 of hash
	dst_len = sizeof( pdst->data_signature);
	retv = LCRYPTO_SE_sign_ECC( pdst->data_signature, &dst_len, hash, sizeof( hash));
	if( retv == kStatus_QMC_Ok)
	{
#ifdef DATALOGGER_POSITIVE_DEBUG
		dbgRecPRINTF("Data sign Successful. %d/%d/%d\n\r", dst_len, sizeof( pdst->data_signature), sizeof( hash));
#endif
	}
	else
	{
		dbgRecPRINTF("Data sign Error %d!!!\n\r", retv);
		return kStatus_QMC_Err;
	}

	pdst->length = sizeof( log_encrypted_record_t);

	return kStatus_QMC_Ok;
}

/*!
 * @brief Stops all motors.
 */
static void StopAllMotors(void)
{
	for (mc_motor_id_t stopMotorId = kMC_Motor1; stopMotorId < MC_MAX_MOTORS; stopMotorId++)
	{
		mc_motor_command_t cmd = {0};
		cmd.eMotorId = stopMotorId;
		cmd.eAppSwitch = kMC_App_FreezeAndStop;

		int motorQueueTimeout = MOTOR_QUEUE_TIMEOUT_ATTEMPTS;
		do
		{
			qmc_status_t status = MC_QueueMotorCommand(&cmd);
			if ((status == kStatus_QMC_Ok) || (status == kStatus_QMC_ErrBusy))
			{
				break;
			}

			motorQueueTimeout--;
			vTaskDelay(10);
		} while (motorQueueTimeout > 0);

		if (motorQueueTimeout <= 0)
		{
			log_record_t logEntryError = {0};
			logEntryError.type = kLOG_FaultDataWithoutID;
			logEntryError.data.faultDataWithoutID.source = LOG_SRC_FaultHandling;
			logEntryError.data.faultDataWithoutID.category = LOG_CAT_Fault;
			logEntryError.data.faultDataWithoutID.eventCode = LOG_EVENT_QueueingCommandFailedQueue;
			LOG_QueueLogEntry(&logEntryError, true);

			BOARD_SetLifecycle(kQMC_LcError);
			return;
		}
	}
}

/*!
 * @brief Disable motor interrupts.
 */
static void DisableMotorInterrupts(void)
{
	NVIC_DisableIRQ(M1_fastloop_irq);
	NVIC_DisableIRQ(M1_slowloop_irq);
	NVIC_DisableIRQ(M2_fastloop_irq);
	NVIC_DisableIRQ(M2_slowloop_irq);
	NVIC_DisableIRQ(M3_fastloop_irq);
	NVIC_DisableIRQ(M3_slowloop_irq);
	NVIC_DisableIRQ(M4_fastloop_irq);
	NVIC_DisableIRQ(M4_slowloop_irq);
}

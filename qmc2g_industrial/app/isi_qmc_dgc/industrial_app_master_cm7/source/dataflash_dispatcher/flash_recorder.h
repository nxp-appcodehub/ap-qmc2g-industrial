/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef __FLASH_RECORDER_H__
#define __FLASH_RECORDER_H__


//#include <sha256.h>

//Recorder start address
#define FLASH_RECORDER_HASH_SIZE                 (32U)

typedef struct __attribute__((__packed__)) _record_head
{
	union {
		uint8_t          hash[ FLASH_RECORDER_HASH_SIZE]; /*!< Hash value of the entire log record. */
		uint32_t         chksum;
	};
    uint32_t             uuid;                /*!< Unique identifier of this record */
    qmc_timestamp_t		ts;
} record_head_t;

typedef struct __attribute__(( __packed__ )) RECORDER_STATUS
{
	uint32_t Idr;
	uint32_t Pt;
	uint32_t AreaBegin;
	uint32_t AreaLength;
	uint16_t PageSize;
	uint16_t RecordSize;

	uint32_t cnt;
    qmc_timestamp_t		ts;
} recorder_status_t;

typedef struct __attribute__(( __packed__ )) RECORDER
{
	uint32_t Idr;
	uint32_t Pt;
	const void *InfRec;
#ifdef DATALOGGER_FLASH_RECORDER_USE_HASH
    mbedtls_sha256_context *flash_recorder_ctx;
#endif
	const uint32_t AreaBegin;
	const uint32_t AreaLength;
	const uint16_t PageSize; 
	const uint16_t RecordSize;
} recorder_t;

typedef struct __attribute__((__packed__)) RECORDER_INFO
{
	record_head_t        rhead;
	uint32_t             RotationNumber;	//Number of loops
	uint8_t              RecordOrigin;		//The reason why this record was created
} recorder_info_t;

//extern struct RECORDER LogRecorder;	//konfigurace a info k Rec2

qmc_status_t FlashRecorderInit( recorder_t *prec);
qmc_status_t FlashRecorderFormat( recorder_t *prec);
qmc_status_t FlashWriteRecord( void *pt, recorder_t *prec);
void *FlashGetRecord( uint32_t idr, recorder_t *prec, void* record, TickType_t ticks);
int FlashGetStatusInfo( recorder_status_t *rstat, recorder_t *prec);
uint32_t FlashGetLastIdr( recorder_t *prec);

#endif	// __FLASH_RECORDER_H__

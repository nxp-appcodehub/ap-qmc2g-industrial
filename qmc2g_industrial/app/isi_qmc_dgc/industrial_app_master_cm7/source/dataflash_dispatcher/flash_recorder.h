/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __FLASH_RECORDER_H__
#define __FLASH_RECORDER_H__

#include <lcrypto.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DATALOGGER_HASH_SIZE                 LCRYPTO_HASH_SIZE
#define DATALOGGER_AES_KEY_SIZE              LCRYPTO_AES_KEY_SIZE
#define DATALOGGER_AES_IV_SIZE               LCRYPTO_AES_IV_SIZE

typedef struct __attribute__((__packed__)) _record_head
{
	union {
		uint8_t          hash[ DATALOGGER_HASH_SIZE]; /*!< Hash value of the entire log record. */
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
	uint32_t RotNumber;
	const void *InfRec;
	const uint32_t AreaBegin;
	const uint32_t AreaLength;
	const uint16_t PageSize; 
	const uint16_t RecordSize;
	const uint16_t Flags;
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
uint32_t FlashGetFirstIdr( recorder_t *prec);

#endif	// __FLASH_RECORDER_H__

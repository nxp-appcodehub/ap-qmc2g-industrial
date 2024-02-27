/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
 
#include "main_cm7.h"
#include "app.h"
#include "flash_recorder.h"
#include "api_board.h"
#include <api_logging.h>
#include <dispatcher.h>

//#define FLASH_RECORDER_POSITIVE_DEBUG

AT_NONCACHEABLE_SECTION_ALIGN( mbedtls_sha256_context g_flash_recorder_sha256_ctx, 32);
AT_NONCACHEABLE_SECTION_ALIGN( lcrypto_aes_ctx_t g_flash_recorder_ctx1, 32);
AT_NONCACHEABLE_SECTION_ALIGN( lcrypto_aes_ctx_t g_flash_recorder_ctx2, 32);

/*
 * Function returns the value of the Last Id in log.
 * Return value:
 * value of the Last Id in log.
 */
uint32_t FlashGetLastIdr( recorder_t *prec)
{
	return prec->Idr;
}

/*
 * Function checks the hash value stored in first DATALOGGER_HASH_SIZE bytes of input data pointed by *pt
 * and checks it against hash of remaining data started at offset DATALOGGER_HASH_SIZE.
 * Return value:
 * kStatus_QMC_Ok                   hash equals to hash of data
 * kStatus_QMC_ErrSignatureInvalid  hash does not equal hash of data
 * kStatus_QMC_ErrMem               cannot allocate memory on heap, no checks done
 * kStatus_QMC_ErrBusy              cannot get CAAM mutex
 * kStatus_QMC_Err                  cannot give back CAAM mutex
 */
static qmc_status_t HashRecordCheck( void *pt, recorder_t *prec)
{
	qmc_status_t retv;
	const TickType_t xDelayms = pdMS_TO_TICKS( DATALOGGER_MUTEX_XDELAYS_MS);

	if( prec->RecordSize < DATALOGGER_HASH_SIZE)
		return kStatus_QMC_ErrArgInvalid;
	const int dsize = prec->RecordSize - DATALOGGER_HASH_SIZE;

	uint8_t *shabuff = pvPortMalloc( 2*DATALOGGER_HASH_SIZE + dsize + 32);
	if( shabuff == NULL)
	{
		dbgRecPRINTF("dec HRC Cannot alloc mem.\n\r");
		return kStatus_QMC_ErrMem;
	}
	uint8_t *shabuff32 = (uint8_t *)MAKE_NUMBER_ALIGN( (uint32_t)shabuff, 32);

	memcpy( shabuff32 + DATALOGGER_HASH_SIZE, pt, prec->RecordSize);
	retv = LCRYPTO_get_sha256( shabuff32, shabuff32 + (2*DATALOGGER_HASH_SIZE) , dsize, &g_flash_recorder_sha256_ctx, xDelayms);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("dec HRC Err:%d\n\r", retv);
		vPortFree( shabuff);
		return retv;
	}

	SCB_InvalidateDCache_by_Addr ( shabuff32, DATALOGGER_HASH_SIZE);
	if( memcmp( shabuff32, shabuff32 + DATALOGGER_HASH_SIZE, DATALOGGER_HASH_SIZE) != 0)
	{
		vPortFree( shabuff);
		return kStatus_QMC_ErrSignatureInvalid;
	}
	vPortFree( shabuff);

	return kStatus_QMC_Ok;
}

/*
 * Function calculate hash of data started at offset DATALOGGER_HASH_SIZE
 * and updates the hash value in first DATALOGGER_HASH_SIZE bytes of input data pointed by *pt.
 * Return value:
 * kStatus_QMC_Ok                   hash equals to hash of data
 * kStatus_QMC_ErrSignatureInvalid  hash does not equal hash of data
 * kStatus_QMC_ErrMem               cannot allocate memory on heap, no checks done
 * kStatus_QMC_ErrBusy              cannot get CAAM mutex
 * kStatus_QMC_Err                  cannot give back CAAM mutex
 */
static qmc_status_t HashRecordUpdate( void *pt, recorder_t *prec)
{
	qmc_status_t retv = kStatus_QMC_Err;
	const TickType_t xDelayms = pdMS_TO_TICKS( DATALOGGER_MUTEX_XDELAYS_MS);
	if( prec->RecordSize < DATALOGGER_HASH_SIZE)
	{
		dbgRecPRINTF("dec HRU Cannot dsize.\n\r");
		return kStatus_QMC_ErrArgInvalid;
	}
	const int dsize = prec->RecordSize - DATALOGGER_HASH_SIZE;
	uint8_t *shabuff = pvPortMalloc( prec->RecordSize +32);
	if( shabuff == NULL)
	{
		dbgRecPRINTF("dec HRU Cannot alloc mem.\n\r");
		return kStatus_QMC_ErrMem;
	}
	uint8_t *shabuff32 = (uint8_t *)MAKE_NUMBER_ALIGN( (uint32_t)shabuff, 32);
	memcpy( shabuff32 + DATALOGGER_HASH_SIZE, pt + DATALOGGER_HASH_SIZE, dsize);

	retv = LCRYPTO_get_sha256( shabuff32, shabuff32 + DATALOGGER_HASH_SIZE, dsize, &g_flash_recorder_sha256_ctx, xDelayms);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("dec HRU Err:%d\n\r", retv);
		vPortFree( shabuff);
		return retv;
	}
	SCB_InvalidateDCache_by_Addr ( shabuff32, DATALOGGER_HASH_SIZE);
    memcpy( pt, shabuff32, DATALOGGER_HASH_SIZE);
    vPortFree( shabuff);

    return kStatus_QMC_Ok;
}

/*
 * Function align *pt at the begin of next sector when needed. Otherwise it is left as it is.
 * Return value:
 * 0  when pt + sizeof( FLASH_RECORD) fits in actual sector. *pt is not modified.
 * 1  when pt + sizeof( FLASH_RECORD) does not fit in actual sector. *pt is set to point at the beginning of the next sector.
 * 2  when pt + sizeof( FLASH_RECORD) does not fit in last sector of recorder. *pt is set to point at the beginning of the first sector of recorder.
 */
static int FlashAlignPt( uint32_t *pt, recorder_t *prec)
{
	uint32_t off=*pt % prec->PageSize;
	if( !off || ( off + prec->RecordSize > prec->PageSize))
	{
		//does not fit -> use next sector
		if( off)
			*pt+=prec->PageSize-off;

		if( *pt >= prec->AreaBegin + prec->AreaLength)
		{
			*pt=prec->AreaBegin;
			//Closed circle -> We need to update RotationNumber in RECORDER_INFO
			return 2;
		}
		return 1;
	}
	return 0;
}

/*
 * Function writes record into the recorder. * uuid, timestamp_s, timestamp_ms are updated.
 * if *prec->flag is 1 recod data are ecrypted by AES256-CTR.
 * if *prec->inf points to inf recorder function updates and write the inf record.
 * Return value:
 * kStatus_QMC_ErrMem               cannot allocate memory on heap, no write done
 * kStatus_QMC_ErrBusy              cannot get dispatcher mutex or CAAM mutex
 * kStatus_QMC_Err                  general error
 * kStatus_QMC_Ok                   write succesfuly done
 */
qmc_status_t FlashWriteRecord( void *pt, recorder_t *prec)
{
	qmc_status_t retv;
	int state;

	if( prec == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( pt == NULL)
		return kStatus_QMC_ErrArgInvalid;

	record_head_t *h=( record_head_t *)pt;
	h->uuid=prec->Idr;
	h->uuid++;
	if( h->uuid==0xFFFFFFFF)	//0xFFFFFFFF is reserved for "clear space"
		h->uuid=0;

    qmc_timestamp_t	ts = {0,0};
	retv = BOARD_GetTime( &ts);
	if( retv!= kStatus_QMC_Ok)
	{
		dbgRecPRINTF("FlashWriteRecord. Cannot read BOARD_GetTime(): %d:%d retv:%d\n\r", ts.seconds, ts.milliseconds, retv);
	}

	h->ts = ts;

	HashRecordUpdate( pt, prec);

	if( prec->Flags & 0x1)
	{
		const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);
		const size_t rsize16 = MAKE_NUMBER_ALIGN( prec->RecordSize, 16);

		uint8_t *rbuff = pvPortMalloc( 2*rsize16 + 16);
		if( rbuff == NULL)
		{
			dbgRecPRINTF("dec HRC Cannot alloc mem.\n\r");
			return kStatus_QMC_ErrMem;
		}
		uint8_t *rbuff16 = (uint8_t *)MAKE_NUMBER_ALIGN( (uint32_t)rbuff, 16);
		memcpy( rbuff16 + rsize16, pt, prec->RecordSize);

		uint32_t flash_pt = prec->Pt;
		state = FlashAlignPt( &flash_pt, prec);
		if( state != 0)
		{
			//Erase page
			retv=dispatcher_erase_sectors( (uint8_t *)flash_pt, 1, portMAX_DELAY);
#ifdef FLASH_RECORDER_POSITIVE_DEBUG
			dbgRecPRINTF("Erase %x\n\r", flash_pt);
#endif
			if( retv!= kStatus_QMC_Ok)
			{
				dbgRecPRINTF("FlashWriteRecord1 erase Err:%d\n\r", retv);
				vPortFree( rbuff);
				return retv;	//error
			}
			if( state == 2)
			{
				prec->RotNumber++;
			}
		}

		//Credentials IV
#ifdef NO_SBL
		memset( g_flash_recorder_ctx1.iv, 0, sizeof(g_flash_recorder_ctx1.iv));
#else
		memcpy( g_flash_recorder_ctx1.iv, (void *)g_sbl_prov_keys.nonceLog, sizeof(g_flash_recorder_ctx1.iv));
#endif
		*((uint32_t*)g_flash_recorder_ctx1.iv+3) = flash_pt;
		*((uint32_t*)g_flash_recorder_ctx1.iv+2) = prec->RotNumber;

		retv = LCRYPTO_crypt_aes256_ctr( rbuff16, rbuff16 + rsize16, rsize16 , &g_flash_recorder_ctx1, xDelayms);
		if( retv != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("WR enc Err:%d\n\r", retv);
			vPortFree( rbuff);
			return retv;
		}
		SCB_InvalidateDCache_by_Addr ( rbuff16, rsize16);

		retv=dispatcher_write_memory( (uint8_t *)flash_pt, rbuff16, prec->RecordSize, portMAX_DELAY);
		vPortFree( rbuff);
		if( retv != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("FlashWriteRecord2 Err:%d uuid:%d\n\r", retv, h->uuid);
			return retv;
		}

		prec->Pt = flash_pt + prec->RecordSize;
		prec->Idr=h->uuid;
	}
	else
	{
		uint32_t flash_pt = prec->Pt;
		state = FlashAlignPt( &flash_pt, prec);
		if( state != 0)
		{
			//Erase page
			retv=dispatcher_erase_sectors( (uint8_t *)flash_pt, 1, portMAX_DELAY);
#ifdef FLASH_RECORDER_POSITIVE_DEBUG
			dbgRecPRINTF("Erase %x\n\r", flash_pt);
#endif
			if( retv!= kStatus_QMC_Ok)
			{
				dbgRecPRINTF("FlashWriteRecord3 erase Err:%d\n\r", retv);
				return retv;	//error
			}
		}

		retv=dispatcher_write_memory( (uint8_t *)flash_pt, pt, prec->RecordSize, portMAX_DELAY);
		if( retv != kStatus_QMC_Ok)
		{
			dbgRecPRINTF("FlashWriteRecord4 Err:%d uuid:%d\n\r", retv, h->uuid);
			return retv;
		}

		prec->Pt = flash_pt + prec->RecordSize;
		prec->Idr=h->uuid;
	}

	if( state == 2 )
	{
		//Closed loop, make recorder_info_t if InfRecorder exists
		if( prec->InfRec)
		{
			//First get the last inf record if exists to get RotationNumber
			recorder_info_t inf, *pinf;
			uint32_t lastid = FlashGetLastIdr( (recorder_t *)prec->InfRec);
			if( (pinf = FlashGetRecord( lastid, (recorder_t *)prec->InfRec, &inf, portMAX_DELAY)) == NULL)
			{
				inf.RotationNumber = 1;
			}
			else
			{
				inf.RotationNumber = prec->RotNumber;
			}
			inf.RecordOrigin = 1;

			prec->RotNumber = inf.RotationNumber;
			retv = FlashWriteRecord ( &inf, (recorder_t *)prec->InfRec);
			if( retv!= kStatus_QMC_Ok)
			{
				//OK, try once to format InfRecorder
#ifdef FLASH_RECORDER_POSITIVE_DEBUG
				dbgRecPRINTF("FlashWriteRecord5 format inf\n\r");
#endif
				retv = FlashRecorderFormat( (recorder_t *)prec->InfRec);
				if( retv != kStatus_QMC_Ok)
				{
					dbgRecPRINTF("FlashWriteRecord6 inf1 Err:%d\n\r", retv);
					return retv;	//error
				}
				retv = FlashWriteRecord ( &inf, (recorder_t *)prec->InfRec);
				if( retv!= kStatus_QMC_Ok)
				{
					dbgRecPRINTF("WriteRecord write7 inf2 Err:%d\n\r", retv);
					return retv;	//error
				}
			}
		}
	}
	return retv;
}

/* Function reads and decrypts recorder data stored at *Pt. Data are copied in *record.
 * AES256-CTR is used for decryption.
 *
 * Return value:
 * kStatus_QMC_ErrMem               cannot allocate memory on heap, no read done
 * kStatus_QMC_ErrBusy              cannot get CAAM mutex
 * kStatus_QMC_Err                  general error
 * kStatus_QMC_Ok                   read succesfuly done
 */
void *FlashReadRecord( void *Pt, recorder_t *prec, void *record, TickType_t ticks)
{
	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		const size_t rsize16 = MAKE_NUMBER_ALIGN( prec->RecordSize, 16);
		if( prec->Flags & 0x1)
		{
			uint8_t *rbuff = pvPortMalloc( 2*rsize16 + 16);
			if( rbuff == NULL)
			{
				dbgRecPRINTF("dec FRR Cannot alloc mem.\n\r");
				dispatcher_release_flash_lock();
				return NULL;
			}
			uint8_t *rbuff16 = (uint8_t *)MAKE_NUMBER_ALIGN( (uint32_t)rbuff, 16);
			memcpy( rbuff16 + rsize16, Pt, prec->RecordSize);

			//Credentials IV
#ifdef NO_SBL
			memset( g_flash_recorder_ctx2.iv, 0, sizeof(g_flash_recorder_ctx2.iv));
#else
			memcpy( g_flash_recorder_ctx2.iv, (void *)g_sbl_prov_keys.nonceLog, sizeof(g_flash_recorder_ctx2.iv));
#endif
			*((uint32_t*)g_flash_recorder_ctx2.iv+3) = (uint32_t)Pt;
			if( prec->Pt < (uint32_t)Pt)
			{
				*((uint32_t*)g_flash_recorder_ctx2.iv+2) = prec->RotNumber - 1;
			}
			else
			{
				*((uint32_t*)g_flash_recorder_ctx2.iv+2) = prec->RotNumber;
			}

			qmc_status_t retv = LCRYPTO_crypt_aes256_ctr( rbuff16, rbuff16 + rsize16, rsize16 , &g_flash_recorder_ctx2, ticks);
			if( retv != kStatus_QMC_Ok)
			{
				dbgRecPRINTF("FRR dec Err:%d\n\r", retv);
				vPortFree( rbuff);
				dispatcher_release_flash_lock();
				return NULL;
			}
			SCB_InvalidateDCache_by_Addr ( rbuff16, rsize16);

			if( HashRecordCheck( rbuff16, prec) == kStatus_QMC_Ok)
			{
				if( record != NULL)
					memcpy( record, rbuff16, prec->RecordSize);
				vPortFree( rbuff);
				dispatcher_release_flash_lock();
				return Pt;
			}
			vPortFree( rbuff);
			dispatcher_release_flash_lock();
			return NULL;
		}
		else
		{
			if( record != NULL)
				memcpy( record, Pt, prec->RecordSize);
			dispatcher_release_flash_lock();
			return Pt;
		}
	}
	return NULL;
}

/* Function calculates and returns the address to recorded data based on idr (uuid).
 * Return value:
 * NULL    cannot return pointer to requested idr data
 * !=NULL  pointer to requested data
 */
void *FlashGetAddress( uint32_t idr, recorder_t *prec)
{
	if( prec==NULL)
		return NULL;
	if ( prec->Idr == 0)
		return NULL;
	if( idr > prec->Idr)
		return NULL;

	//Efective Len of required record record L = L1 + L2 + L3
	uint32_t L1, L2, L3;
	uint32_t IL, IL1, IL2, IL3, IL1p;
	uint32_t NIL2;
	const uint32_t LPSS = prec->PageSize % prec->RecordSize;	//size of unused space in the end of each page
	const uint32_t NPS  = prec->PageSize / prec->RecordSize;	//number of records per page

	IL = prec->Idr - idr + 1;	//number of records we want to go back

	IL1p = prec->Pt % prec->PageSize / prec->RecordSize;	//number of records currently fit in actual page
	IL1 = (IL > IL1p) ? IL1p : IL;
	L1 = IL1 * prec->RecordSize + ((IL > IL1p) ? LPSS : 0);	//size in bytes to required record

	NIL2 = ((IL > IL1) ? (IL - IL1 - 1) : (IL - IL1)) / NPS;
	IL2 = NIL2 * NPS;

	uint64_t tmp = (uint64_t)NIL2 * prec->PageSize;
	if( tmp > UINT32_MAX)
		return NULL;
	L2 = (uint32_t)tmp;

	IL3 = IL - (IL1 + IL2);	//IL1, IL2, IL3 that's number of records in logger
	L3 = IL3 * prec->RecordSize;

	if( prec->Pt < prec->AreaBegin)
		return NULL;

	tmp = L1 + L2 + L3;
	if( tmp > UINT32_MAX)
		return NULL;
	uint32_t Size = (uint32_t)tmp;

	if( prec->Pt < prec->AreaBegin)
		return NULL;
	uint32_t Asize = prec->Pt - prec->AreaBegin;

	if( Size <= Asize)
		return  (void *)(prec->Pt - Size);

	if( Size < Asize)
		return NULL;
	Size = Size - Asize;

	tmp = prec->AreaBegin + prec->AreaLength;
	if( tmp > UINT32_MAX)
		return NULL;
	uint32_t AreaEnd = (uint32_t)tmp;

	uint32_t Bsize = AreaEnd - prec->Pt - ( prec->PageSize - prec->Pt % prec->PageSize);
	if( Size <= Bsize)
		return (void *)(AreaEnd - Size);

	return NULL;
}

/*
 * Function gets the record by idr (uuid) key.
 * Hleda record struktury FLASH_RECORD *x ve flash podle x->idr.
 * retv:
 * != NULL  - record data is found and return pointer value points to record located in recorder!!! Data are copied to *record if != NULL
 * NULL     - record data not found
 */
void *FlashGetRecord( uint32_t idr, recorder_t *prec, void* record, TickType_t ticks)
{
	void *pt = FlashGetAddress( idr, prec);
	if( !pt) return NULL;

	pt = FlashReadRecord( pt, prec, record, ticks);
	if( pt)
	{
		if( record)
		{
			record_head_t *hf=( record_head_t *)record;
			if( hf->uuid != 0xFFFFFFFF )
			if( hf->uuid == idr )
			{
				return pt;
			}
		}
		else
		{
			return pt;
		}
	}
	return NULL;
}

/*
 * Recorder info filled into recorder_status_t *rstat.
 */
int FlashGetStatusInfo( recorder_status_t *rstat, recorder_t *prec)
{
	rstat->Idr = prec->Idr;
	rstat->Pt = prec->Pt;
	rstat->AreaBegin = prec->AreaBegin;
	rstat->AreaLength = prec->AreaLength;
	rstat->PageSize = prec->PageSize;
	rstat->RecordSize = prec->RecordSize;
	rstat->ts.seconds=0;
	rstat->ts.milliseconds=0;

	//Let's count all records.
	uint32_t LastIdr = FlashGetLastIdr( prec);
	uint32_t FirstIdr = FlashGetFirstIdr( prec);
	if( LastIdr < FirstIdr)
	{
		rstat->cnt = 0;
		return -1;
	}
	rstat->cnt = LastIdr-FirstIdr;
	return 0;
}

/*
 * Format recorder. Erase all recorder sectors.
 * if prec->inf != NULL updates and writes the inf record.
 */
qmc_status_t FlashRecorderFormat( recorder_t *prec)
{
	uint32_t sect_cn = prec->AreaLength/prec->PageSize;
	if( sect_cn > UINT16_MAX)
	{
		dbgRecPRINTF("FlashRecorderFormat sect_cn Err:%d\n\r", sect_cn);
		return kStatus_QMC_ErrArgInvalid;	//error
	}

	qmc_status_t retv = dispatcher_erase_sectors( (void *)prec->AreaBegin, (uint16_t)sect_cn, portMAX_DELAY);
	prec->Pt = prec->AreaBegin;
	prec->Idr = 0;

	if(retv != kStatus_QMC_Ok)
	{
		return retv;
	}

	//make recorder_info_t if InfRecorder exists
	if( prec->InfRec)
	{
		//First get the last inf record if exists to get RotationNumber
		recorder_info_t inf, *pinf;
		uint32_t lastid = FlashGetLastIdr( (recorder_t *)prec->InfRec);
		if( (pinf = FlashGetRecord( lastid, (recorder_t *)prec->InfRec, &inf, portMAX_DELAY)) == NULL)
		{
			inf.RotationNumber = 1;
		}
		else
		{
			inf.RotationNumber++;
		}
		inf.RecordOrigin = 0;	//Format record

		prec->RotNumber = inf.RotationNumber;
		retv = FlashWriteRecord ( &inf, (recorder_t *)prec->InfRec);
		if( retv!= kStatus_QMC_Ok)
		{
			//OK try once format the InfRecorder
			retv = FlashRecorderFormat( (recorder_t *)prec->InfRec);
			if( retv != kStatus_QMC_Ok)
			{
				dbgRecPRINTF("FlashRecorderFormat inf1 Err:%d\n\r", retv);
				return retv;	//error
			}
			retv = FlashWriteRecord ( &inf, (recorder_t *)prec->InfRec);
			if( retv!= kStatus_QMC_Ok)
			{
				dbgRecPRINTF("FlashRecorderFormat inf2 Err:%d\n\r", retv);
				return retv;	//error
			}
		}
	}

	return retv;
}

/*
FFFFFFFFFFFFFFFFFFF
DDDFFFFFDDDDDDDDDDD
FFFFDDDDDDFFFFFFFFF

F - Unused (clear 0xFF) flash space
D - Used (data) flash space

Each line represents one possible case how the recorder space can be.
FlashRecorderInit needs to evaluate start and end of the data area from the each of bellow three cases.
*/
qmc_status_t FlashRecorderInit( recorder_t *prec)
{
	qmc_status_t retv=kStatus_QMC_Err;

	if( prec == NULL)
		return kStatus_QMC_ErrArgInvalid;

	uint32_t pt=prec->AreaBegin;
	prec->Idr=0;
	
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);
	const size_t rsize16 = MAKE_NUMBER_ALIGN( prec->RecordSize, 16);
	uint8_t *rbuff16 = NULL, *rbuff = NULL;

	if( prec->Flags & 0x1)
	{
		rbuff = pvPortMalloc( 2*rsize16 + 16);
		if( rbuff == NULL)
		{
			dbgRecPRINTF("dec FRI Cannot alloc mem.\n\r");
			return kStatus_QMC_ErrMem;
		}
		rbuff16 = (uint8_t *)MAKE_NUMBER_ALIGN( (uint32_t)rbuff, 16);

		//First get the last inf record if exists to get RotationNumber
		if( prec->InfRec)
		{
			recorder_info_t inf;
			uint32_t lastid = FlashGetLastIdr( (recorder_t *)prec->InfRec);
			if( FlashGetRecord( lastid, (recorder_t *)prec->InfRec, &inf, xDelayms) == NULL)
			{
				dbgRecPRINTF("dec FRI Cannot read recorder_t record.\n\r");
				vPortFree( rbuff);
				return kStatus_QMC_Err;
			}
			prec->RotNumber = inf.RotationNumber;
		}
	}

#ifdef NO_SBL
	memset( g_flash_recorder_ctx1.key, 0, sizeof(g_flash_recorder_ctx1.key));
	memset( g_flash_recorder_ctx2.key, 0, sizeof(g_flash_recorder_ctx2.key));
#else
	memcpy( g_flash_recorder_ctx1.key, (void *)g_sbl_prov_keys.aesKeyLog, sizeof(g_flash_recorder_ctx1.key));
	memcpy( g_flash_recorder_ctx2.key, (void *)g_sbl_prov_keys.aesKeyLog, sizeof(g_flash_recorder_ctx2.key));
#endif

//First of all let's try to go through block of 0xFFFFs.
	for(;;)
	{
		if( FlashAlignPt( &pt, prec) == 2)	//When we crossed last address of last sector in recorder.
			break;
		if( ((record_head_t *)pt)->uuid != 0xFFFFFFFF)	//When some data has been found..
			break;
		pt+=prec->RecordSize;
	}
	
	//We found IDR!=0xFFFF. So let's try to go through some data.
	for(;;)
	{
		if( FlashAlignPt( &pt, prec) == 2)	//When we crossed last address of last sector in recorder.
		{
			retv=kStatus_QMC_Ok;
			break;
		}

		if( ((record_head_t *)pt)->uuid == 0xFFFFFFFF)	//When there are no data anymore.
		{
			retv=kStatus_QMC_Ok;
			break;
		}

		if( prec->Flags & 0x1)
		{
			//Compose nonce
#ifdef NO_SBL
			memset( g_flash_recorder_ctx2.iv, 0, sizeof(g_flash_recorder_ctx2.iv));
#else
			memcpy( g_flash_recorder_ctx2.iv, (void *)g_sbl_prov_keys.nonceLog, sizeof(g_flash_recorder_ctx2.iv));
#endif
			*((uint32_t*)g_flash_recorder_ctx2.iv+3) = (uint32_t)pt;	//modify the IV
			*((uint32_t*)g_flash_recorder_ctx2.iv+2) = prec->RotNumber;

			memcpy( rbuff16 + rsize16, (void *)pt, prec->RecordSize);

			retv = LCRYPTO_crypt_aes256_ctr( rbuff16, rbuff16 + rsize16, rsize16 , &g_flash_recorder_ctx2, xDelayms);
			if( retv != kStatus_QMC_Ok)
			{
				dbgRecPRINTF("FRI dec Err:%d\n\r", retv);
				break;
			}
			SCB_InvalidateDCache_by_Addr ( rbuff16, rsize16);
			retv = HashRecordCheck( rbuff16, prec);
			if( retv != kStatus_QMC_Ok ) //Record validation.
			{
				dbgRecPRINTF("FRI XXX:%p\n\r", pt);
				break;
			}
			prec->Idr=(( record_head_t *)rbuff16)->uuid;
		}
		else
		{
			retv = HashRecordCheck( (void*)pt, prec);
			if( retv != kStatus_QMC_Ok ) //Record validation.
			{
				dbgRecPRINTF("FRI XXX1:%p\n\r", pt);
				break;
			}
			prec->Idr=(( record_head_t *)pt)->uuid;
		}
		pt+=prec->RecordSize;
	}
	if( prec->Flags & 0x1)
		vPortFree( rbuff);
	prec->Pt=(uint32_t)pt;
	return retv;
}

bool FlashNextWriteEraseSector( recorder_t *prec)
{
	uint32_t flash_pt = prec->Pt;
	return (FlashAlignPt( &flash_pt, prec) != 0);
}

uint32_t FlashGetFirstIdr( recorder_t *prec)
{
	const TickType_t xDelayms = pdMS_TO_TICKS( DATALOGGER_MUTEX_XDELAYS_MS);
	uint8_t record[ prec->RecordSize];
	uint32_t PtSecBegin = prec->Pt - prec->Pt % OCTAL_FLASH_SECTOR_SIZE;
	uint32_t PtSec = PtSecBegin;
	for(;;)
	{
		uint64_t tmp = (uint64_t)PtSec + OCTAL_FLASH_SECTOR_SIZE;
		if( tmp > UINT32_MAX)
			return 0;
		PtSec = (uint32_t)tmp;

		FlashAlignPt( &PtSec, prec);
		if( PtSecBegin == PtSec)
			break;
		if( FlashReadRecord( (void *)PtSec, prec, record, xDelayms) != NULL)
		{
			record_head_t *hf=( record_head_t *)record;
			if( hf->uuid != 0xFFFFFFFF )
				return hf->uuid;
		}
	}
	return 0;
}

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#include "main_cm7.h"
#include "app.h"
#include "api_board.h"
#include <api_logging.h>
#include <dispatcher.h>

//#include <sha256.h>


static qmc_status_t HashRecordCheck( void *pt, recorder_t *prec)
{
#ifdef DATALOGGER_FLASH_RECORDER_USE_HASH
    uint8_t buf[ prec->RecordSize];
    memcpy( buf, pt, prec->RecordSize);
#if 0
    mbedtls_sha256_starts( &flash_recorder_ctx_sha256, 0);
    mbedtls_sha256_update( &flash_recorder_ctx_sha256, (uint8_t*)pt + FLASH_RECORDER_HASH_SIZE, prec->RecordSize - FLASH_RECORDER_HASH_SIZE);
    mbedtls_sha256_finish( &flash_recorder_ctx_sha256, (uint8_t*)output);
#else
    mbedtls_sha256_starts( prec->flash_recorder_ctx, 0);
    mbedtls_sha256_update( prec->flash_recorder_ctx, buf + FLASH_RECORDER_HASH_SIZE, prec->RecordSize - FLASH_RECORDER_HASH_SIZE);
    mbedtls_sha256_finish( prec->flash_recorder_ctx, buf);
#endif
    uint32_t *pt_dst = (uint32_t*)pt;
    uint32_t *pt_src = (uint32_t*)buf;
    uint32_t *pt_src_e = pt_src + (FLASH_RECORDER_HASH_SIZE/4);
    while( pt_src < pt_src_e)
    {
    	if( *pt_src != *pt_dst)
    		return kStatus_QMC_Err;
    	pt_src++;
    	pt_dst++;
    }
#else
    uint8_t *ptb = (uint8_t*)pt;
    ptb += FLASH_RECORDER_HASH_SIZE;
    uint32_t sum = 0;
    int i;
    for( i=0; i<prec->RecordSize - FLASH_RECORDER_HASH_SIZE; i++)
    {
    	sum += *ptb++;
    }
    if( *(uint32_t*)pt != sum)
    	return kStatus_QMC_Err;
#endif
	return kStatus_QMC_Ok;
}

static void HashRecordUpdate( void *pt, recorder_t *prec)
{
#ifdef DATALOGGER_FLASH_RECORDER_USE_HASH
	uint8_t buf[ prec->RecordSize];
    memcpy( buf, pt, prec->RecordSize);

    mbedtls_sha256_starts( prec->flash_recorder_ctx, 0);
    mbedtls_sha256_update( prec->flash_recorder_ctx, buf + FLASH_RECORDER_HASH_SIZE, prec->RecordSize - FLASH_RECORDER_HASH_SIZE);
    mbedtls_sha256_finish( prec->flash_recorder_ctx, buf);
    memcpy( pt, buf, FLASH_RECORDER_HASH_SIZE);
#else
    uint8_t *ptb = (uint8_t*)pt;
    ptb += FLASH_RECORDER_HASH_SIZE;
    uint32_t sum = 0;
    int i;
    for( i=0; i<prec->RecordSize - FLASH_RECORDER_HASH_SIZE; i++)
    {
    	sum += *ptb++;
    }
    *(uint32_t*)pt = sum;
#endif
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
 * Function writes record into the recorder.
 * uuid, timestamp_s, timestamp_ms are updated.
 * This function needs to be reetrant!
 */
qmc_status_t FlashWriteRecord( void *pt, recorder_t *prec)
{
	qmc_status_t retv;
	record_head_t *h=( record_head_t *)pt;
	h->uuid=prec->Idr;
	h->uuid++;
	if( h->uuid==0xFFFFFFFF)	//0xFFFFFFFF is reserved for "clear space"
		h->uuid=0;

	//TODO need implementation of BOARD_GetTime
#if 1
    qmc_timestamp_t	ts = {0,0};
	retv = BOARD_GetTime( &ts);
	if( retv!= kStatus_QMC_Ok)
	{
		//dbgRecPRINTF("FlashWriteRecord TS Err:%d\n\r", retv);
	}
	else
	{
		//dbgRecPRINTF("FlashWriteRecord TS:%d:%d\n\r", ts.seconds, ts.milliseconds);
	}
	h->ts = ts;
#else
	h->ts.seconds=0;
	h->ts.milliseconds=0;
#endif

	HashRecordUpdate( pt, prec);

	uint32_t flash_pt = prec->Pt;
	int state = FlashAlignPt( &flash_pt, prec);
	if( state != 0)
	{
		//Erase page
		retv=dispatcher_erase_sectors( (uint8_t *)flash_pt, 1, portMAX_DELAY);
		dbgRecPRINTF("Erase %x\n\r", flash_pt);
		if( retv!= kStatus_QMC_Ok)
		{
			dbgRecPRINTF("FlashWriteRecord erase Err:%d\n\r", retv);
			return retv;	//error
		}
	}

	retv=dispatcher_write_memory( (uint8_t *)flash_pt, pt, prec->RecordSize, portMAX_DELAY);
	if( retv != kStatus_QMC_Ok)
	{
		dbgRecPRINTF("FlashWriteRecord Err:%d uuid:%d\n\r", retv, h->uuid);
		return retv;
	}

	prec->Pt = flash_pt + prec->RecordSize;
	prec->Idr=h->uuid;

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
				inf.RotationNumber++;
			}
			inf.RecordOrigin = 1;

			retv = FlashWriteRecord ( &inf, (recorder_t *)prec->InfRec);
			if( retv!= kStatus_QMC_Ok)
			{
				//OK, try once to format InfRecorder
				dbgRecPRINTF("FlashWriteRecord format inf\n\r");
				retv = FlashRecorderFormat( (recorder_t *)prec->InfRec);
				if( retv != kStatus_QMC_Ok)
				{
					dbgRecPRINTF("FlashWriteRecord inf1 Err:%d\n\r", retv);
					return retv;	//error
				}
				retv = FlashWriteRecord ( &inf, (recorder_t *)prec->InfRec);
				if( retv!= kStatus_QMC_Ok)
				{
					dbgRecPRINTF("WriteRecord write inf2 Err:%d\n\r", retv);
					return retv;	//error
				}
			}
#if 1
			recorder_status_t rstat;
			FlashGetStatusInfo( &rstat, (recorder_t *)prec->InfRec);
			dbgRecPRINTF("InfStat uuid:%x cnt:%d Pt:%x\n\r", rstat.Idr, rstat.cnt, rstat.Pt);
#endif


		}
	}
	return retv;
}

/*
 * Function gets the record by idr (uuid) key.
 * Hleda record struktury FLASH_RECORD *x ve flash podle x->idr.
 * retv: 0 - record is found and return pointer value points to record located in recorder!!!
 *      -1 - record hasn't been found
 */
void *FlashGetRecord( uint32_t idr, recorder_t *prec, void* record, TickType_t ticks)
{
	uint32_t ptf=prec->Pt;
	uint32_t size=prec->AreaLength;

	//First of all we need to obtain flash_lock to be sure nobody is using flash now
	if( dispatcher_get_flash_lock( ticks) == kStatus_QMC_Ok)
	{
		while( size >= prec->RecordSize)
		{
			size-=prec->RecordSize;
			ptf-=prec->RecordSize;
			if( ptf < prec->AreaBegin)	//when we cross the begin
				ptf=prec->AreaBegin + prec->AreaLength - prec->RecordSize;

			uint32_t off=ptf % prec->PageSize % prec->RecordSize;	//Align for sizeof(FLASH_RECORD) since the begin of the sector
			size-=off;
			ptf-=off;

			record_head_t *hf=( record_head_t *)ptf;
			if( hf->uuid == 0xFFFFFFFF )	//when not found
				break;
			if( hf->uuid == idr )
			{
				//if( hf->chksum == SumMem( (uint8_t*)&((record_head_t*)ptf)->uuid, prec->RecordSize-sizeof(((record_head_t*)ptf)->hash)) )
				if( HashRecordCheck( (void*)ptf, prec) == kStatus_QMC_Ok)
				{
					if( record != NULL)
						memcpy( record, (void*)ptf, prec->RecordSize);
					dispatcher_release_flash_lock();
					return (void*)ptf;	//Pointer to record in recorder.
				}
				else
				{
					break;	//Chksum error. NULL returned.
				}
			}
		}
		dispatcher_release_flash_lock();
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
	uint32_t idr = FlashGetLastIdr( prec);
	uint32_t LastIdr = idr;
	while( FlashGetRecord( idr, prec, NULL, portMAX_DELAY) != NULL) { idr--; }
	rstat->cnt = LastIdr-idr;
	return 0;
}

/*
 * Format recorder. Erase all recorder sectors.
 */
qmc_status_t FlashRecorderFormat( recorder_t *prec)
{
	qmc_status_t retv = dispatcher_erase_sectors( (void *)prec->AreaBegin, prec->AreaLength/prec->PageSize, portMAX_DELAY);
	prec->Pt = prec->AreaBegin;
	prec->Idr = 0;

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
	qmc_status_t retv;
	uint32_t pt=prec->AreaBegin;
	prec->Idr=0;
	
#ifdef DATALOGGER_FLASH_RECORDER_USE_HASH
	prec->flash_recorder_ctx = pvPortMalloc( sizeof( mbedtls_sha256_context ));
	if( prec->flash_recorder_ctx == NULL)
	{
		dbgPRINTF("FlashRecorderInit: Cannot alloc heap memory for handler. S;%d\n\r", sizeof( mbedtls_sha256_context));
		return kStatus_QMC_Err;
	}
	mbedtls_sha256_init( prec->flash_recorder_ctx);
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
		if( HashRecordCheck( (void*)pt, prec) != kStatus_QMC_Ok ) //Record validation.
		{
			retv=kStatus_QMC_Err;
			break;
		}

		prec->Idr=(( record_head_t *)pt)->uuid;
		pt+=prec->RecordSize;
	}
	prec->Pt=(uint32_t)pt;
	return retv;
}

uint32_t FlashGetLastIdr( recorder_t *prec)
{
	return prec->Idr;
}

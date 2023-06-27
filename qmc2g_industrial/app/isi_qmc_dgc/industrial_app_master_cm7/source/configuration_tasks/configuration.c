/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "main_cm7.h"
#include "app.h"
#include "lcrypto.h"
#include "configuration.h"
#include "dispatcher.h"

//Configuration items data are allocated on the RTOS heap by pvPortMalloc.

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Globals
 ******************************************************************************/
extern uint32_t  __FWU_STORAGE_START[];
extern uint32_t  __FWU_STORAGE_SIZE[];

static StaticSemaphore_t gs_configMutex;
SemaphoreHandle_t g_config_xSemaphore = NULL;

//Config defaults
static const cnf_data_t gs_cnf_data_default = {{
		{ kCONFIG_KeyUser1Id,   "Test_Cnf_1", "Test_Cnf_1_Value" },
		{ kCONFIG_KeyUser2Id,   "Test_Cnf_2", "Test_Cnf_2_Value" },
		{ kCONFIG_KeyUser2Role, "Test_Cnf_3", "Test_Cnf_3_Value" },
		{ kCONFIG_Key_Ip,       "IP_config", {192,168,0,1} },
		{ kCONFIG_Key_Ip_mask,  "IP_mask_config", {255,255,255,0} },
		{ kCONFIG_Key_Ip_GW,	"IP_gateway", {192,168,0,254}},
		{ kCONFIG_Key_Ip_DNS, 	"IP_DNS", {192,168,0,254}},
		{ kCONFIG_Key_MAC_address, "MAC_address", {0x02, 0x00, 0x00, 0x00, 0x00, 0x00}},
		{ kCONFIG_Key_VLAN_ID, "TSN_VLAN_ID", {2}},
		{ kCONFIG_Key_TSN_RX_STREAM_MAC_ADDR, "TSN_RX_Stream_MAC", {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x70}},
		{ kCONFIG_Key_TSN_TX_STREAM_MAC_ADDR, "TSN_TX_Stream_MAC", {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x71}},
}};

AT_NONCACHEABLE_SECTION_ALIGN(static cnf_struct_t gs_cnf_struct_data, 16);

mbedtls_sha256_context g_cnf_sha256_ctx __ALIGNED(32);

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t gs_shabuf[CONFIGURATION_HASH_SIZE], 16);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t gs_in_buf[OCTAL_FLASH_SECTOR_SIZE], 16);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t gs_out_buf[OCTAL_FLASH_SECTOR_SIZE], 16);
AT_NONCACHEABLE_SECTION_ALIGN(lcrypto_aes_ctx_t g_config_aes_ctx, 16);

/*******************************************************************************
 * Code
 ******************************************************************************/

//Blocking function to get dedicated access to config globals
static qmc_status_t config_get_lock( TickType_t ticks)
{
	if( g_config_xSemaphore != NULL)
	if( xSemaphoreTake( g_config_xSemaphore, ticks ) == pdTRUE )
	{
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrBusy;
}

static qmc_status_t config_release_lock()
{
	if( xSemaphoreGive( g_config_xSemaphore ) != pdTRUE )
	{
		return kStatus_QMC_Err;
	}
	return kStatus_QMC_Ok;
}

static void CONFIG_get_sum( uint8_t *value, TickType_t ticks)
{
	LCRYPTO_get_sha256( gs_shabuf, (uint8_t*)gs_cnf_struct_data.cnf_data, sizeof( gs_cnf_struct_data.cnf_data), &g_cnf_sha256_ctx, ticks);
	memcpy( value, gs_shabuf, CONFIGURATION_HASH_SIZE);
}

qmc_status_t CONFIG_UpdateFlash( )
{
	qmc_status_t retv = kStatus_QMC_Err;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);
	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{

		retv = dispatcher_erase_sectors( (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN ,RECORDER_REC_CONFIG_AREALENGTH/OCTAL_FLASH_SECTOR_SIZE , portMAX_DELAY);
		dbgCnfPRINTF("Erase %x\n\r",RECORDER_REC_CONFIG_AREABEGIN );
		if( retv!= kStatus_QMC_Ok)
		{
			dbgCnfPRINTF("ER Err:%d\n\r", retv);
			config_release_lock();
			return retv;	//error
		}

		CONFIG_get_sum( gs_cnf_struct_data.hash, xDelayms);

#ifdef NO_SBL
		memset( g_config_aes_ctx.iv, 0, sizeof( g_config_aes_ctx.iv));
#else
		memcpy( g_config_aes_ctx.iv, (void *)g_sbl_prov_keys.nonceConfig, sizeof( g_config_aes_ctx.iv));
#endif

		const size_t bsize = sizeof(gs_in_buf);
		size_t size = sizeof(cnf_struct_t);
		uint8_t *psrc = (uint8_t*)&gs_cnf_struct_data;
		uint8_t *pdst = (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN;

		while( size)
		{
			memcpy( gs_in_buf, psrc, bsize);
			retv = LCRYPTO_encrypt_aes256_cbc( gs_out_buf, gs_in_buf, bsize, &g_config_aes_ctx, xDelayms);
			if( retv != kStatus_QMC_Ok)
			{
				dbgCnfPRINTF("enc Err:%d\n\r", retv);
				break;
			}
			retv = dispatcher_write_memory( pdst, gs_out_buf, bsize, portMAX_DELAY);
			pdst += bsize;
			psrc += bsize;
			size = size > bsize ? size-bsize : 0;
			if( retv != kStatus_QMC_Ok)
			{
				dbgCnfPRINTF("WR Err:%d\n\r", retv);
				break;
			}
		}
		config_release_lock();
		return retv;
	}
	return kStatus_QMC_Err;
}


static qmc_status_t CONFIG_ReadFlash()
{
	qmc_status_t retv = kStatus_QMC_Err;

	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

#ifdef NO_SBL
	memset( g_config_aes_ctx.iv, 0, sizeof( g_config_aes_ctx.iv));
#else
	memcpy( g_config_aes_ctx.iv, g_sbl_prov_keys.nonceConfig, sizeof( g_config_aes_ctx.iv));
#endif

	const size_t bsize = sizeof(gs_in_buf);
	size_t size = sizeof(cnf_struct_t);
	uint8_t *pdst = (uint8_t *)&gs_cnf_struct_data;
	uint8_t *psrc = (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN;

	while( size)
	{
		retv = dispatcher_read_memory( gs_in_buf, psrc, bsize, portMAX_DELAY);
		if( retv != kStatus_QMC_Ok)
		{
			dbgCnfPRINTF("RD Err:%d Configuration.\n\r", retv);
			return retv;
		}
		retv = LCRYPTO_decrypt_aes256_cbc( gs_out_buf, gs_in_buf, bsize, &g_config_aes_ctx, xDelayms);
		if( retv != kStatus_QMC_Ok)
		{
			dbgCnfPRINTF("dec Err:%d Configuration.\n\r", retv);
			return retv;
		}

		if( size > bsize)
		{
			memcpy( pdst, gs_out_buf, bsize );
			size-=bsize;
		}
		else
		{
			memcpy( pdst, gs_out_buf, size);
			size=0;
		}
		pdst += bsize;
		psrc += bsize;
	}
	return kStatus_QMC_Ok;
}

qmc_status_t CONFIG_Init()
{
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( g_config_xSemaphore == NULL)
	{
		g_config_xSemaphore = xSemaphoreCreateMutexStatic( &gs_configMutex);
		if( g_config_xSemaphore == NULL)
		{
			dbgCnfPRINTF("Mutex init fail. Configuration.\r\n");
			return kStatus_QMC_Err;
		}
		dbgCnfPRINTF("Mutex init ok. Configuration.\r\n");
	}

	if( config_get_lock( xDelayms) != kStatus_QMC_Ok)
	{
		dbgCnfPRINTF("CONFIG_Init mutex fail. Configuration.\r\n");
		return kStatus_QMC_Err;
	}

#ifdef NO_SBL
	memset( g_config_aes_ctx.key, 0, sizeof( g_config_aes_ctx.key));
	memset( g_config_aes_ctx.iv, 0, sizeof( g_config_aes_ctx.iv));
#else
	memcpy( g_config_aes_ctx.key, (void *)g_sbl_prov_keys.aesKeyConfig, sizeof( g_config_aes_ctx.key));
	memcpy( g_config_aes_ctx.iv, (void *)g_sbl_prov_keys.nonceConfig, sizeof( g_config_aes_ctx.iv));
#endif

	if( CONFIG_ReadFlash() != kStatus_QMC_Ok)
	{
		config_release_lock();
		return kStatus_QMC_Err;
	}

	CONFIG_get_sum( gs_shabuf, xDelayms);
	if( memcmp( gs_shabuf, gs_cnf_struct_data.hash, CONFIGURATION_HASH_SIZE) != 0)
	{
		dbgCnfPRINTF("Read flash hash fail. Using defaults. Configuration.\r\n");
		memcpy( gs_cnf_struct_data.cnf_data, &gs_cnf_data_default, sizeof( cnf_data_t) );
	}
	else
	{
		dbgCnfPRINTF("Read flash hash match. Configuration.\n\r");
	}

	config_release_lock();
	return kStatus_QMC_Ok;
}

static qmc_status_t CONFIG_GetIndexByKey(const unsigned char *key, int *index)
{
	int i;
	int cnt = sizeof( cnf_data_t)/sizeof( cnf_record_t);
	for( i=0; i < cnt; i++)
	{
		if( strncmp( (const char *)gs_cnf_struct_data.cnf_data[i].key, (const char *)key, CONFIG_MAX_KEY_LEN) == 0)
		{
			*index = i;
			return kStatus_QMC_Ok;
		}
	}
	return kStatus_QMC_ErrRange;
}

static qmc_status_t CONFIG_GetIndexById( config_id_t id, int *index)
{
	int i;
	int cnt = sizeof( cnf_data_t)/sizeof( cnf_record_t);
	for( i=0; i < cnt; i++)
	{
		if( gs_cnf_struct_data.cnf_data[i].id == id)
		{
			*index = i;
			return kStatus_QMC_Ok;
		}
	}
	return kStatus_QMC_ErrRange;
}

qmc_status_t CONFIG_GetStrValue(const unsigned char* key, unsigned char* value)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( (key == NULL) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexByKey( key, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		strncpy( (char *)value, (const char *)gs_cnf_struct_data.cnf_data[i].value, sizeof(gs_cnf_struct_data.cnf_data[i].value));
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_GetValue Mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_GetStrValueById(config_id_t id, unsigned char* value)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( (id == kCONFIG_KeyNone) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexById( id, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		strncpy( (char *)value, (const char *)gs_cnf_struct_data.cnf_data[i].value, sizeof(gs_cnf_struct_data.cnf_data[i].value));
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_GetValueById Mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_SetStrValue(const unsigned char* key, const unsigned char* value)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( (key == NULL) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexByKey( key, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		strncpy( (char *)gs_cnf_struct_data.cnf_data[i].value, (const char *)value, sizeof(gs_cnf_struct_data.cnf_data[i].value));
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_SetValue mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_SetStrValueById(config_id_t id, const unsigned char* value)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( (id == kCONFIG_KeyNone) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexById( id, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		strncpy( (char *)gs_cnf_struct_data.cnf_data[i].value, (const char *)value, sizeof(gs_cnf_struct_data.cnf_data[i].value));
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_SetValueById mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_GetBinValue(const unsigned char* key, unsigned char* value, const uint16_t length)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( length > CONFIG_MAX_VALUE_LEN)
		return kStatus_QMC_ErrArgInvalid;

	if( (key == NULL) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexByKey( key, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		memcpy( (char *)value, (const char *)gs_cnf_struct_data.cnf_data[i].value, length);
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_GetValue Mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_GetBinValueById(config_id_t id, unsigned char* value, const uint16_t length)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( length > CONFIG_MAX_VALUE_LEN)
		return kStatus_QMC_ErrArgInvalid;

	if( (id == kCONFIG_KeyNone) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexById( id, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		memcpy( (char *)value, (const char *)gs_cnf_struct_data.cnf_data[i].value, length);
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_GetValueById Mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_SetBinValue(const unsigned char* key, const unsigned char* value, const uint16_t length)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( length > CONFIG_MAX_VALUE_LEN)
		return kStatus_QMC_ErrArgInvalid;

	if( (key == NULL) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexByKey( key, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		memcpy( (char *)gs_cnf_struct_data.cnf_data[i].value, (const char *)value, length);
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_SetValue mutex Err.\n\r");
	return kStatus_QMC_Err;
}

qmc_status_t CONFIG_SetBinValueById(config_id_t id, const unsigned char* value, const uint16_t length)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( length > CONFIG_MAX_VALUE_LEN)
		return kStatus_QMC_ErrArgInvalid;

	if( (id == kCONFIG_KeyNone) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexById( id, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kStatus_QMC_ErrRange;
		}

		memcpy( (char *)gs_cnf_struct_data.cnf_data[i].value, (const char *)value, length);
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	dbgCnfPRINTF("CONFIG_SetValueById mutex Err.\n\r");
	return kStatus_QMC_Err;
}

config_id_t CONFIG_GetIdfromKey(const unsigned char* key)
{
	int i;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( key == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		if( CONFIG_GetIndexByKey( key, &i) != kStatus_QMC_Ok)
		{
			config_release_lock();
			return kCONFIG_KeyNone;
		}

		config_release_lock();
		return gs_cnf_struct_data.cnf_data[i].id;
	}
	dbgCnfPRINTF("CONFIG_GetIdfromKey mutex Err.\n\r");
	return kCONFIG_KeyNone;
}

qmc_status_t CONFIG_GetIntegerFromValue(const unsigned char* value, int* integer)
{
	if( (integer == NULL) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	size_t size = strnlen( (const char *)value, CONFIG_MAX_VALUE_LEN);
	if( (size > 0) && (size < CONFIG_MAX_VALUE_LEN))
	{
		*integer = atoi( (const char *)value);
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrArgInvalid;
}

qmc_status_t CONFIG_SetIntegerAsValue(int integer, size_t valueLen, unsigned char* value)
{
	if( value == NULL)
		return kStatus_QMC_ErrArgInvalid;

	itoa( integer, (char *)value, 10);
	return kStatus_QMC_Ok;
}

qmc_status_t CONFIG_GetBooleanFromValue(const unsigned char* value, bool* boolean)
{
	const char words[][6] = { "true", "false", "yes", "no", "ok", "err", "on", "off", "1", "0" };
	int i, cnt;
	size_t size = strnlen( (const char *)value, CONFIG_MAX_VALUE_LEN);

	if( (boolean == NULL) || (value == NULL) )
		return kStatus_QMC_ErrArgInvalid;

	if( (size > 0) && (size < CONFIG_MAX_VALUE_LEN))
	{
		cnt = sizeof(words)/sizeof(words[0][0]);
		for( i=0; i < cnt; i++)
		{
			if( strncasecmp( (const char *)value, words[i], sizeof(words[0][0])) == 0)
			{
				if( i&1)
					*boolean = false;
				else
					*boolean = true;
				return kStatus_QMC_Ok;
			}
		}
	}
	return kStatus_QMC_ErrArgInvalid;
}

qmc_status_t CONFIG_SetBooleanAsValue(bool boolean, size_t valueLen, unsigned char* value)
{
	const char *str_true =  "1";
	const char *str_false = "0";

	if( value == NULL)
		return kStatus_QMC_ErrArgInvalid;

	if( (valueLen < sizeof( str_true)) || (valueLen < sizeof( str_false)) )
			return kStatus_QMC_ErrRange;

	if( boolean == false)
		strncpy( (char *)value, str_false, sizeof( str_false));
	else
		strncpy( (char *)value, str_true, sizeof( str_true));

	return kStatus_QMC_Ok;
}

/*!
 * @brief Write a chunk of data to the firmware update location.
 *
 * @param[in] offset Position to write the chunk of data to
 * @param[in] data Pointer to the data to write
 * @param[in] dataLen Length of the data to write
 */
qmc_status_t CONFIG_WriteFwUpdateChunk(size_t offset, const uint8_t* data, size_t dataLen)
{
	qmc_status_t retv = kStatus_QMC_Err;
	const TickType_t xDelayms = pdMS_TO_TICKS( CONFIG_MUTEX_XDELAYS_MS);

	if( dataLen == 0)
		return kStatus_QMC_Ok;

	if( dataLen & 0x1) //Invalid dataLen. dataLen must be aligned with 16bits.
	{
		dbgCnfPRINTF("Invalid even dataLen: 0x%x. CONFIG_WriteFwUpdateChunk. Configuration.\r\n", dataLen);
		return kStatus_QMC_ErrArgInvalid;
	}
	uint32_t flashAddr = (uint32_t)__FWU_STORAGE_START + offset;
	uint32_t fwsize = (uint32_t)__FWU_STORAGE_SIZE;

	if( (offset > fwsize) || (dataLen > fwsize) || ((offset+dataLen) > fwsize))
	{
		dbgCnfPRINTF("Invalid space requirement. dataLen: 0x%x  offset: 0x%x  FW_STORAGE_SIZE: 0x%x\r\n. CONFIG_WriteFwUpdateChunk. Configuration.\r\n", dataLen, offset, fwsize);
		return kStatus_QMC_ErrRange;
	}

	if( config_get_lock( xDelayms) == kStatus_QMC_Ok)
	{
		size_t size = dataLen;
		uint8_t *p_data = (uint8_t *)data;

		while( size)
		{
			//get sector address in OCTAL FLASH
			uint32_t sectorDataOffset = flashAddr % OCTAL_FLASH_SECTOR_SIZE;
			uint32_t sectorAddr = flashAddr - sectorDataOffset;

			//read content of sector
			retv = dispatcher_read_memory( gs_in_buf, (void *)sectorAddr, OCTAL_FLASH_SECTOR_SIZE, xDelayms);
			if( retv != kStatus_QMC_Ok)
			{
				dbgCnfPRINTF("CONFIG_WriteFwUpdateChunk. Read Err:%d\n\r", retv);
				config_release_lock();
				return retv;	//error
			}

			//get size of data within the sector
			size_t s;
			if( size > OCTAL_FLASH_SECTOR_SIZE - sectorDataOffset)
			{
				s = OCTAL_FLASH_SECTOR_SIZE - sectorDataOffset;
			}
			else
			{
				s = size;
			}

			//modify sector content by incoming data
			memcpy( gs_in_buf + sectorDataOffset, p_data, s);

			//erase sector in Octal flash
			retv = dispatcher_erase_sectors( (void *)sectorAddr, 1, portMAX_DELAY);
			dbgCnfPRINTF("CONFIG_WriteFwUpdateChunk. Erase %x\n\r", sectorAddr);
			if( retv != kStatus_QMC_Ok)
			{
				dbgCnfPRINTF("CONFIG_WriteFwUpdateChunk. Erase Err:%d\n\r", retv);
				config_release_lock();
				return retv;	//error
			}

			//write former + modified data back to flash sector
			retv = dispatcher_write_memory( (void *)sectorAddr, gs_in_buf, s + sectorDataOffset, xDelayms);
			if( retv != kStatus_QMC_Ok)
			{
				dbgCnfPRINTF("CONFIG_WriteFwUpdateChunk. Write Err %x\n\r", retv);
			}

			size-=s;
			flashAddr+=s;
			p_data+=s;
		}

		if( config_release_lock() != kStatus_QMC_Ok)
		{
			dbgCnfPRINTF("CONFIG_WriteFwUpdateChunk. config_release_lock() err\n\r");
			return kStatus_QMC_Err;
		}
	}

	return retv;
}

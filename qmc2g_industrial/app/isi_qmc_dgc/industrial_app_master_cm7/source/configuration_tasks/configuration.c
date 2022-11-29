/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "main_cm7.h"
#include "app.h"
#include "configuration.h"
#include "dispatcher.h"

//Configuration items data are allocated on the RTOS heap by pvPortMalloc.

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef FEATURE_CONFIG_USE_CRYPTO
#define AES256_CBC_KEY_SIZE (32)
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Globals
 ******************************************************************************/

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

static cnf_struct_t gs_cnf_struct_data __ALIGNED(16);
static cnf_struct_t *gs_cnf_struct = &gs_cnf_struct_data;
//mbedtls_sha256_context cnf_sha256_context __ALIGNED(32);
//lcrypto_aes_ctx_t cnf_aes_context;

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

static void CONFIG_get_sum( uint8_t *value)
{
#ifdef FEATURE_CONFIG_USE_HASH

	LCRYPTO_get_sha256( value, (uint8_t*)gs_cnf_struct->cnf_data, sizeof( gs_cnf_struct->cnf_data), &cnf_sha256_context);

#else

	uint8_t *ptb = (uint8_t*)gs_cnf_struct->cnf_data;
	uint8_t *pte = ptb + sizeof( gs_cnf_struct->cnf_data);
	uint32_t chksum = 0;
	while( ptb < pte)
		chksum += *ptb++;

	*(uint32_t*)value = chksum;
#endif
}

qmc_status_t CONFIG_UpdateFlash( )
{
	qmc_status_t retv;
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

#ifdef FEATURE_CONFIG_USE_HASH
		CONFIG_get_sum( gs_cnf_struct->hash);
#else
		CONFIG_get_sum( (uint8_t *)&gs_cnf_struct->chksum);
#endif

#ifdef FEATURE_CONFIG_USE_CRYPTO
		uint8_t *base_buf = pvPortMalloc( sizeof(cnf_struct_t) + 32);
		if( base_buf == NULL)
		{
			dbgCnfPRINTF("Err. Cannot alloc memory on heap %d.\n\r", sizeof(cnf_struct_t) + 32);
			config_release_lock();
			return kStatus_QMC_Err;
		}
		uint8_t *crypt_buf = base_buf + ((uint32_t)base_buf % 32);	//allign to 32


		LCRYPTO_encrypt_aes256_cbc( crypt_buf, (uint8_t *)gs_cnf_struct, sizeof( cnf_struct_t), &cnf_aes_context);


		retv = dispatcher_write_memory( (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN, crypt_buf, sizeof( cnf_struct_t), portMAX_DELAY);
		vPortFree( base_buf);
#else
		retv = dispatcher_write_memory( (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN, gs_cnf_struct, sizeof( cnf_struct_t), portMAX_DELAY);
#endif
		if( retv != kStatus_QMC_Ok)
		{
			dbgCnfPRINTF("WR Err:%d\n\r", retv);
			config_release_lock();
			return retv;
		}
		config_release_lock();
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_Err;
}

static qmc_status_t CONFIG_ReadFlash()
{
#ifdef FEATURE_CONFIG_USE_CRYPTO
		uint8_t *base_buf = pvPortMalloc( sizeof(cnf_struct_t) + 32);
		if( base_buf == NULL)
		{
			dbgCnfPRINTF("Err. Cannot alloc memory on heap %d.\n\r", sizeof(cnf_struct_t) + 32);
			return kStatus_QMC_Err;
		}
		uint8_t *crypt_buf = base_buf + ((uint32_t)base_buf % 32);	//allign to 32

		if( dispatcher_read_memory( crypt_buf, (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN, sizeof( cnf_struct_t), portMAX_DELAY) != kStatus_QMC_Ok)
			return kStatus_QMC_Err;

		LCRYPTO_decrypt_aes256_cbc( (uint8_t *)gs_cnf_struct, crypt_buf, sizeof( cnf_struct_t), &cnf_aes_context);
		vPortFree( base_buf);
#else
		if( dispatcher_read_memory( gs_cnf_struct, (uint8_t *)RECORDER_REC_CONFIG_AREABEGIN, sizeof( cnf_struct_t), portMAX_DELAY) != kStatus_QMC_Ok)
			return kStatus_QMC_Err;
#endif
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

	if( dispatcher_init() != kStatus_QMC_Ok)
	{
		dbgCnfPRINTF("Dispatcher_init fail. Configuration.\r\n");
		config_release_lock();
		return kStatus_QMC_Err;
	}

#ifdef FEATURE_CONFIG_USE_CRYPTO
	uint8_t key[AES256_CBC_KEY_SIZE];
	memset( key, 0, sizeof(key));
	LCRYPTO_init_aes256_cbc( cnf_aes_context.iv , key, &cnf_aes_context.ctx);
#endif

	if( CONFIG_ReadFlash() != kStatus_QMC_Ok)
	{
		config_release_lock();
		return kStatus_QMC_Err;
	}

#ifdef FEATURE_CONFIG_USE_HASH
	//pcnf_sha256_context = pvPortMalloc( sizeof( mbedtls_sha256_context));
	//if( pcnf_sha256_context == NULL)
	//{
	//	dbgCnfPRINTF("Cannot alloc %d memory on heap. Configuration\n\r", sizeof(mbedtls_sha256_context));
	//	return kStatus_QMC_Err;
	//}

	uint8_t hash[CONFIGURATION_HASH_SIZE];
	CONFIG_get_sum( hash);
	if( memcmp( gs_cnf_struct->hash, hash, sizeof( hash)) != 0 )
#else
	uint32_t chksum;
	CONFIG_get_sum( (uint8_t*)&chksum);
	if( chksum != gs_cnf_struct->chksum)
#endif
	{
		dbgCnfPRINTF("Read flash chksum fail. Using defaults. Configuration.\r\n");
		memcpy( gs_cnf_struct->cnf_data, &gs_cnf_data_default, sizeof( cnf_data_t) );
	}
	else
	{
		dbgCnfPRINTF("Read flash chksum match. Configuration.\n\r");
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
		if( strncmp( (const char *)gs_cnf_struct->cnf_data[i].key, (const char *)key, CONFIG_MAX_KEY_LEN) == 0)
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
		if( gs_cnf_struct->cnf_data[i].id == id)
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

		strncpy( (char *)value, (const char *)gs_cnf_struct->cnf_data[i].value, sizeof(gs_cnf_struct->cnf_data[i].value));
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

		strncpy( (char *)value, (const char *)gs_cnf_struct->cnf_data[i].value, sizeof(gs_cnf_struct->cnf_data[i].value));
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

		strncpy( (char *)gs_cnf_struct->cnf_data[i].value, (const char *)value, sizeof(gs_cnf_struct->cnf_data[i].value));
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

		strncpy( (char *)gs_cnf_struct->cnf_data[i].value, (const char *)value, sizeof(gs_cnf_struct->cnf_data[i].value));
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

		memcpy( (char *)value, (const char *)gs_cnf_struct->cnf_data[i].value, length);
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

		memcpy( (char *)value, (const char *)gs_cnf_struct->cnf_data[i].value, length);
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

		memcpy( (char *)gs_cnf_struct->cnf_data[i].value, (const char *)value, length);
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

		memcpy( (char *)gs_cnf_struct->cnf_data[i].value, (const char *)value, length);
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
		return gs_cnf_struct->cnf_data[i].id;
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

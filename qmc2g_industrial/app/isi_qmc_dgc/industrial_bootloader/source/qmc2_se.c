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
 * @file    sbl_se.c
 * @brief   SE API.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "fsl_sss_api.h"
#include "fsl_sss_se05x_types.h"

#if SSS_HAVE_APPLET_SE05X_IOT
#include <fsl_sss_se05x_policy.h>
#endif

#include <qmc2_se.h>
#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include <qmc2_fwu.h>
#include <qmc2_puf.h>

#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>
#include <se05x_APDU.h>
#include <se05x_const.h>
#include <se05x_ecc_curves.h>
#include <se05x_ecc_curves_values.h>
#include <se05x_tlv.h>

#include <ex_sss.h>
#include <ex_sss_scp03_keys.h>
#include <ex_scp03_puf.h>
#include <fsl_puf.h>
#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>

#include "error.h"
#include "sha256.h"
#include "sha512.h"
//#include <puf_rotate_scp03_s.h>
#include <nxEnsure.h>

#include "ksdk_mbedtls.h"

#include "ex_sss_auth.h"
#include "smCom.h"


#ifdef __cplusplus
}
#endif

#include <string.h> /* memset */
#include "PlugAndTrust_Pkg_Ver.h"
#include "string.h" /* memset */
#include "fsl_iomuxc.h"

/* Expsosed variables */
#include "ax_reset.h"
#include "board.h"
#include "fsl_gpio.h"
#include "ledHandler.h"
#include "pin_mux.h"
#include "se_reset_config.h"
#include "sm_timer.h"

#include "qmc2_sd_fatfs.h"

static sss_tunnel_t gs_tunnel;

#define AES_KEY_LEN_nBYTE 0x10
#define PUT_KEYS_KEY_TYPE_CODING_AES 0x88
#define CRYPTO_KEY_CHECK_LEN 0x03
#define GP_CLA_BYTE 0x80
#define GP_INS_PUTKEY 0xD8
#define GP_P2_MULTIPLEKEYS 0x81

#define AUTH_KEY_SIZE 16
#define SCP03_MAX_AUTH_KEY_SIZE 52

/* Establish connecton with SE */
static sss_status_t SE_EstablishScp03Conn(ex_sss_boot_ctx_t *pCtx, scp03_keys_t *scp03, uint8_t skip_select_applet);
static sss_status_t SE_EstablishPolicySession(ex_sss_boot_ctx_t *pCtx, ex_sss_boot_ctx_t *pPolicyCtx, uint8_t *aesKey);
/* Sreate Key Data for SCP03 rotation */
static sss_status_t SE_CreateKeyData(uint8_t *key, uint8_t *targetStore, ex_sss_boot_ctx_t *pCtx, uint32_t Id);
/* Generate encrypted Key Value */
static sss_status_t SE_GenKcvAndEncryptKey(uint8_t *encryptedkey, uint8_t *keyCheckVal, uint8_t *plainKey, ex_sss_boot_ctx_t *pCtx, uint32_t keyId);
/* Rotate SCP03 keys */
static sss_status_t SE_RotatePlatformScpKeys(ex_sss_boot_ctx_t *pCtx, uint8_t *enc, uint8_t *mac, uint8_t *dek);
/* Function to Set Init and Allocate static Scp03Keys and Init Allocate dynamic keys */
static sss_status_t SE_PrepareHostScp(sss_session_t *pHostSession, sss_key_store_t *pKs, SE_Connect_Ctx_t *se05x_open_ctx, scp03_keys_t *scp03Key);
/* Alloc SCP03 keys */
static sss_status_t SE_AllocScp03KeyToSE05xAuthCtx(sss_object_t *keyObject, sss_key_store_t *pKs, uint32_t keyId);
/* Write binary data into the SE realated to ID. */
static sss_status_t SE_WriteBinaryData(ex_sss_boot_ctx_t *pCtx, uint32_t object_id, uint8_t *seData, uint32_t length);
/* Read binary data from the SE bind to ID. */
static sss_status_t SE_ReadBinaryData(ex_sss_boot_ctx_t *pCtx, uint32_t object_id, uint8_t *seData, uint32_t length);
/* Mandate SCP03. Once set, encrypted communitcation is requested. */
static sss_status_t SE_MandateScp(ex_sss_boot_ctx_t *pCtx);
/* Mandate SCP03. Once set, encrypted communitcation is requested. */
static sss_status_t SE_UnMandateScp(ex_sss_boot_ctx_t *pCtx);
/* Erase Object */
static sss_status_t SE_EraseObject(ex_sss_boot_ctx_t *pCtx, uint32_t object_id);
/* Read status from SE */
static sss_status_t SE_ReadStatus(ex_sss_boot_ctx_t *pCtx, se_state_t *seState);
static sss_status_t SE_MbedtlsSha256(uint32_t input, size_t ilen, uint8_t output[32],  int is224);
static sss_status_t SE_MbedtlsSha512(uint32_t input, size_t ilen, uint8_t output[64],  int is384);

/* clang-format off */
#define MandateSCP_UserID_VALUE                 \
    {                                           \
        'N', 'E', 'E', 'D', 'S', 'C', 'P'            \
    }
/* clang-format ON */


/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/

static sss_status_t SE_MbedtlsSha512(uint32_t input, size_t ilen, uint8_t output[64],  int is384)
{
	sss_status_t status = kStatus_SSS_Fail;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_sha512_context ctx;
    uint8_t ramInputBuff[RAM_BUFFER_SIZE];
    uint32_t address = 0;
    size_t length = 0;
    size_t rest = 0;

#if !defined(MBEDTLS_SHA512_NO_SHA384)
    if(!(is384 == 0 || is384 == 1))
        return kStatus_SSS_Fail;
#else
    if(!(is384 == 0)
        return kStatus_SSS_Fail;
#endif

    if(ilen == 0)
    	return kStatus_SSS_Fail;

    if((uint8_t *)output == NULL)
    	return kStatus_SSS_Fail;

    mbedtls_sha512_init( &ctx );

    if( ( ret = mbedtls_sha512_starts_ret( &ctx, is384 ) ) != 0 )
        goto exit;

    rest = ilen % RAM_BUFFER_SIZE;
    length = ilen - rest;
    address = input;

    while(length)
    {
    	memcpy(ramInputBuff, (void *)address, RAM_BUFFER_SIZE);

    	if( ( ret = mbedtls_sha512_update_ret( &ctx, ramInputBuff, RAM_BUFFER_SIZE ) ) != 0 )
    		goto exit;

    	length -= RAM_BUFFER_SIZE;
    	address += RAM_BUFFER_SIZE;
    }

    if(rest)
    {
    	memcpy((void *)ramInputBuff, (void *)address, rest);
    	if( ( ret = mbedtls_sha512_update_ret( &ctx, ramInputBuff, rest ) ) != 0 )
    		goto exit;
    }

    if( ( ret = mbedtls_sha512_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

    status = kStatus_SSS_Success;

exit:
    mbedtls_sha512_free( &ctx );

    return status;
}


static sss_status_t SE_MbedtlsSha256(uint32_t input, size_t ilen, uint8_t output[32],  int is224)
{
	sss_status_t status = kStatus_SSS_Fail;
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    mbedtls_sha256_context ctx;
    uint8_t ramInputBuff[RAM_BUFFER_SIZE];
    uint32_t address = 0;
    size_t length = 0;
    size_t rest = 0;

    if(!(is224 == 0 || is224 == 1))
    	return kStatus_SSS_Fail;

    if(ilen == 0)
    	return kStatus_SSS_Fail;

    if((uint8_t *)output == NULL)
    	return kStatus_SSS_Fail;

    mbedtls_sha256_init( &ctx );

    if( ( ret = mbedtls_sha256_starts_ret( &ctx, is224 ) ) != 0 )
        goto exit;

    rest = ilen % RAM_BUFFER_SIZE;
    length = ilen - rest;
    address = input;

    while(length)
    {
    	memcpy(ramInputBuff, (void *)address, RAM_BUFFER_SIZE);

    	if( ( ret = mbedtls_sha256_update_ret( &ctx, ramInputBuff, RAM_BUFFER_SIZE ) ) != 0 )
    		goto exit;

    	length -= RAM_BUFFER_SIZE;
    	address += RAM_BUFFER_SIZE;
    }

    if(rest)
    {
    	memcpy((void *)ramInputBuff, (void *)address, rest);
    	if( ( ret = mbedtls_sha256_update_ret( &ctx, ramInputBuff, rest ) ) != 0 )
    		goto exit;
    }

    if( ( ret = mbedtls_sha256_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

    status = kStatus_SSS_Success;

exit:
    mbedtls_sha256_free( &ctx );

    return status;
}

static sss_status_t SE_ReadStatus(ex_sss_boot_ctx_t *pCtx, se_state_t *seState)
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t sm_status = SM_NOT_OK;
    sss_se05x_session_t *pSession = (sss_se05x_session_t *)&pCtx->session;
    uint8_t state[4] = {0};
    size_t stateLen  = sizeof(state);

    assert(pCtx != NULL);
    assert(seState != NULL);

    sm_status = Se05x_API_ReadState(&pSession->s_ctx, state, &stateLen);
    if (sm_status != SM_OK)
    {
        LOG_E("Se05x_API_ReadState Failed");
        goto exit;
    }

    if (stateLen == 3)
    {
        LOG_I("SE05x Read State Successfully!!!");
        LOG_I("Following is the SE05x Read State status");
        seState->lockState = state[0];
        if (seState->lockState == kSE05x_LockState_LOCKED)
        {
            LOG_I("%s = 0x%0X %s", "SE05x Lock State", seState->lockState, " i.e. SE05x is Locked!!!");
        }
        else
        {
            LOG_I("%s = 0x%0X %s", "SE05x Lock State", seState->lockState, " i.e. SE05x is Unlocked!!!");
        }
        seState->restrictMode = state[1];

        if (seState->restrictMode  == kSE05x_RestrictMode_NA)
        {
            LOG_I("%s = 0x%0X %s",
                "SE05x Restrict Mode",
                seState->restrictMode ,
                " i.e. No Restriction is applied for object creation!!!");
        }
        else if (seState->restrictMode  == kSE05x_RestrictMode_RESTRICT_NEW)
        {
            LOG_I("%s = 0x%0X %s",
                "SE05x Restrict Mode",
                seState->restrictMode ,
                " i.e. Restriction is applied for new object creation!!!");
        }
        else if (seState->restrictMode  == kSE05x_RestrictMode_RESTRICT_ALL)
        {
            LOG_I("%s = 0x%0X %s",
                "SE05x Restrict Mode",
                seState->restrictMode ,
                " i.e. Restriction is applied for all objects !!!");
        }
        else
        {
            goto exit;
        }
        seState->platformSCPRequest  = state[2];
        if ( seState->platformSCPRequest == kSE05x_PlatformSCPRequest_REQUIRED)
        {
            LOG_I("%s = 0x%0X %s",
                "SE05x Platform SCP Request",
                 seState->platformSCPRequest,
                " i.e. Platform SCP is Requested for Communication!!!");
        }
        else if ( seState->platformSCPRequest == kSE05x_PlatformSCPRequest_NOT_REQUIRED)
        {
            LOG_I("%s = 0x%0X %s",
                "SE05x Platform SCP Request",
                 seState->platformSCPRequest,
                " i.e. Platform SCP is not required for Communication!!!");
        }
        else
        {
            goto exit;
        }

        status = kStatus_SSS_Success;
    }

exit:


    return status;
}

static sss_status_t SE_MandateScp(ex_sss_boot_ctx_t *pCtx)
{
	sss_status_t status = kStatus_SSS_Fail;
	smStatus_t sw_status = SM_NOT_OK;
	SE_Connect_Ctx_t eraseAuthCtx = {0};
	sss_se05x_session_t *pSession = (sss_se05x_session_t*) &pCtx->session;
	Se05xSession_t *pSe05xSession = NULL;
	SE_Connect_Ctx_t *pOpenCtx = NULL;
	sss_object_t ex_id = {0};

    const uint8_t host_userid_value[] = MandateSCP_UserID_VALUE;
    const uint8_t userid_value_factoryreset[] = MandateSCP_UserID_VALUE;
	eraseAuthCtx.auth.ctx.idobj.pObj = &ex_id;

	 assert(pCtx != NULL);

	/* Prepare host */
	status = sss_key_object_init(eraseAuthCtx.auth.ctx.idobj.pObj, &pCtx->host_ks);
	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_key_object_init");
		goto cleanup;
	}

	status = sss_key_object_allocate_handle(eraseAuthCtx.auth.ctx.idobj.pObj,
			SE_MANDATE_SCP_ID, kSSS_KeyPart_Default,
			kSSS_CipherType_UserID, sizeof(host_userid_value),
			kKeyObject_Mode_Transient);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_key_object_allocate_handle");
		goto cleanup;
	}

	status = sss_key_store_set_key(&pCtx->host_ks,
			eraseAuthCtx.auth.ctx.idobj.pObj, host_userid_value,
			sizeof(host_userid_value), sizeof(host_userid_value) * 8,
			NULL, 0);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_key_store_set_key");
		goto cleanup;
	}

	pSe05xSession = &pSession->s_ctx;

	sw_status = Se05x_API_WriteUserID(pSe05xSession, NULL,
			SE05x_MaxAttemps_NA, kSE05x_AppletResID_PLATFORM_SCP,
			userid_value_factoryreset, sizeof(userid_value_factoryreset),
			kSE05x_AttestationType_AUTH);

	pOpenCtx = &pCtx->se05x_open_ctx;
	eraseAuthCtx.tunnelCtx = pOpenCtx->tunnelCtx;
	eraseAuthCtx.connType = pOpenCtx->connType;
	eraseAuthCtx.portName = pOpenCtx->portName;
	eraseAuthCtx.auth.authType = kSSS_AuthType_ID;

	sss_session_close(&pCtx->session);

	pSe05xSession = &pSession->s_ctx;

	status = sss_session_open(&pCtx->session, kType_SSS_SE_SE05x,
			kSE05x_AppletResID_PLATFORM_SCP, kSSS_ConnectionType_Password,
			&eraseAuthCtx);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_session_open");
		goto cleanup;
	}

	status = kStatus_SSS_Fail;
	/* Call SE05X API to Mandate Platform SCP. */
	sw_status = Se05x_API_SetPlatformSCPRequest(&pSession->s_ctx, kSE05x_PlatformSCPRequest_REQUIRED);

	if (sw_status != SM_OK)
	{
		LOG_E("Se05x_API_SetPlatformSCPRequest Failed");
		goto cleanup;
	}
	else
	{
		status = kStatus_SSS_Success;
		LOG_I("Se05x_API_SetPlatformSCPRequest Successful");
		LOG_W("Further communication must be encrypted");
	}

cleanup:

	return status;
}

static sss_status_t SE_UnMandateScp(ex_sss_boot_ctx_t *pCtx)
{
	sss_status_t status = kStatus_SSS_Fail;
	smStatus_t sw_status = SM_NOT_OK;
	sss_session_t reEnableSession = {0};
	sss_tunnel_t reEnableTunnel = {0};
	SE_Connect_Ctx_t eraseAuthCtx = {0};
	Se05xSession_t *pSe05xSession = NULL;
	sss_object_t ex_id = {0};

    const uint8_t host_userid_value[] = MandateSCP_UserID_VALUE;
	eraseAuthCtx.auth.ctx.idobj.pObj = &ex_id;

	assert(pCtx != NULL);

	/* Prepare host */
	status = sss_key_object_init(eraseAuthCtx.auth.ctx.idobj.pObj, &pCtx->host_ks);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_key_object_init");
		goto cleanup;
	}

	status = sss_key_object_allocate_handle(eraseAuthCtx.auth.ctx.idobj.pObj,
			SE_MANDATE_SCP_ID, kSSS_KeyPart_Default,
			kSSS_CipherType_UserID, sizeof(host_userid_value),
			kKeyObject_Mode_Transient);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_key_object_allocate_handle");
		goto cleanup;
	}

	status = sss_key_store_set_key(&pCtx->host_ks,
			eraseAuthCtx.auth.ctx.idobj.pObj, host_userid_value,
			sizeof(host_userid_value), sizeof(host_userid_value) * 8,
			NULL, 0);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_key_store_set_key");
		goto cleanup;
	}

	pSe05xSession = &((sss_se05x_session_t*) &reEnableSession)->s_ctx;
	eraseAuthCtx.tunnelCtx = &reEnableTunnel;
	reEnableTunnel.session = &pCtx->session;
	eraseAuthCtx.connType = kType_SE_Conn_Type_Channel; // pOpenCtx->connType;
	eraseAuthCtx.portName = NULL; // pOpenCtx->portName;
	eraseAuthCtx.auth.authType = kSSS_AuthType_ID;

	status = sss_session_open(&reEnableSession, kType_SSS_SE_SE05x,
			kSE05x_AppletResID_PLATFORM_SCP, kSSS_ConnectionType_Password,
			&eraseAuthCtx);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("Failed sss_session_open");
		goto cleanup;
	}

	status = kStatus_SSS_Fail;
	/* Call SE05X API to Mandate Platform SCP. */
	sw_status = Se05x_API_SetPlatformSCPRequest(pSe05xSession, kSE05x_PlatformSCPRequest_NOT_REQUIRED);

	if (SM_OK != sw_status)
	{
		LOG_E("Se05x_API_SetPlatformSCPRequest Failed");
		goto cleanup;
	}
	else
	{
		status = kStatus_SSS_Success;
		LOG_I("Se05x_API_SetPlatformSCPRequest Successful");
		LOG_W("Further communication can be plain");
	}

cleanup:

	return status;
}

static sss_status_t SE_WriteBinaryData(ex_sss_boot_ctx_t *pCtx, uint32_t object_id, uint8_t *seData, uint32_t length)
{
    sss_status_t status = kStatus_SSS_Fail;
    sss_object_t dataObject = {0};

    assert((length > 0) && (length <= SE_MAX_BYTE_WRITE_LENGTH));
    assert(seData != NULL);
    assert(object_id != 0);
    assert(pCtx != 0);

    status = sss_key_object_init(&dataObject, &pCtx->ks);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = sss_key_object_allocate_handle(&dataObject,
        object_id,
        kSSS_KeyPart_Default,
        kSSS_CipherType_Binary,
        length,
        kKeyObject_Mode_Persistent);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    LOG_I("Injecting binary data at 0x%08X", object_id);
    status = sss_key_store_set_key(&pCtx->ks, &dataObject, seData, length, length * 8, NULL, 0);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

    return status;
}

static sss_status_t SE_ReadBinaryData(ex_sss_boot_ctx_t *pCtx, uint32_t object_id, uint8_t *seData, uint32_t length)
{
    sss_status_t status = kStatus_SSS_Fail;
    sss_object_t dataObject = {0};
    size_t len = length;
    size_t lenInBits = length*8;
    uint8_t data[SE_MAX_BYTE_WRITE_LENGTH] = {0};

    assert((length > 0) && (length <= 200));
    assert(seData != NULL);
    assert(object_id != 0);
    assert(pCtx != 0);

    status = sss_key_object_init(&dataObject, &pCtx->ks);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = sss_key_object_get_handle(&dataObject, object_id);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    LOG_I("Reading binary data at 0x%08X", object_id);
    status = sss_key_store_get_key(&pCtx->ks, &dataObject, data, &len, &lenInBits);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    memcpy(seData, &data, len);

cleanup:

    return status;
}

//static sss_status_t SE_EraseObject(ex_sss_boot_ctx_t *pCtx, uint32_t object_id)
sss_status_t SE_EraseObject(ex_sss_boot_ctx_t *pCtx, uint32_t object_id)
{
    sss_status_t status = kStatus_SSS_Fail;
    sss_object_t dataObject = {0};

    assert(object_id != 0);
    assert(pCtx != 0);

    status = sss_key_object_init(&dataObject, &pCtx->ks);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = sss_key_object_get_handle(&dataObject, object_id);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    LOG_I("Erasing binary data at 0x%08X", object_id);
    /* delete the provisioned secure object (binary file) */
    status = sss_key_store_erase_key(&pCtx->ks, &dataObject);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

    return status;
}

static sss_status_t SE_GenKcvAndEncryptKey(
    uint8_t *encryptedkey, uint8_t *keyCheckVal, uint8_t *plainKey, ex_sss_boot_ctx_t *pCtx, uint32_t keyId)
{
	sss_status_t status = kStatus_SSS_Fail;
	sss_algorithm_t algorithm = kAlgorithm_SSS_AES_ECB;
	sss_mode_t mode = kMode_SSS_Encrypt;
	uint8_t keyCheckValLen = AES_KEY_LEN_nBYTE;
	uint8_t encryptedkeyLen = AES_KEY_LEN_nBYTE;
	uint8_t refOneArray[AES_KEY_LEN_nBYTE] = {0};
	sss_symmetric_t symm = {0};
	sss_object_t keyObj = {0};
	uint8_t DekEnckey[256] = {0};
	size_t DekEnckeyLen = sizeof(DekEnckey);
	size_t DekEnckeyBitLen = 1024;
	uint8_t dummyBuffer[AES_KEY_LEN_nBYTE] = {0};

	/* Initialize the key Object */
	status = sss_key_object_init(&keyObj, &pCtx->host_ks);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Allocate the key Object handle */
	status = sss_key_object_allocate_handle(&keyObj, keyId,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, 16,
			kKeyObject_Mode_Transient);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Set the key */
	status = sss_key_store_set_key(&pCtx->host_ks, &keyObj, plainKey,
			AES_KEY_LEN_nBYTE, (AES_KEY_LEN_nBYTE) * 8, NULL, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Init EBC Encrypt Symmetric Algorithm */
	status = sss_symmetric_context_init(&symm, &pCtx->host_session, &keyObj,
			algorithm, mode);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	memset(refOneArray, 1, sizeof(refOneArray));

	/* Generate key check values*/
	status = sss_cipher_one_go(&symm, NULL, 0, refOneArray, keyCheckVal,
			keyCheckValLen);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Encyrpt the sensitive data */
	status = sss_key_store_get_key(&pCtx->host_ks,
			&pCtx->se05x_open_ctx.auth.ctx.scp03.pStatic_ctx->Dek, DekEnckey,
			&DekEnckeyLen, &DekEnckeyBitLen);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Set the key */
	status = sss_key_store_set_key(&pCtx->host_ks, &keyObj, DekEnckey,
			AES_KEY_LEN_nBYTE, (AES_KEY_LEN_nBYTE) * 8, NULL, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Encrypt the key */
	status = sss_cipher_one_go(&symm, NULL, 0, plainKey, dummyBuffer,
			encryptedkeyLen);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
	memcpy(encryptedkey, dummyBuffer, encryptedkeyLen);

	/* Free Symmetric Object */
	if (symm.keyObject) {
		sss_symmetric_context_free(&symm);
	}
	sss_key_object_free(&keyObj);

	cleanup: return status;
}

static sss_status_t SE_CreateKeyData(uint8_t *key, uint8_t *targetStore, ex_sss_boot_ctx_t *pCtx, uint32_t Id)
{
    uint8_t keyCheckValues[AES_KEY_LEN_nBYTE] = {0};
    sss_status_t status                       = kStatus_SSS_Fail;

    /* For Each Key add Key Type Length of Key data and key length*/

    targetStore[0]                     = PUT_KEYS_KEY_TYPE_CODING_AES; //Key Type
    targetStore[1]                     = AES_KEY_LEN_nBYTE + 1;        // Length of the 'AES key data'
    targetStore[2]                     = AES_KEY_LEN_nBYTE;            // Length of 'AES key'
    targetStore[3 + AES_KEY_LEN_nBYTE] = CRYPTO_KEY_CHECK_LEN;         //Lenth of KCV

    /* Encrypt Key and generate key check values */
    status = SE_GenKcvAndEncryptKey(&targetStore[3], keyCheckValues, key, pCtx, Id);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    /* Copy the Key Check values */
    memcpy(&targetStore[3 + AES_KEY_LEN_nBYTE + 1], &keyCheckValues[0], CRYPTO_KEY_CHECK_LEN);

cleanup:
    return status;
}

/* Functions to rotate PlatfSCP03 keys */
static sss_status_t SE_RotatePlatformScpKeys(ex_sss_boot_ctx_t *pCtx, uint8_t *enc, uint8_t *mac, uint8_t *dek)
{
    sss_status_t status = kStatus_SSS_Fail;
    smStatus_t st       = SM_NOT_OK;
    uint8_t keyVersion  = pCtx->se05x_open_ctx.auth.ctx.scp03.pStatic_ctx->keyVerNo;
    tlvHeader_t hdr     = {{GP_CLA_BYTE, GP_INS_PUTKEY, keyVersion, GP_P2_MULTIPLEKEYS}};
    uint8_t response[64] = {0};
    size_t responseLen = sizeof(response);
    uint8_t cmdBuf[128] = {0};
    uint8_t len = 0;
    uint8_t keyChkValues[16] = {0};
    uint8_t keyChkValLen = 0;

	assert(enc != NULL);
	assert(mac != NULL);
	assert(dek != NULL);
	assert(pCtx != NULL);

    /* Prepare the packet for SCP03 keys Provision */
    cmdBuf[len] = keyVersion; //keyVersion to replace
    len += 1;
    keyChkValues[keyChkValLen] = keyVersion;
    keyChkValLen += 1;
    sss_se05x_session_t *pSession = (sss_se05x_session_t *)&pCtx->session;
    LOG_I("Updating key with version - %02x", keyVersion);

    /* Prepare the packet for ENC Key */
    status = SE_CreateKeyData(enc, &cmdBuf[len], pCtx, MAKE_TEST_ID(__LINE__));
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    memcpy(&keyChkValues[keyChkValLen], &cmdBuf[len + 3 + AES_KEY_LEN_nBYTE + 1], CRYPTO_KEY_CHECK_LEN);
    len += (3 + AES_KEY_LEN_nBYTE + 1 + CRYPTO_KEY_CHECK_LEN);
    keyChkValLen += CRYPTO_KEY_CHECK_LEN;

    /* Prepare the packet for MAC Key */
    status = SE_CreateKeyData(mac, &cmdBuf[len], pCtx, MAKE_TEST_ID(__LINE__));
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    memcpy(&keyChkValues[keyChkValLen], &cmdBuf[len + 3 + AES_KEY_LEN_nBYTE + 1], CRYPTO_KEY_CHECK_LEN);
    len += (3 + AES_KEY_LEN_nBYTE + 1 + CRYPTO_KEY_CHECK_LEN);
    keyChkValLen += CRYPTO_KEY_CHECK_LEN;

    /* Prepare the packet for DEK Key */
    status = SE_CreateKeyData(dek, &cmdBuf[len], pCtx, MAKE_TEST_ID(__LINE__));
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
    memcpy(&keyChkValues[keyChkValLen], &cmdBuf[len + 3 + AES_KEY_LEN_nBYTE + 1], CRYPTO_KEY_CHECK_LEN);
    len += (3 + AES_KEY_LEN_nBYTE + 1 + CRYPTO_KEY_CHECK_LEN);
    keyChkValLen += CRYPTO_KEY_CHECK_LEN;

    /* Reset status to fail */
    status = kStatus_SSS_Fail;
    st     = DoAPDUTxRx_s_Case4(&pSession->s_ctx, &hdr, cmdBuf, len, response, &responseLen);
    ENSURE_OR_GO_CLEANUP(st == SM_OK);
    // reconstruct Return Value
    st = (response[responseLen - 2] << 8) + response[responseLen - 1];
    ENSURE_OR_GO_CLEANUP(st == SM_OK);
    status = kStatus_SSS_Success;

cleanup:


    return status;

}

/* Function to Set Init and Allocate static Scp03Keys and Init Allocate dynamic keys */
static sss_status_t SE_PrepareHostScp(sss_session_t *pHostSession, sss_key_store_t *pKs, SE_Connect_Ctx_t *se05x_open_ctx, scp03_keys_t *scp03Key)
{
    sss_status_t status 			 = kStatus_SSS_Fail;
    NXSCP03_StaticCtx_t *pStatic_ctx = se05x_open_ctx->auth.ctx.scp03.pStatic_ctx;
    NXSCP03_DynCtx_t *pDyn_ctx       = se05x_open_ctx->auth.ctx.scp03.pDyn_ctx;
    pStatic_ctx->keyVerNo 		     = EX_SSS_AUTH_SE05X_KEY_VERSION_NO;
    uint8_t keyLength			     = sizeof(scp03Key->enc_key);

    /* Init Allocate ENC Static Key */
    status = SE_AllocScp03KeyToSE05xAuthCtx(&pStatic_ctx->Enc, pKs, MAKE_TEST_ID(__LINE__));
    if (status != kStatus_SSS_Success) {
        return status;
    }
    /* Set ENC Static Key */
    status = sss_host_key_store_set_key(pKs, &pStatic_ctx->Enc, scp03Key->enc_key, keyLength, keyLength * 8, NULL, 0);
    if (status != kStatus_SSS_Success) {
        return status;
    }

    /* Init Allocate MAC Static Key */
    status = SE_AllocScp03KeyToSE05xAuthCtx(&pStatic_ctx->Mac, pKs, MAKE_TEST_ID(__LINE__));
    if (status != kStatus_SSS_Success) {
        return status;
    }
    /* Set MAC Static Key */
    status = sss_host_key_store_set_key(pKs, &pStatic_ctx->Mac, scp03Key->mac_key, keyLength, keyLength * 8, NULL, 0);
    if (status != kStatus_SSS_Success) {
        return status;
    }

    /* Init Allocate DEK Static Key */
    status = SE_AllocScp03KeyToSE05xAuthCtx(&pStatic_ctx->Dek, pKs, MAKE_TEST_ID(__LINE__));
    if (status != kStatus_SSS_Success) {
        return status;
    }
    /* Set DEK Static Key */
    status = sss_host_key_store_set_key(pKs, &pStatic_ctx->Dek,  scp03Key->dek_key, keyLength, keyLength * 8, NULL, 0);
    if (status != kStatus_SSS_Success) {
        return status;
    }

    /* Init Allocate ENC Session Key */
    status = SE_AllocScp03KeyToSE05xAuthCtx(&pDyn_ctx->Enc, pKs, MAKE_TEST_ID(__LINE__));
    if (status != kStatus_SSS_Success) {
        return status;
    }
    /* Init Allocate MAC Session Key */
    status = SE_AllocScp03KeyToSE05xAuthCtx(&pDyn_ctx->Mac, pKs, MAKE_TEST_ID(__LINE__));
    if (status != kStatus_SSS_Success) {
        return status;
    }
    /* Init Allocate RMAC Session Key */
    status = SE_AllocScp03KeyToSE05xAuthCtx(&pDyn_ctx->Rmac, pKs, MAKE_TEST_ID(__LINE__));
    return status;
}

static sss_status_t SE_AllocScp03KeyToSE05xAuthCtx(sss_object_t *keyObject, sss_key_store_t *pKs, uint32_t keyId)
{
    sss_status_t status = kStatus_SSS_Fail;
    status              = sss_host_key_object_init(keyObject, pKs);
    if (status != kStatus_SSS_Success) {
        return status;
    }

    status = sss_host_key_object_allocate_handle(keyObject,
        keyId,
        kSSS_KeyPart_Default,
        kSSS_CipherType_AES,
        SCP03_MAX_AUTH_KEY_SIZE,
        kKeyObject_Mode_Transient);
    return status;
}

static sss_status_t SE_EstablishScp03Conn(ex_sss_boot_ctx_t *pCtx, scp03_keys_t *scp03, uint8_t skip_select_applet)
 {
	sss_status_t status = kStatus_SSS_Fail;
	SE05x_Connect_Ctx_t *pConnectCtx = NULL;

	assert(scp03 != NULL);
    assert(pCtx != NULL);

    pConnectCtx = &pCtx->se05x_open_ctx;
	pCtx->se05x_open_ctx.skip_select_applet = skip_select_applet;
    // Auth type is Platform SCP03
	pConnectCtx->auth.authType               = kSSS_AuthType_SCP03;
	pConnectCtx->connType	                 = kSSS_ConnectionType_Encrypted;
	pConnectCtx->portName 					 = NULL;
	pConnectCtx->auth.ctx.scp03.pStatic_ctx  = &pCtx->ex_se05x_auth.scp03.ex_static;
	pConnectCtx->auth.ctx.scp03.pDyn_ctx     = &pCtx->ex_se05x_auth.scp03.ex_dyn;

    /* SE */
    axReset_HostConfigure();
    axReset_PowerUp();

    if (sm_initSleep())
    {
        LOG_E("Timer initialization failed");\
        goto cleanup;
    }

	status = sss_host_session_open(&pCtx->host_session, kType_SSS_mbedTLS, 0, kSSS_ConnectionType_Plain, NULL);
	if(status != kStatus_SSS_Success) {
		LOG_E("Failed to open Host Session");
		goto cleanup;
	}

	status = sss_host_key_store_context_init(&pCtx->host_ks, &pCtx->host_session);
	if(status != kStatus_SSS_Success) {
		LOG_E("Host: sss_key_store_context_init failed");
		goto cleanup;
	}

	status = sss_host_key_store_allocate(&pCtx->host_ks, __LINE__);
	if(status != kStatus_SSS_Success) {
		LOG_E("Host: sss_key_store_allocate failed");
		goto cleanup;
	}

	status = SE_PrepareHostScp(&pCtx->host_session, &pCtx->host_ks, &pCtx->se05x_open_ctx, scp03);
	if(status != kStatus_SSS_Success) {
		LOG_E("ex_sss_session_open Failed");
		goto cleanup;
	}

	status = sss_session_open(&pCtx->session, kType_SSS_SE_SE05x, 0, pCtx->se05x_open_ctx.connType, &pCtx->se05x_open_ctx);
    if (status != kStatus_SSS_Success) {
       LOG_E("sss_session_open failed");
       goto cleanup;
    }

	status = ex_sss_key_store_and_object_init(pCtx);
	if(status != kStatus_SSS_Success) {
		LOG_E("x_sss_key_store_and_object_init( Failed");
		goto cleanup;
	}

	return status;

cleanup:

	nLog_DeInit();
	ex_sss_session_close(pCtx);

	LOG_E("!ERROR!");
	return status;
}

static sss_status_t SE_EstablishPolicySession(ex_sss_boot_ctx_t *pCtx, ex_sss_boot_ctx_t *pPolicyCtx, uint8_t *aesKey)
{
	sss_status_t status = kStatus_SSS_Fail;
	SE05x_Connect_Ctx_t *pConnectCtx = NULL;
	NXSCP03_StaticCtx_t *user_static;
	NXSCP03_DynCtx_t *user_dyn;

	assert(pCtx != NULL);
	assert(pPolicyCtx != NULL);
	assert(aesKey != NULL);

	pConnectCtx = &pPolicyCtx->se05x_open_ctx;
	pPolicyCtx->se05x_open_ctx.skip_select_applet = 0;
	pConnectCtx->auth.authType = kSSS_AuthType_AESKey;
	pConnectCtx->connType = kType_SE_Conn_Type_Channel;
	pConnectCtx->portName = NULL;
	pConnectCtx->refresh_session = 0;
	pConnectCtx->auth.ctx.scp03.pStatic_ctx = &pPolicyCtx->ex_se05x_auth.scp03.ex_static;
	pConnectCtx->auth.ctx.scp03.pDyn_ctx = &pPolicyCtx->ex_se05x_auth.scp03.ex_dyn;

	user_static = &pPolicyCtx->ex_se05x_auth.scp03.ex_static;
	user_dyn = &pPolicyCtx->ex_se05x_auth.scp03.ex_dyn;

	memset(&gs_tunnel, 0, sizeof(gs_tunnel));

	/* set user AES key */
	status = sss_key_object_init(&user_static->Enc, &pCtx->host_ks);
	status |= sss_key_object_allocate_handle(&user_static->Enc, __LINE__,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, PUF_AES_POLICY_KEY_SIZE,
			kKeyObject_Mode_Transient);
	status |= sss_key_store_set_key(&pCtx->host_ks, &user_static->Enc,
			aesKey, PUF_AES_POLICY_KEY_SIZE,
			PUF_AES_POLICY_KEY_SIZE * 8, NULL, 0);
	status |= sss_key_object_init(&user_static->Mac, &pCtx->host_ks);
	status |= sss_key_object_allocate_handle(&user_static->Mac, __LINE__,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, PUF_AES_POLICY_KEY_SIZE,
			kKeyObject_Mode_Transient);
	status |= sss_key_store_set_key(&pCtx->host_ks, &user_static->Mac,
			aesKey, PUF_AES_POLICY_KEY_SIZE,
			PUF_AES_POLICY_KEY_SIZE * 8, NULL, 0);
	status |= sss_key_object_init(&user_static->Dek, &pCtx->host_ks);
	status |= sss_key_object_allocate_handle(&user_static->Dek, __LINE__,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, PUF_AES_POLICY_KEY_SIZE,
			kKeyObject_Mode_Transient);
	status |= sss_key_store_set_key(&pCtx->host_ks, &user_static->Dek,
			aesKey, PUF_AES_POLICY_KEY_SIZE,
			PUF_AES_POLICY_KEY_SIZE * 8, NULL, 0);

	status |= sss_key_object_init(&user_dyn->Enc, &pCtx->host_ks);
	status |= sss_key_object_allocate_handle(&user_dyn->Enc, __LINE__,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, PUF_AES_POLICY_KEY_SIZE,
			kKeyObject_Mode_Transient);
	status |= sss_key_object_init(&user_dyn->Mac, &pCtx->host_ks);
	status |= sss_key_object_allocate_handle(&user_dyn->Mac, __LINE__,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, PUF_AES_POLICY_KEY_SIZE,
			kKeyObject_Mode_Transient);
	status |= sss_key_object_init(&user_dyn->Rmac, &pCtx->host_ks);
	status |= sss_key_object_allocate_handle(&user_dyn->Rmac, __LINE__,
			kSSS_KeyPart_Default, kSSS_CipherType_AES, PUF_AES_POLICY_KEY_SIZE,
			kKeyObject_Mode_Transient);

	if (status != kStatus_SSS_Success)
	{
		LOG_E("x_sss_key_store_and_object_init( Failed");
		goto cleanup;
	}



	/* policies */
	/* init tunnel context */
	status = sss_tunnel_context_init(&gs_tunnel, &pCtx->session); //pPolicyCtx->se05x_open_ctx.tunnelCtx
	if (status != kStatus_SSS_Success)
	{
		LOG_E("x_sss_key_store_and_object_init( Failed");
		goto cleanup;
	}

	pPolicyCtx->se05x_open_ctx.tunnelCtx =  &gs_tunnel;

	/* init SSS API session (SE050 connection) */
	status = sss_session_open(&pPolicyCtx->session, kType_SSS_SE_SE05x, SE_AES_AUTH_OBJ_SBL_ID,	kSSS_ConnectionType_Encrypted, &pPolicyCtx->se05x_open_ctx);
	if (status != kStatus_SSS_Success)
	{
		LOG_E("x_sss_key_store_and_object_init( Failed");
		goto cleanup;
	}

	/* init SSS API keystore (SE050 keystore) */
	status = ex_sss_key_store_and_object_init(pPolicyCtx);
	if (status != kStatus_SSS_Success)
	{
		LOG_E("x_sss_key_store_and_object_init( Failed");
		goto cleanup;
	}

	return status;

	cleanup:

	ex_sss_session_close(pPolicyCtx);

	LOG_E("!ERROR!");
	return status;
}

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/
sss_status_t QMC2_SE_CheckScp03Conn(scp03_keys_t *scp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;

	se_state_t seState = {0};

	assert(scp03 != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
	memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_ReadStatus(&sss_policy_ctx, &seState);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	if(seState.platformSCPRequest != kSE05x_PlatformSCPRequest_REQUIRED)
	{
#ifdef RELEASE_SBL
		status = SE_MandateScp(&sss_boot_ctx);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
#endif
	}

cleanup:

	ex_sss_session_close(&sss_policy_ctx);
	ex_sss_session_close(&sss_boot_ctx);

	return status;
}


sss_status_t QMC2_SE_ReadVersionsFromSe(se_data_t *seData, scp03_keys_t *scp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;

	assert(scp03 != NULL);
	assert(seData != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
	memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_ReadBinaryData(&sss_policy_ctx, SE_FW_VERSION_ID, (uint8_t *)&seData->fwVersion, sizeof(uint32_t));
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_ReadBinaryData(&sss_policy_ctx, SE_MAN_VERSION_ID, (uint8_t *)&seData->manVersion, sizeof(uint32_t));
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close((&sss_policy_ctx));
	ex_sss_session_close((&sss_boot_ctx));

	return status;
}

/* Mandate new SCP03 KeySet */
sss_status_t QMC2_SE_MandateScp03Keys(scp03_keys_t *scp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;

	assert(scp03 != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_MandateScp(&sss_boot_ctx);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_boot_ctx);

	return status;
}

/* Un-Mandate new SCP03 KeySet */
sss_status_t QMC2_SE_UnMandateScp03Keys(scp03_keys_t *scp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;

	assert(scp03 != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_UnMandateScp(&sss_boot_ctx);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_boot_ctx);

	return status;
}

/* Rotate new SCP03 KeySet */
sss_status_t QMC2_SE_RotateSCP03Keys(scp03_keys_t *activeScp03, scp03_keys_t *newScp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;

	assert(newScp03 != NULL);
	assert(activeScp03 != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, activeScp03, 1);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_RotatePlatformScpKeys(&sss_boot_ctx, newScp03->enc_key, newScp03->mac_key, newScp03->dek_key);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_boot_ctx);

	return status;
}


sss_status_t QMC2_SE_EraseObj(scp03_keys_t *scp03, uint32_t objectId)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;

	assert(scp03 != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* delete the provisioned secure object (binary file) */
	status = SE_EraseObject(&sss_boot_ctx, objectId);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_boot_ctx);

	if (status == kStatus_SSS_Success)
	{
		LOG_I("Device is commissioned !");

	} else
	{
		LOG_E("Device is not commissioned !");
	}

	return status;
}

sss_status_t QMC2_SE_ReadPolicyAesKey(scp03_keys_t *scp03, uint8_t *aesPolicyKey)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;

	assert(scp03 != NULL);
	assert(aesPolicyKey != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_ReadBinaryData(&sss_boot_ctx, SE_AES_POLICY_KEY_SBL_ID, aesPolicyKey, PUF_AES_POLICY_KEY_SIZE);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_boot_ctx);

	if (status == kStatus_SSS_Success)
	{
		LOG_I("Device is commissioned !");

	} else
	{
		LOG_E("Device is not commissioned !");
	}

	return status;
}

sss_status_t QMC2_SE_CommitManVersionToSe(scp03_keys_t *scp03, manifest_t *man, uint32_t *manVersionInSe)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;

	assert(scp03 != NULL);
	assert(man != NULL);
	assert(manVersionInSe != NULL);

	if(man->version > *manVersionInSe)
	{
		memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
		memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

		status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

		status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

		status = SE_WriteBinaryData(&sss_policy_ctx, SE_MAN_VERSION_ID, (uint8_t *)&man->version, sizeof(uint32_t));
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

		status = SE_ReadBinaryData(&sss_policy_ctx, SE_MAN_VERSION_ID, (uint8_t *)manVersionInSe, sizeof(uint32_t));
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
	}

cleanup:

	ex_sss_session_close(&sss_policy_ctx);
	ex_sss_session_close(&sss_boot_ctx);

	return status;
}

sss_status_t QMC2_SE_CommitFwVersionToSe(scp03_keys_t *scp03, header_t *hdr, uint32_t *fwVersionInSe)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;

	assert(scp03 != NULL);
	assert(hdr != NULL);
	assert(fwVersionInSe != NULL);

	if(hdr->version >= *fwVersionInSe)
	{
		memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
		memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

		status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

		status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

		status = SE_WriteBinaryData(&sss_policy_ctx, SE_FW_VERSION_ID, (uint8_t *)&hdr->version, sizeof(uint32_t));
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

		status = SE_ReadBinaryData(&sss_policy_ctx, SE_FW_VERSION_ID, (uint8_t *)fwVersionInSe, sizeof(uint32_t));
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
	}

cleanup:

	ex_sss_session_close(&sss_policy_ctx);
	ex_sss_session_close(&sss_boot_ctx);

	return status;
}

sss_status_t QMC2_SE_VerifySignature(scp03_keys_t *scp03, se_verify_sign_t *seAuth)
{
	sss_status_t status 	= kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;
	uint8_t digest[64]  	= {0};
	size_t digestLen;
	sss_object_t key_pub;
	sss_asymmetric_t ctx_verify = {0};

	assert(scp03 != NULL);
	assert(seAuth != NULL);

	if (seAuth->algorithm == kAlgorithm_SSS_SHA512)
	{
		status = SE_MbedtlsSha512(seAuth->fwDataAddr, (size_t) seAuth->fwDataLength, digest, 0);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
		digestLen = 64;

	} else
	{
		status = SE_MbedtlsSha256(seAuth->fwDataAddr, (size_t) seAuth->fwDataLength, digest, 0);
		ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
		digestLen = 32;
	}

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
	memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* Pre-requiste for Verifying Part */
	status = sss_key_object_init(&key_pub, &sss_policy_ctx.ks);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);
	/* Get Handle based on ID */
	status = sss_key_object_get_handle(&key_pub, seAuth->keyId);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	/* doc:start ex_sss_asymmetric-asym-verify */
	status = sss_asymmetric_context_init(&ctx_verify, &sss_policy_ctx.session, &key_pub, seAuth->algorithm, seAuth->mode);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	LOG_I("Do Verify");
	LOG_MAU8_I("digest", digest, digestLen);
	LOG_MAU8_I("signature", (uint8_t *)seAuth->signDataAddr, (size_t)seAuth->signDataLength);

	status = sss_asymmetric_verify_digest(&ctx_verify, digest, digestLen,
			(uint8_t *)seAuth->signDataAddr, (size_t)seAuth->signDataLength);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	LOG_I("Verification Successful !!!");
	/* doc:end ex_sss_asymmetric-asym-verify */

cleanup:

	if (ctx_verify.session != NULL)
	{
		sss_asymmetric_context_free(&ctx_verify);
	}

	ex_sss_session_close(&sss_policy_ctx);
	ex_sss_session_close(&sss_boot_ctx);

	return status;
}


sss_status_t QMC2_SE_FactoryReset(scp03_keys_t *scp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;

	assert(scp03 != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
	memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = ex_sss_boot_factory_reset(&sss_policy_ctx);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_policy_ctx);
	ex_sss_session_close(&sss_boot_ctx);

	return status;
}


sss_status_t QMC2_SE_GetRpcSeedAndKey(scp03_keys_t *scp03, uint8_t *data, size_t dataLength, uint8_t *key, size_t keyLength)
{
	sss_status_t status = kStatus_SSS_Fail;
	ex_sss_boot_ctx_t sss_boot_ctx;
	ex_sss_boot_ctx_t sss_policy_ctx;
	sss_rng_context_t rng_ctx;

	assert(scp03 != NULL);
	assert(data != NULL);
	assert(key != NULL);

	memset(&sss_boot_ctx, 0, sizeof(ex_sss_boot_ctx_t));
	memset(&sss_policy_ctx, 0, sizeof(ex_sss_boot_ctx_t));

	status = SE_EstablishScp03Conn(&sss_boot_ctx, scp03, 0);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_EstablishPolicySession(&sss_boot_ctx, &sss_policy_ctx, scp03->aes_policy_key);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = SE_ReadBinaryData(&sss_policy_ctx, SE_RPC_KEY_ID, key, keyLength);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = sss_rng_context_init(&rng_ctx, &sss_policy_ctx.session);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

	status = sss_rng_get_random(&rng_ctx, data, dataLength);
	ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

    status = sss_rng_context_free(&rng_ctx);
    ENSURE_OR_GO_CLEANUP(status == kStatus_SSS_Success);

cleanup:

	ex_sss_session_close(&sss_policy_ctx);
	ex_sss_session_close(&sss_boot_ctx);

	return status;
}

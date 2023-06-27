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
#include <lcrypto.h>
#include <se_session.h>
#include <fsl_sss_api.h>
#include <ex_sss_boot.h>



/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile log_keys_t g_sbl_prov_keys __attribute__((section(".noinit_RAM5_FIX0")));

static StaticSemaphore_t gs_CAAM_Mutex;
SemaphoreHandle_t g_CAAM_xSemaphore = NULL;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Init function for LCRYPTO layer used by datalogger and configuration service
 */
qmc_status_t LCRYPTO_init()
{
	if( g_CAAM_xSemaphore == NULL)
	{
		g_CAAM_xSemaphore = xSemaphoreCreateMutexStatic( &gs_CAAM_Mutex);
		if( g_CAAM_xSemaphore == NULL)
		{
			dbgLcryptPRINTF("Mutex init fail. LCRYPTO.\r\n");
			return kStatus_QMC_Err;
		}
		dbgLcryptPRINTF("Mutex init ok. LCRYPTO.\r\n");
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_Err;
}

/*!
 * @brief Get SHA256 using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_get_sha256( uint8_t *dst, const uint8_t *src, size_t size, mbedtls_sha256_context *pctx, TickType_t ticks)
{
	if( g_CAAM_xSemaphore != NULL)
	if( xSemaphoreTake( g_CAAM_xSemaphore, ticks ) == pdTRUE )
	{
		mbedtls_sha256_init( pctx);
		mbedtls_sha256_starts( pctx, 0); /* SHA-256, not 224 */
		mbedtls_sha256_update( pctx, src, size);
		mbedtls_sha256_finish( pctx, dst);
		mbedtls_sha256_free( pctx);

		if( xSemaphoreGive( g_CAAM_xSemaphore ) != pdTRUE )
		{
			return kStatus_QMC_Err;
		}
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrBusy;
}

/*!
 * @brief Init AES256 context using with mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_init_aes256( lcrypto_aes_ctx_t *pctx, TickType_t ticks)
{
	if( g_CAAM_xSemaphore != NULL)
	if( xSemaphoreTake( g_CAAM_xSemaphore, ticks ) == pdTRUE )
	{
		mbedtls_aes_init( &(pctx->ctx));

		if( xSemaphoreGive( g_CAAM_xSemaphore ) != pdTRUE )
		{
			return kStatus_QMC_Err;
		}
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrBusy;
}

/*!
 * @brief Encrypt data by AES256_CBC cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_encrypt_aes256_cbc( uint8_t *dst, const uint8_t *src, size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks)
{
	if( g_CAAM_xSemaphore != NULL)
	if( xSemaphoreTake( g_CAAM_xSemaphore, ticks ) == pdTRUE )
	{
		mbedtls_aes_init( &pctx->ctx );
		mbedtls_aes_setkey_enc( &pctx->ctx, pctx->key, 256);

		if( mbedtls_aes_crypt_cbc( &pctx->ctx, MBEDTLS_AES_ENCRYPT, size, pctx->iv, src, dst) != 0)
		{
			dbgLcryptPRINTF("Encrypt AES CBC failed. LCRYPTO.\r\n");
			mbedtls_aes_free( &pctx->ctx );
			xSemaphoreGive( g_CAAM_xSemaphore );
			return kStatus_QMC_Err;
		}
		mbedtls_aes_free( &pctx->ctx );

		if( xSemaphoreGive( g_CAAM_xSemaphore ) != pdTRUE )
		{
			return kStatus_QMC_Err;
		}
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrBusy;
}

/*!
 * @brief Decrypt data by AES256_CBC cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_decrypt_aes256_cbc( uint8_t *dst, const uint8_t *src, size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks)
{
	if( g_CAAM_xSemaphore != NULL)
	if( xSemaphoreTake( g_CAAM_xSemaphore, ticks ) == pdTRUE )
	{
		mbedtls_aes_init( &pctx->ctx );
		mbedtls_aes_setkey_enc( &pctx->ctx, pctx->key, 256);

		if( mbedtls_aes_crypt_cbc( &pctx->ctx, MBEDTLS_AES_DECRYPT, size, pctx->iv, src, dst) != 0)
		{
			dbgLcryptPRINTF("Decrypt AES CBC failed. LCRYPTO.\r\n");
			mbedtls_aes_free( &pctx->ctx );
			xSemaphoreGive( g_CAAM_xSemaphore );
			return kStatus_QMC_Err;
		}
		mbedtls_aes_free( &pctx->ctx );

		if( xSemaphoreGive( g_CAAM_xSemaphore ) != pdTRUE )
		{
			return kStatus_QMC_Err;
		}
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrBusy;
}

/*!
 * @brief Encrypt data by AES256_CTR cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_crypt_aes256_ctr( uint8_t *dst, const uint8_t *src, size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks)
{
	if( g_CAAM_xSemaphore != NULL)
	if( xSemaphoreTake( g_CAAM_xSemaphore, ticks ) == pdTRUE )
	{
		mbedtls_aes_init( &pctx->ctx );
		mbedtls_aes_setkey_enc( &pctx->ctx, pctx->key, 256);

		size_t nc_off = 0;
		if( mbedtls_aes_crypt_ctr( &pctx->ctx, size, &nc_off, pctx->iv, NULL, src, dst) != 0)
		{
			dbgLcryptPRINTF("Encrypt AES CTR failed. LCRYPTO.\r\n");
			mbedtls_aes_free( &pctx->ctx );
			xSemaphoreGive( g_CAAM_xSemaphore );
			return kStatus_QMC_Err;
		}
		mbedtls_aes_free( &pctx->ctx );

		if( xSemaphoreGive( g_CAAM_xSemaphore ) != pdTRUE )
		{
			return kStatus_QMC_Err;
		}
		return kStatus_QMC_Ok;
	}
	return kStatus_QMC_ErrBusy;
}

/*!
 * @brief Encrypt data by RSA cipher using mbedtls (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_crypt_RSA( uint8_t *pdst, size_t *pdst_len, const uint8_t *psrc, const size_t src_len)
{
	sss_status_t status	= kStatus_SSS_Fail;
	sss_object_t key_pub;
	sss_asymmetric_t ctx_encrypt;

	if( !SE_IsInitialized())
		return kStatus_QMC_Err;

	//The output data require space size of LCRYPTO_EX_RSA_KEY_SIZE
	//The input data must be less then LCRYPTO_EX_RSA_KEY_SIZE
	if(( *pdst_len < LCRYPTO_EX_RSA_KEY_SIZE)||( src_len >= LCRYPTO_EX_RSA_KEY_SIZE))
		return kStatus_QMC_ErrArgInvalid;

	if(( pdst == NULL)||( psrc == NULL))
		return kStatus_QMC_ErrArgInvalid;

	memset( &ctx_encrypt, 0, sizeof(sss_asymmetric_t));
	memset( &key_pub, 0, sizeof(sss_object_t));

	status = sss_key_object_init( &key_pub, SE_GetKeystore());
	if(status != kStatus_SSS_Success)
		return kStatus_QMC_Err;

	/* Get Handle based on ID */
	status = sss_key_object_get_handle( &key_pub, idLogReaderIdPubKey);
	if(status != kStatus_SSS_Success)
	{
		sss_key_object_free( &key_pub);
		return kStatus_QMC_Err;
	}

	status = sss_asymmetric_context_init( &ctx_encrypt, SE_GetSession(), &key_pub, kAlgorithm_SSS_RSAES_PKCS1_OAEP_SHA1, kMode_SSS_Encrypt);
	if(status != kStatus_SSS_Success)
	{
		sss_key_object_free( &key_pub);
		return kStatus_QMC_Err;
	}

	status = sss_asymmetric_encrypt( &ctx_encrypt, psrc, src_len, pdst, pdst_len);
	if(status != kStatus_SSS_Success)
	{
		sss_key_object_free( &key_pub);
		sss_asymmetric_context_free( &ctx_encrypt);
		return kStatus_QMC_Err;
	}
	sss_key_object_free( &key_pub);
	sss_asymmetric_context_free( &ctx_encrypt);

	return kStatus_QMC_Ok;
}

/*!
 * @brief Sign data by ECC cipher using mbedtls (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_sign_ECC( uint8_t *pdst, size_t *pdst_len, const uint8_t *psrc, const size_t src_len)
{
	sss_status_t status	= kStatus_SSS_Fail;
	sss_object_t key_pair;
	sss_asymmetric_t ctx_sign;

	if( !SE_IsInitialized())
		return kStatus_QMC_Err;

	//The output data require space size of LCRYPTO_EX_RSA_KEY_SIZE
	//The input data must be less then LCRYPTO_EX_RSA_KEY_SIZE
	if(( *pdst_len < LCRYPTO_EX_SIGN_SIZE)||( src_len >= LCRYPTO_EX_RSA_KEY_SIZE))
		return kStatus_QMC_ErrArgInvalid;

	if(( pdst == NULL)||( psrc == NULL))
		return kStatus_QMC_ErrArgInvalid;

	memset( &ctx_sign, 0, sizeof(sss_asymmetric_t));
	memset( &key_pair, 0, sizeof(sss_object_t));

	status = sss_key_object_init( &key_pair, SE_GetKeystore());
	if(status != kStatus_SSS_Success)
		return kStatus_QMC_Err;

	/* Get Handle based on ID */
	status = sss_key_object_get_handle( &key_pair, idDevIdKeyPair);
	if(status != kStatus_SSS_Success)
	{
		sss_key_object_free( &key_pair);
		return kStatus_QMC_Err;
	}

	status = sss_asymmetric_context_init( &ctx_sign, SE_GetSession(), &key_pair, kAlgorithm_SSS_ECDSA_SHA384, kMode_SSS_Sign);
	if(status != kStatus_SSS_Success)
	{
		sss_key_object_free( &key_pair);
		return kStatus_QMC_Err;
	}

	status = sss_asymmetric_sign_digest( &ctx_sign, psrc, src_len, pdst, pdst_len);
	if(status != kStatus_SSS_Success)
	{
		sss_key_object_free( &key_pair);
		sss_asymmetric_context_free( &ctx_sign);
		return kStatus_QMC_Err;
	}
	sss_key_object_free( &key_pair);
	sss_asymmetric_context_free( &ctx_sign);

	return kStatus_QMC_Ok;
}

/*!
 * @brief Get random data using (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_get_RND( uint8_t *pdst, size_t dst_len)
{
	sss_status_t status	= kStatus_SSS_Fail;
	sss_rng_context_t ctx_rnd;

	if( !SE_IsInitialized())
		return kStatus_QMC_Err;

	if(( pdst == NULL)||( dst_len < 4))
		return kStatus_QMC_ErrArgInvalid;

	memset( &ctx_rnd, 0, sizeof( sss_rng_context_t));

	status = sss_rng_context_init( &ctx_rnd, SE_GetSession());
	if(status != kStatus_SSS_Success)
	{
		return kStatus_QMC_Err;
	}

	status = sss_rng_get_random( &ctx_rnd, pdst, dst_len);
	if(status != kStatus_SSS_Success)
	{
		sss_rng_context_free( &ctx_rnd);
		return kStatus_QMC_Err;
	}
	sss_rng_context_free( &ctx_rnd);

	return kStatus_QMC_Ok;
}


/*!
 * @brief Get SHA384 using mbedtls (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_get_sha384( uint8_t *pdst, size_t *dst_len, const uint8_t *psrc, const size_t src_len)
{
	sss_status_t status	= kStatus_SSS_Fail;
	sss_digest_t ctx_digest;

	if( !SE_IsInitialized())
		return kStatus_QMC_Err;

	if(( pdst == NULL)||( psrc == NULL))
		return kStatus_QMC_ErrArgInvalid;

	memset( &ctx_digest, 0, sizeof( sss_digest_t));

	status = sss_digest_context_init( &ctx_digest, SE_GetHostSession(), kAlgorithm_SSS_SHA384, kMode_SSS_Digest);
	if (status != kStatus_SSS_Success) {
		return kStatus_QMC_Err;
	}

	status = sss_digest_init( &ctx_digest);
	if (status != kStatus_SSS_Success) {
		sss_digest_context_free( &ctx_digest);
		return kStatus_QMC_Err;
	}

	status = sss_digest_update( &ctx_digest, psrc, src_len);
	if (status != kStatus_SSS_Success) {
		sss_digest_context_free( &ctx_digest);
		return kStatus_QMC_Err;
	}

	status = sss_digest_finish( &ctx_digest, pdst, dst_len);
	if (status != kStatus_SSS_Success) {
		sss_digest_context_free( &ctx_digest);
		return kStatus_QMC_Err;
	}

	sss_digest_context_free( &ctx_digest);
	return kStatus_QMC_Ok;
}

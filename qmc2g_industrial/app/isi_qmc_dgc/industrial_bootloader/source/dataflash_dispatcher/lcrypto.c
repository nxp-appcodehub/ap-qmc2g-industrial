/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "app.h"
#include <lcrypto.h>
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
log_keys_t g_sbl_prov_keys;


/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Init function for LCRYPTO layer used by datalogger and configuration service
 */
qmc_status_t LCRYPTO_init()
{
	return kStatus_QMC_Ok;
}

/*!
 * @brief Get SHA256 using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_get_sha256(uint8_t *dst, const uint8_t *src, size_t size,
		mbedtls_sha256_context *pctx, TickType_t ticks) {

	mbedtls_sha256_init(pctx);
	mbedtls_sha256_starts(pctx, 0); /* SHA-256, not 224 */
	mbedtls_sha256_update(pctx, src, size);
	mbedtls_sha256_finish(pctx, dst);
	mbedtls_sha256_free(pctx);

	return kStatus_QMC_Ok;

}

/*!
 * @brief Encrypt data by AES256_CTR cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_crypt_aes256_ctr(uint8_t *dst, const uint8_t *src,
		size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks) {

	mbedtls_aes_init(&pctx->ctx);
		if( mbedtls_aes_setkey_enc( &pctx->ctx, pctx->key, 256) != 0)
		{
			dbgLcryptPRINTF("Encrypt1 AES CTR failed. LCRYPTO.\r\n");
			mbedtls_aes_free( &pctx->ctx );
			return kStatus_QMC_Err;
		}

	size_t nc_off = 0;
	if (mbedtls_aes_crypt_ctr(&pctx->ctx, size, &nc_off, pctx->iv, NULL, src, dst) != 0)
	{
		dbgLcryptPRINTF("Encrypt AES CTR failed. LCRYPTO.\r\n");
		mbedtls_aes_free(&pctx->ctx);

		return kStatus_QMC_Err;
	}
	mbedtls_aes_free(&pctx->ctx);

	return kStatus_QMC_Ok;
}

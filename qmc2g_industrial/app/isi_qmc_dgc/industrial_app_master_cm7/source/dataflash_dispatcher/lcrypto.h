/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __LCRYPTO_H__
#define __LCRYPTO_H__

#include "api_qmc_common.h"
#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>

//macros for encrypting log_records for flash-nor
#define LCRYPTO_HASH_SIZE                 (32U)
#define LCRYPTO_AES_KEY_SIZE              (32U)
#define LCRYPTO_AES_IV_SIZE               (16U)

//macros for encrypting log_records for export to SDcard or Cloud services
#define LCRYPTO_EX_HASH384_SIZE              (48U)
#define LCRYPTO_EX_AES_KEY_SIZE              (32U)
#define LCRYPTO_EX_AES_IV_SIZE               (16U)
#define LCRYPTO_EX_RSA_KEY_SIZE              (3*1024/8)
#define LCRYPTO_EX_SIGN_SIZE                 (150U)

typedef struct {
	mbedtls_aes_context ctx;
	uint8_t iv[LCRYPTO_AES_IV_SIZE];
	uint8_t key[LCRYPTO_AES_KEY_SIZE];
} lcrypto_aes_ctx_t;

//Credentials provided by SBL
typedef struct _log_keys
{
       uint8_t aesKeyConfig[LCRYPTO_AES_KEY_SIZE]; // 32 bytes
       uint8_t nonceConfig[LCRYPTO_AES_IV_SIZE];   // 16 bytes
       uint8_t aesKeyLog[LCRYPTO_AES_KEY_SIZE];    // 32
       uint8_t nonceLog[LCRYPTO_AES_IV_SIZE];      // 16
}  log_keys_t;

extern volatile log_keys_t g_sbl_prov_keys;

/*!
 * @brief Init function for LCRYPTO layer used by datalogger and configuration service
 */
qmc_status_t LCRYPTO_init();

/*!
 * @brief Get SHA256 using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_get_sha256( uint8_t *dst, const uint8_t *src, size_t size, mbedtls_sha256_context *pctx, TickType_t ticks);

/*!
 * @brief Init AES256 context using with mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_init_aes256( lcrypto_aes_ctx_t *pctx, TickType_t ticks);

/*!
 * @brief Encrypt data by AES256_CBC cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_encrypt_aes256_cbc( uint8_t *dst, const uint8_t *src, size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks);

/*!
 * @brief Decrypt data by AES256_CBC cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_decrypt_aes256_cbc( uint8_t *dst, const uint8_t *src, size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks);

/*!
 * @brief Encrypt data by AES256_CTR cipher using mbedtls (CAAM) library.
 */
qmc_status_t LCRYPTO_crypt_aes256_ctr( uint8_t *dst, const uint8_t *src, size_t size, lcrypto_aes_ctx_t *pctx, TickType_t ticks);

/*!
 * @brief Encrypt data by RSA cipher using mbedtls (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_crypt_RSA( uint8_t *pdst, size_t *pdst_len, const uint8_t *psrc, const size_t src_len);

/*!
 * @brief Get random data using (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_get_RND( uint8_t *pdst, size_t dst_len);

/*!
 * @brief Get SHA384 using mbedtls (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_get_sha384( uint8_t *pdst, size_t *dst_len, const uint8_t *psrc, const size_t src_len);

/*!
 * @brief Sign data by ECDSA algorithm using mbedtls (SE05x sss) library.
 */
qmc_status_t LCRYPTO_SE_sign_ECC( uint8_t *pdst, size_t *pdst_len, const uint8_t *psrc, const size_t src_len);

#endif

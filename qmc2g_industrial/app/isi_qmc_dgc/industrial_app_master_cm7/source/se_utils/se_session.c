/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "se_session.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "ax_reset.h"
#include "se_reset_config.h"
#include "sm_timer.h"
#include "ksdk_mbedtls.h"
#include "PlugAndTrust_Pkg_Ver.h"
#include "nxLog_App.h"
#include "ex_sss_boot.h"
#include "sm_demo_utils.h"

#include <stdatomic.h>
#include "ThreadSafetyWorkaround.h"

#define SE_CHECK_SUCCESS(x, y)  if (kStatus_SSS_Success != (x)) { \
                                   LOG_E((y)); \
                                   goto cleanup; \
                                   }

#ifdef NO_SBL
  /* SE051C2 default SCP keys */
  const uint8_t scp_enc_51c2[] = {0xbf, 0xc2, 0xdb, 0xe1, 0x82, 0x8e, 0x03, 0x5d, 0x3e, 0x7f, 0xa3, 0x6b, 0x90, 0x2a, 0x05, 0xc6};
  const uint8_t scp_mac_51c2[] = {0xbe, 0xf8, 0x5b, 0xd7, 0xba, 0x04, 0x97, 0xd6, 0x28, 0x78, 0x1c, 0xe4, 0x7b, 0x18, 0x8c, 0x96};
  const uint8_t scp_dek_51c2[] = {0xd8, 0x73, 0xf3, 0x16, 0xbe, 0x29, 0x7f, 0x2f, 0xc9, 0xc0, 0xe4, 0x5f, 0x54, 0x71, 0x06, 0x99};
#endif

/* Use the ex_sss_boot_ctx_t type for convenience and
 * because that is what some SDK code requires       */
static ex_sss_boot_ctx_t gs_seCtx;
ex_sss_boot_ctx_t *g_pSeCtx = &gs_seCtx;
static SE05x_Connect_Ctx_t gs_platformCtx;
static sss_session_t gs_platformSession;
static sss_tunnel_t gs_tunnel;

/* file local flag */
#define SE_USE_SCP03 (1)
#define SCP_KEY_LENGTH (16)
#define AUTH_KEY_LENGTH (16)
#define MAIN_FW_SCP03_KEYS_ADDRESS (0x2034FC80U) /* needs to be aligned with qmc2g_bootloader/source/includes/qmc2_boot_cfg.h */

typedef struct _scp03_keys
{
	uint8_t enc_key[SCP_KEY_LENGTH];
	uint8_t mac_key[SCP_KEY_LENGTH];
	uint8_t dek_key[SCP_KEY_LENGTH];
} scp03_keys_t;

#if defined(SE_USE_SCP03) && (SE_USE_SCP03)
static sss_status_t SE_OpenSCP03(ex_sss_boot_ctx_t *pCtx)
{

#ifdef NO_SBL
    const uint8_t *scp_enc = scp_enc_51c2;
    const uint8_t *scp_mac = scp_mac_51c2;
    const uint8_t *scp_dek = scp_dek_51c2;
#else
    const uint8_t *scp_enc = ((scp03_keys_t*) MAIN_FW_SCP03_KEYS_ADDRESS)->enc_key;
    const uint8_t *scp_mac = ((scp03_keys_t*) MAIN_FW_SCP03_KEYS_ADDRESS)->mac_key;
    const uint8_t *scp_dek = ((scp03_keys_t*) MAIN_FW_SCP03_KEYS_ADDRESS)->dek_key;
#endif
    sss_status_t status = kStatus_SSS_Fail;
    size_t userAuthAesKeyDataSize = AUTH_KEY_LENGTH, userAuthAesKeyDataSizeBits = 8 * AUTH_KEY_LENGTH;
    uint8_t userAuthAesKeyData[AUTH_KEY_LENGTH];
    bool isActiveSessionPlatform = false, isActiveSessionUserAuth = false, isActiveTunnel = false;
    static NXSCP03_StaticCtx_t scp_static;
    static NXSCP03_DynCtx_t scp_dyn;
    sss_key_store_t platformKeyStore;
    sss_object_t userAuthAesKeyObject;

    memset(&gs_platformCtx, 0, sizeof(gs_platformCtx));
    memset(&gs_platformSession, 0, sizeof(gs_platformSession));
	memset(&gs_tunnel, 0, sizeof(gs_tunnel));
    memset(&scp_static, 0, sizeof(scp_static));
    memset(&scp_dyn, 0, sizeof(scp_dyn));
    memset(&platformKeyStore, 0, sizeof(platformKeyStore));

    /* setup SE platform connection context */
    gs_platformCtx.portName                   = NULL;
    gs_platformCtx.connType                   = kType_SE_Conn_Type_T1oI2C;
    gs_platformCtx.refresh_session            = 0;
    gs_platformCtx.auth.authType              = kSSS_AuthType_SCP03;
    gs_platformCtx.auth.ctx.scp03.pStatic_ctx = &scp_static;
    gs_platformCtx.auth.ctx.scp03.pDyn_ctx    = &scp_dyn;

    /* setup SE user authentication connection context (AES authentication object) */
    pCtx->se05x_open_ctx.portName                   = NULL;
    pCtx->se05x_open_ctx.connType                   = kType_SE_Conn_Type_Channel;
    pCtx->se05x_open_ctx.refresh_session            = 0;
    pCtx->se05x_open_ctx.auth.authType              = kSSS_AuthType_AESKey;
    pCtx->se05x_open_ctx.auth.ctx.scp03.pStatic_ctx = &pCtx->ex_se05x_auth.scp03.ex_static;
    pCtx->se05x_open_ctx.auth.ctx.scp03.pDyn_ctx    = &pCtx->ex_se05x_auth.scp03.ex_dyn;
    pCtx->se05x_open_ctx.tunnelCtx                  = &gs_tunnel;
    pCtx->pTunnel_ctx                               = &gs_tunnel;

    /* HOST session and key store */
    status = sss_session_open(&pCtx->host_session, kType_SSS_mbedTLS, 0, kSSS_ConnectionType_Plain, NULL);
    SE_CHECK_SUCCESS(status, "Failed to open mbedtls Session");
    status = sss_key_store_context_init(&pCtx->host_ks, &pCtx->host_session);
    SE_CHECK_SUCCESS(status, "sss_key_store_context_init failed");
    status = sss_key_store_allocate(&pCtx->host_ks, __LINE__);
    SE_CHECK_SUCCESS(status, "sss_key_store_allocate failed");

    /* set platform SCP keys (static) */
    status = sss_key_object_init(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Enc, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static SCP03 ENC key failed");
    status = sss_key_object_allocate_handle(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Enc, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static SCP03 ENC key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Enc, scp_enc, SCP_KEY_LENGTH, SCP_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static SCP03 ENC key failed");
    status = sss_key_object_init(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Mac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static SCP03 MAC key failed");
    status = sss_key_object_allocate_handle(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Mac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static SCP03 MAC key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Mac, scp_mac, SCP_KEY_LENGTH, SCP_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static SCP03 MAC key failed");
    status = sss_key_object_init(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Dek, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static SCP03 DEK key failed");
    status = sss_key_object_allocate_handle(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Dek, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static SCP03 DEK key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Dek, scp_dek, SCP_KEY_LENGTH, SCP_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static SCP03 DEK key failed");

    /* set platform SCP keys (dynamic) */
    status = sss_key_object_init(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Enc, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic SCP03 ENC key failed");
    status = sss_key_object_allocate_handle(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Enc, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic SCP03 ENC key failed");
    status = sss_key_object_init(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Mac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic SCP03 MAC key failed");
    status = sss_key_object_allocate_handle(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Mac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic SCP03 MAC key failed");
    status = sss_key_object_init(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Rmac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic SCP03 RMAC key failed");
    status = sss_key_object_allocate_handle(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Rmac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic SCP03 RMAC key failed");

    /* SE platform session (SCP03) */
    status = sss_session_open(&gs_platformSession, kType_SSS_SE_SE05x, 0, kSSS_ConnectionType_Encrypted, &gs_platformCtx);
    SE_CHECK_SUCCESS(status, "sss_session_open failed (platform)");
    isActiveSessionPlatform = true;

    /* fetch user authentication key */
    status = sss_key_store_context_init(&platformKeyStore, &gs_platformSession);
    SE_CHECK_SUCCESS(status, "sss_key_store_context_init failed (platform)");
    status = sss_key_store_allocate(&platformKeyStore, __LINE__);
    SE_CHECK_SUCCESS(status, "sss_key_store_allocate failed (platform)");
    status = sss_key_object_init(&userAuthAesKeyObject, &platformKeyStore);
    SE_CHECK_SUCCESS(status, "initializing user authentication key failed");
    status = sss_key_object_get_handle(&userAuthAesKeyObject, idAppAuthObjectFirstRun);
    SE_CHECK_SUCCESS(status, "accessing user authentication key failed");
    /* compared to the SBL authentication key, the app authentication key is not security relevant.
     * It serves as a default authentication object to apply limited default policies to the objects stored in the SE */
    status = sss_key_store_get_key(&platformKeyStore, &userAuthAesKeyObject, userAuthAesKeyData, &userAuthAesKeyDataSize, &userAuthAesKeyDataSizeBits);
    SE_CHECK_SUCCESS(status, "reading user authentication key failed");
    sss_key_object_free(&userAuthAesKeyObject);
    sss_key_store_context_free(&platformKeyStore);

    /* set user authentication SCP keys (static) */
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_static.Enc, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static user authentication ENC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_static.Enc, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, AUTH_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static user authentication ENC key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &pCtx->ex_se05x_auth.scp03.ex_static.Enc, userAuthAesKeyData, AUTH_KEY_LENGTH, AUTH_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static user authentication ENC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_static.Mac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static user authentication MAC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_static.Mac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, AUTH_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static user authentication MAC key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &pCtx->ex_se05x_auth.scp03.ex_static.Mac, userAuthAesKeyData, AUTH_KEY_LENGTH, AUTH_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static user authentication MAC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_static.Dek, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static user authentication DEK key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_static.Dek, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, AUTH_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static user authentication DEK key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &pCtx->ex_se05x_auth.scp03.ex_static.Dek, userAuthAesKeyData, AUTH_KEY_LENGTH, AUTH_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static user authentication DEK key failed");

    /* set user authentication SCP keys (dynamic) */
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic user authentication ENC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic user authentication ENC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic user authentication MAC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic user authentication MAC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic user authentication RMAC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic user authentication RMAC key failed");

    /* init tunnel context */
    status = sss_tunnel_context_init(&gs_tunnel, &gs_platformSession);
    SE_CHECK_SUCCESS(status, "initializing tunnel failed");
    isActiveTunnel = true;

    /* SE session (AES user authentication) */
    status = sss_session_open(&pCtx->session, kType_SSS_SE_SE05x, idAppAuthObject, kSSS_ConnectionType_Encrypted, &pCtx->se05x_open_ctx);
    SE_CHECK_SUCCESS(status, "sss_session_open failed (user authentication)");
    isActiveSessionUserAuth = true;

    /* SE key store */
    status = sss_key_store_context_init(&pCtx->ks, &pCtx->session);
    SE_CHECK_SUCCESS(status, "sss_key_store_context_init Failed");
    status = sss_key_store_allocate(&pCtx->ks, __LINE__);
    SE_CHECK_SUCCESS(status, "sss_key_store_allocate Failed");

    return kStatus_SSS_Success;

cleanup:
    sss_key_object_free(&userAuthAesKeyObject);
    sss_key_store_context_free(&platformKeyStore);
    if(isActiveSessionUserAuth)
        sss_session_close(&pCtx->session);
    if(isActiveSessionPlatform)
        sss_session_close(&gs_platformSession);
    sss_key_store_context_free(&pCtx->ks);
    if(isActiveTunnel)
    	sss_tunnel_context_free(&gs_tunnel);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Dek);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Mac);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Enc);
    sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Enc);
    sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Mac);
    sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Rmac);
    sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Dek);
    sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Mac);
    sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Enc);
    sss_session_close(&pCtx->host_session);
    sss_key_store_context_free(&pCtx->host_ks);
    return status;
}

static void SE_CloseSCP03(ex_sss_boot_ctx_t *pCtx)
{
    sss_session_close(&pCtx->session);
    sss_session_close(&gs_platformSession);
	sss_key_store_context_free(&pCtx->ks);
	sss_tunnel_context_free(&gs_tunnel);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Dek);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Mac);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Enc);
	sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Enc);
	sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Mac);
	sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pDyn_ctx->Rmac);
	sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Dek);
	sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Mac);
	sss_key_object_free(&gs_platformCtx.auth.ctx.scp03.pStatic_ctx->Enc);
	sss_session_close(&pCtx->host_session);
	sss_key_store_context_free(&pCtx->host_ks);
}
#else
static sss_status_t SE_OpenPlain(ex_sss_boot_ctx_t *pCtx)
{
    sss_status_t status = kStatus_SSS_Fail;
    size_t userAuthAesKeyDataSize = AUTH_KEY_LENGTH, userAuthAesKeyDataSizeBits = 8 * AUTH_KEY_LENGTH;
    uint8_t userAuthAesKeyData[AUTH_KEY_LENGTH];
    bool isActiveSessionPlatform = false, isActiveSessionUserAuth = false, isActiveTunnel = false;
    sss_key_store_t platformKeyStore;
    sss_object_t userAuthAesKeyObject;

    memset(&gs_platformCtx, 0, sizeof(gs_platformCtx));
    memset(&gs_platformSession, 0, sizeof(gs_platformSession));
	memset(&gs_tunnel, 0, sizeof(gs_tunnel));
    memset(&platformKeyStore, 0, sizeof(platformKeyStore));

    /* setup SE platform connection context */
    gs_platformCtx.portName                   = NULL;
    gs_platformCtx.connType                   = kType_SE_Conn_Type_T1oI2C;

    /* setup SE user authentication connection context (AES authentication object) */
    pCtx->se05x_open_ctx.portName                   = NULL;
    pCtx->se05x_open_ctx.connType                   = kType_SE_Conn_Type_Channel;
    pCtx->se05x_open_ctx.refresh_session            = 0;
    pCtx->se05x_open_ctx.auth.authType              = kSSS_AuthType_AESKey;
    pCtx->se05x_open_ctx.auth.ctx.scp03.pStatic_ctx = &pCtx->ex_se05x_auth.scp03.ex_static;
    pCtx->se05x_open_ctx.auth.ctx.scp03.pDyn_ctx    = &pCtx->ex_se05x_auth.scp03.ex_dyn;
    pCtx->se05x_open_ctx.tunnelCtx                  = &gs_tunnel;
    pCtx->pTunnel_ctx                               = &gs_tunnel;

    /* HOST session and key store */
    status = sss_session_open(&pCtx->host_session, kType_SSS_mbedTLS, 0, kSSS_ConnectionType_Plain, NULL);
    SE_CHECK_SUCCESS(status, "Failed to open mbedtls Session");
    status = sss_key_store_context_init(&pCtx->host_ks, &pCtx->host_session);
    SE_CHECK_SUCCESS(status, "sss_key_store_context_init failed");
    status = sss_key_store_allocate(&pCtx->host_ks, __LINE__);
    SE_CHECK_SUCCESS(status, "sss_key_store_allocate failed");

    /* SE platform session (plain) */
    status = sss_session_open(&gs_platformSession, kType_SSS_SE_SE05x, 0, kSSS_ConnectionType_Plain, &gs_platformCtx);
    SE_CHECK_SUCCESS(status, "sss_session_open failed (platform)");
    isActiveSessionPlatform = true;

    /* fetch user authentication key */
    status = sss_key_store_context_init(&platformKeyStore, &gs_platformSession);
    SE_CHECK_SUCCESS(status, "sss_key_store_context_init failed (platform)");
    status = sss_key_store_allocate(&platformKeyStore, __LINE__);
    SE_CHECK_SUCCESS(status, "sss_key_store_allocate failed (platform)");
    status = sss_key_object_init(&userAuthAesKeyObject, &platformKeyStore);
    SE_CHECK_SUCCESS(status, "initializing user authentication key failed");
    status = sss_key_object_get_handle(&userAuthAesKeyObject, idAppAuthObjectFirstRun);
    SE_CHECK_SUCCESS(status, "accessing user authentication key failed");
    /* compared to the SBL authentication key, the app authentication key is not security relevant.
     * It serves as a default authentication object to apply limited default policies to the objects stored in the SE */
    status = sss_key_store_get_key(&platformKeyStore, &userAuthAesKeyObject, userAuthAesKeyData, &userAuthAesKeyDataSize, &userAuthAesKeyDataSizeBits);
    SE_CHECK_SUCCESS(status, "reading user authentication key failed");
    sss_key_object_free(&userAuthAesKeyObject);
    sss_key_store_context_free(&platformKeyStore);

    /* set user authentication SCP keys (static) */
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_static.Enc, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static user authentication ENC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_static.Enc, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, AUTH_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static user authentication ENC key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &pCtx->ex_se05x_auth.scp03.ex_static.Enc, userAuthAesKeyData, AUTH_KEY_LENGTH, AUTH_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static user authentication ENC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_static.Mac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static user authentication MAC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_static.Mac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, AUTH_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static user authentication MAC key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &pCtx->ex_se05x_auth.scp03.ex_static.Mac, userAuthAesKeyData, AUTH_KEY_LENGTH, AUTH_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static user authentication MAC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_static.Dek, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing static user authentication DEK key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_static.Dek, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, AUTH_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating static user authentication DEK key failed");
    status = sss_key_store_set_key(&pCtx->host_ks, &pCtx->ex_se05x_auth.scp03.ex_static.Dek, userAuthAesKeyData, AUTH_KEY_LENGTH, AUTH_KEY_LENGTH * 8, NULL, 0);
    SE_CHECK_SUCCESS(status, "setting static user authentication DEK key failed");

    /* set user authentication SCP keys (dynamic) */
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic user authentication ENC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic user authentication ENC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic user authentication MAC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic user authentication MAC key failed");
    status = sss_key_object_init(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac, &pCtx->host_ks);
    SE_CHECK_SUCCESS(status, "initializing dynamic user authentication RMAC key failed");
    status = sss_key_object_allocate_handle(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac, __LINE__, kSSS_KeyPart_Default, kSSS_CipherType_AES, SCP_KEY_LENGTH, kKeyObject_Mode_Transient);
    SE_CHECK_SUCCESS(status, "allocating dynamic user authentication RMAC key failed");

    /* init tunnel context */
    status = sss_tunnel_context_init(&gs_tunnel, &gs_platformSession);
    SE_CHECK_SUCCESS(status, "initializing tunnel failed");
    isActiveTunnel = true;

    /* SE session (AES user authentication) */
    status = sss_session_open(&pCtx->session, kType_SSS_SE_SE05x, idAppAuthObject, kSSS_ConnectionType_Encrypted, &pCtx->se05x_open_ctx);
    SE_CHECK_SUCCESS(status, "sss_session_open failed (user authentication)");
    isActiveSessionUserAuth = true;

    /* SE key store */
    status = sss_key_store_context_init(&pCtx->ks, &pCtx->session);
    SE_CHECK_SUCCESS(status, "sss_key_store_context_init Failed");
    status = sss_key_store_allocate(&pCtx->ks, __LINE__);
    SE_CHECK_SUCCESS(status, "sss_key_store_allocate Failed");

    return kStatus_SSS_Success;

cleanup:
    sss_key_object_free(&userAuthAesKeyObject);
    sss_key_store_context_free(&platformKeyStore);
    if(isActiveSessionUserAuth)
        sss_session_close(&pCtx->session);
    if(isActiveSessionPlatform)
        sss_session_close(&gs_platformSession);
    sss_key_store_context_free(&pCtx->ks);
    if(isActiveTunnel)
    	sss_tunnel_context_free(&gs_tunnel);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Dek);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Mac);
    sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Enc);
    sss_session_close(&pCtx->host_session);
    sss_key_store_context_free(&pCtx->host_ks);
    return status;
}

static void SE_ClosePlain(ex_sss_boot_ctx_t *pCtx)
{
    sss_session_close(&pCtx->session);
    sss_session_close(&gs_platformSession);
	sss_key_store_context_free(&pCtx->ks);
	sss_tunnel_context_free(&gs_tunnel);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Enc);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Mac);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_dyn.Rmac);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Dek);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Mac);
	sss_key_object_free(&pCtx->ex_se05x_auth.scp03.ex_static.Enc);
	sss_session_close(&pCtx->host_session);
	sss_key_store_context_free(&pCtx->host_ks);
}
#endif /* SE_USE_SCP03 */

static atomic_bool gs_isInitialized = false;

bool SE_IsInitialized(void)
{
    return gs_isInitialized;
}

qmc_status_t SE_Initialize(void)
{
    if(gs_isInitialized) {
        return kStatus_QMC_Ok;
    }

    if(kStatus_SSS_Success != ThreadSafetyWorkaround_Initialize())
    	return kStatus_QMC_Err;

    axReset_HostConfigure();
    axReset_PowerUp();

    CRYPTO_InitHardware();
    sm_initSleep();

    LOG_I(PLUGANDTRUST_PROD_NAME_VER_FULL);
    memset(g_pSeCtx, 0, sizeof(*g_pSeCtx));

    /* Initialize Logging locks */
    if (nLog_Init() != 0) {
        LOG_E("Lock initialization failed");
        return kStatus_QMC_Err;
    }

    sss_status_t status;
#if defined(SE_USE_SCP03) && (SE_USE_SCP03)
    status = SE_OpenSCP03(g_pSeCtx);
#else
    status = SE_OpenPlain(g_pSeCtx);
#endif /* SE_USE_SCP03 */
    if (kStatus_SSS_Success != status) {
        LOG_E("sss init fail");
        return kStatus_QMC_Err;
    }

    LOG_I("sss init success");

    gs_isInitialized = true;
    return kStatus_QMC_Ok;
}

void SE_Deinitialize(void)
{
    if(!gs_isInitialized) {
        return;
    }

    nLog_DeInit();
#if SE_USE_SCP03
    SE_CloseSCP03(g_pSeCtx);
#else
    SE_ClosePlain(g_pSeCtx);
#endif /* SE_USE_SCP03 */

    ThreadSafetyWorkaround_Deinitialize();

    axReset_PowerDown();
    gs_isInitialized = false;
}

sss_session_t* SE_GetSession(void)
{
    return &g_pSeCtx->session;
}

sss_key_store_t* SE_GetKeystore(void)
{
    return &g_pSeCtx->ks;
}

sss_session_t* SE_GetHostSession(void)
{
    return &g_pSeCtx->host_session;
}

sss_key_store_t* SE_GetHostKeystore(void)
{
    return &g_pSeCtx->host_ks;
}

/* for testing, does not cache*/
#define SE_UID_MAX_LEN 18
const char* SE_GetUid(void)
{
    static char seUidStr[SE_UID_MAX_LEN * 2 + 1];
    unsigned int length = SE_UID_MAX_LEN;
    unsigned char data[SE_UID_MAX_LEN];

    /* Get UID as byte array */
    sss_status_t status;
    status = sss_session_prop_get_au8(&g_pSeCtx->session, kSSS_SessionProp_UID, data, &length);
    if (kStatus_SSS_Success != status) {
        LOG_E("sss_session_prop_get_au8 failed");
        seUidStr[0] = '\0';
        return seUidStr; /* return empty string*/
    }

    /* Transform into hex string */
    const char* hexChars = "0123456789ABCDEF";
    char* pSeUidStr = seUidStr;
    for(int i = 0; i < length; i++) {
        *pSeUidStr = hexChars[(data[i] >> 4) & 0x0Fu]; pSeUidStr++;
        *pSeUidStr = hexChars[(data[i]     ) & 0x0Fu]; pSeUidStr++;
    }
    *pSeUidStr = '\0';
    return seUidStr;
}


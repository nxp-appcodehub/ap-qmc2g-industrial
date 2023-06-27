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

/* Use the ex_sss_boot_ctx_t type for convenience and
 * because that is what some SDK code requires       */
static ex_sss_boot_ctx_t gs_seCtx;
ex_sss_boot_ctx_t *g_pSeCtx = &gs_seCtx;

/* file local flag */
#define SE_USE_SCP03 (0)

#if defined(SE_USE_SCP03) && (SE_USE_SCP03)
static sss_status_t SE_OpenSCP03(ex_sss_boot_ctx_t *pCtx)
{
    /* TODO */
    return kStatus_SSS_Fail;
}

static void SE_CloseSCP03(ex_sss_boot_ctx_t *pCtx)
{
    /* TODO */
}
#else
static sss_status_t SE_OpenPlain(ex_sss_boot_ctx_t *pCtx)
{
    sss_status_t status           = kStatus_SSS_Fail;

    pCtx->se05x_open_ctx.connType = kType_SE_Conn_Type_T1oI2C;
    pCtx->se05x_open_ctx.portName = NULL;

    /* SE session */
    status = sss_session_open(&pCtx->session, kType_SSS_SE_SE05x, 0, kSSS_ConnectionType_Plain, &pCtx->se05x_open_ctx);
    if (kStatus_SSS_Success != status) {
        LOG_E("sss_session_open failed");
        goto cleanup;
    }

    status = sss_key_store_context_init(&pCtx->ks, &pCtx->session);
    if (status != kStatus_SSS_Success) {
        LOG_E(" sss_key_store_context_init Failed...");
        goto cleanup;
    }

    status = sss_key_store_allocate(&pCtx->ks, __LINE__);
    if (status != kStatus_SSS_Success) {
        LOG_E(" sss_key_store_allocate Failed...");
        goto cleanup;
    }

    /* HOST session */
    status = sss_session_open(&pCtx->host_session, kType_SSS_Software, 0, kSSS_ConnectionType_Plain, NULL);
    if (kStatus_SSS_Success != status) {
        LOG_E("Failed to open mbedtls Session");
        goto cleanup;
    }

    status = sss_key_store_context_init(&pCtx->host_ks, &pCtx->host_session);
    if (kStatus_SSS_Success != status) {
        LOG_E("sss_key_store_context_init failed");
        goto cleanup;
    }

    status = sss_key_store_allocate(&pCtx->host_ks, __LINE__);
    if (kStatus_SSS_Success != status) {
        LOG_E("sss_key_store_allocate failed");
        goto cleanup;
    }

cleanup:
    return status;
}

static void SE_ClosePlain(ex_sss_boot_ctx_t *pCtx)
{
    if (pCtx->session.subsystem != kType_SSS_SubSystem_NONE) {
        sss_session_close(&pCtx->session);
        sss_session_delete(&pCtx->session);
    }

    if (pCtx->pTunnel_ctx && pCtx->pTunnel_ctx->session) {
        if (pCtx->pTunnel_ctx->session->subsystem != kType_SSS_SubSystem_NONE) {
            sss_session_close(pCtx->pTunnel_ctx->session);
            sss_tunnel_context_free(pCtx->pTunnel_ctx);
        }
    }

    if (pCtx->host_ks.session != NULL) {
        sss_host_key_store_context_free(&pCtx->host_ks);
    }
    if (pCtx->host_session.subsystem != kType_SSS_SubSystem_NONE) {
        sss_host_session_close(&pCtx->host_session);
    }

    if (pCtx->ks.session != NULL) {
        sss_key_store_context_free(&pCtx->ks);
    }
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


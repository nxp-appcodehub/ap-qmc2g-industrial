/*
 * Copyright 2024 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "se_session.h"
#include "mbedtls/entropy.h"
#include "fsl_sss_api.h"
#include "fsl_common.h"
int mbedtls_se_entropy_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
	int result = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
	(void) data;

	if(SE_IsInitialized()){
		sss_session_t* se = SE_GetSession();
		sss_rng_context_t rng;
		sss_status_t rc;

		// get 32bytes max
		len=MIN(len,32);
		*olen=0;

		rc=sss_rng_context_init(&rng, se);
		if(rc == kStatus_SSS_Success)
		{
			rc=sss_rng_get_random(&rng, output, len);
			if(rc == kStatus_SSS_Success)
			{
				*olen=len;
				result = 0;
			}
			sss_rng_context_free(&rng);
		}
	}
	return result;
}

int mbedtls_entropy_add_se_source( mbedtls_entropy_context *ctx)
{
	return mbedtls_entropy_add_source(ctx, &mbedtls_se_entropy_poll, NULL,16,MBEDTLS_ENTROPY_SOURCE_STRONG);
}

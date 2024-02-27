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
 * @file    qmc2_puf.c
 * @brief   PUF API.
 */

#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include <qmc2_puf.h>

extern const uint8_t puf_keystore[];
static sss_status_t QMC2_PUF_IsEnabled(void);
static sss_status_t PUF_LoadKeyStore(puf_key_store_t *pufKeyStore);

/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/
static sss_status_t QMC2_PUF_IsEnabled(void)
{
    /* Enable PUF in shadow registers */
    uint32_t fuse    = 0;
    uint32_t enabled = 0;

    /* Load values from OCOTP */
    fuse = OCOTP->FUSEN[6].FUSE;

    enabled = (fuse & 0x00000040) >> 6U;

    /* Check whether PUF is disabled and FUSE word locked */
    if (enabled == 0)
    {
        return kStatus_SSS_Fail;
    }

    return kStatus_SSS_Success;
}

static sss_status_t PUF_LoadKeyStore(puf_key_store_t *pufKeyStore)
{
	assert(pufKeyStore != NULL);

	/* Load PUF Key Store from the NVM */
	(void) memcpy((void *)pufKeyStore, (void *)PUF_SBL_KEY_STORE_ADDRESS, sizeof(puf_key_store_t));

	if(memcmp((void *)pufKeyStore, (void *)PUF_SBL_KEY_STORE_ADDRESS, sizeof(puf_key_store_t)) != 0)
	{
        return kStatus_SSS_Fail;
	}

	return kStatus_SSS_Success;
}

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/


sss_status_t QMC2_PUF_Init(void)
{
	sss_status_t sssStatus = kStatus_SSS_Fail;
#if (defined(ROM_ENCRYPTED_XIP_ENABLED) && (ROM_ENCRYPTED_XIP_ENABLED==0))
	status_t status = kStatus_Fail;
	uint8_t activationCode[PUF_ACTIVATION_CODE_SIZE];
	puf_config_t pufCfg;
#endif

	sssStatus = QMC2_PUF_IsEnabled();
    if(sssStatus != kStatus_SSS_Success)
    {
        PRINTF("\r\nERROR: PUF is not enabled in FUSE (0x860[6])!/r/n");
        return kStatus_SSS_InvalidArgument;
    }

#if (defined(ROM_ENCRYPTED_XIP_ENABLED) && (ROM_ENCRYPTED_XIP_ENABLED==0))

    PUF_GetDefaultConfig(&pufCfg);
    /* Initialize PUF peripheral */
    status = PUF_Init(PUF, &pufCfg);
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR:  Initializing PUF!\r\n");
        return kStatus_SSS_Fail;
    }

    (void)memcpy((void *)activationCode, (void *)PUF_AC_CODE_ADDRESS, sizeof(activationCode)); //  (void *)&puf_keystore[16]
	/* Start PUF by loading generated activation code */
    status = PUF_Start(PUF, activationCode, PUF_ACTIVATION_CODE_SIZE);
    if (status != kStatus_Success)
    {
		if (status == kStatus_StartNotAllowed)
		{
			PRINTF("PUF Start is not allowed!\r\n");
			PRINTF("PUF will make power-cycle and start again!\r\n");
			(void)PUF_PowerCycle(PUF, &pufCfg);
			status = PUF_Start(PUF, activationCode, PUF_ACTIVATION_CODE_SIZE);
		}
		else
		{
	        PRINTF("\r\nERROR: PUF Error during Start !\r\n");
		}
    }

    if (status != kStatus_Success)
    {
    	sssStatus = kStatus_SSS_Fail;
    	PRINTF("\r\nERROR: PUF Error during Start !\r\n");
    }

#endif

    return sssStatus;
}

sss_status_t QMC2_PUF_ReconstructKeys(PUF_Type *base, boot_keys_t *bootKeys)
{
	status_t status = kStatus_Fail;
	sss_status_t sssStatus = kStatus_SSS_Fail;
	puf_key_store_t pufKeyStore = {0};

	assert(base != NULL);
	assert(bootKeys != NULL);

	sssStatus = PUF_LoadKeyStore(&pufKeyStore);
	if (sssStatus != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: PUF Key Store memcmp failed!\r\n");
	    goto cleanup;
	}

    /* Reconstruct intrinsic key from enc_key_code generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.scp03.enc_key_code, SCP03_PLATFORM_KEY_CODE_SIZE, bootKeys->scp03.enc_key, sizeof(bootKeys->scp03.enc_key));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing SCP03 Encryption key!\r\n");
        goto cleanup;
    }

    /* Reconstruct intrinsic key from mac_key_code generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.scp03.mac_key_code, SCP03_PLATFORM_KEY_CODE_SIZE, bootKeys->scp03.mac_key, sizeof(bootKeys->scp03.mac_key));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing SCP03 MAC key!\r\n");
        goto cleanup;
    }

    /* Reconstruct intrinsic key from dec_key_code generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.scp03.dek_key_code, SCP03_PLATFORM_KEY_CODE_SIZE, bootKeys->scp03.dek_key, sizeof(bootKeys->scp03.dek_key));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing SCP03 Decryption key!\r\n");
        goto cleanup;
    }

    /* Reconstruct AES key from aes_policy_key_code generated by PUF */
    status = PUF_GetKey(PUF, pufKeyStore.scp03.aes_policy_key_code, SE_AES_POLICY_KEY_CODE_SIZE, bootKeys->scp03.aes_policy_key, sizeof(bootKeys->scp03.aes_policy_key));
    if (status != kStatus_Success)
    {
    	PRINTF("\r\nERROR: reconstructing SCP03 Decryption key!\r\n");
        goto cleanup;
    }

    /* Reconstruct intrinsic key 1 from AES log key generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.log.aesKeyConfig, PUF_INTRINSIC_LOG_KEY_CODE_SIZE, bootKeys->log.aesKeyConfig, sizeof(bootKeys->log.aesKeyConfig));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing Log AES-1 key!\r\n");
        goto cleanup;
    }

    memcpy(g_sbl_prov_keys.aesKeyConfig, bootKeys->log.aesKeyConfig, sizeof(bootKeys->log.aesKeyConfig));

    /* Reconstruct intrinsic nonce 1 for AES log generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.log.nonceConfig, PUF_INTRINSIC_LOG_NONCE_CODE_SIZE, bootKeys->log.nonceConfig, sizeof(bootKeys->log.nonceConfig));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing NONCE-1 for LOG AES CTR!\r\n");
        goto cleanup;
    }

    memcpy(g_sbl_prov_keys.nonceConfig, bootKeys->log.nonceConfig, sizeof(bootKeys->log.nonceConfig));

    /* Reconstruct intrinsic key 2 from AES log key generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.log.aesKeyLog, PUF_INTRINSIC_LOG_KEY_CODE_SIZE, bootKeys->log.aesKeyLog, sizeof(bootKeys->log.aesKeyLog));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing Log AES-2 key!\r\n");
        goto cleanup;
    }

    memcpy(g_sbl_prov_keys.aesKeyLog, bootKeys->log.aesKeyLog, sizeof(bootKeys->log.aesKeyLog));

    /* Reconstruct intrinsic nonce 2 for AES log generated by PUF_SetIntrinsicKey() */
	status = PUF_GetKey(PUF, pufKeyStore.log.nonceLog, PUF_INTRINSIC_LOG_NONCE_CODE_SIZE, bootKeys->log.nonceLog, sizeof(bootKeys->log.nonceLog));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: reconstructing NONCE-2 for LOG AES CTR!\r\n");
        goto cleanup;
    }

    memcpy(g_sbl_prov_keys.nonceLog, bootKeys->log.nonceLog, sizeof(bootKeys->log.nonceLog));

    return kStatus_SSS_Success;

cleanup:

	memset(&bootKeys->scp03, 0, sizeof(scp03_keys_t));
	memset(&bootKeys->log, 0, sizeof(log_keys_t));

	return kStatus_SSS_Fail;
}

/* Generatae SCP03 key codes */
sss_status_t QMC2_PUF_GeneratePufKeyCodes(PUF_Type *base, puf_key_store_t *pufKeyStore, uint8_t *aesPolicyKey)
{
	status_t status = kStatus_Fail;

	assert(base != NULL);
	assert(pufKeyStore != NULL);
	assert(aesPolicyKey != NULL);

    /* Generate new intrinsic key with index 2 for SCP03 ENC key */
    status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_ENC, PUF_INTRINSIC_SCP03_KEY_SIZE, pufKeyStore->scp03.enc_key_code, sizeof(pufKeyStore->scp03.enc_key_code));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: setting ENC intrinsic key!\r\n");
        return kStatus_SSS_Fail;
    }

    /* Generate new intrinsic key with index 3 for SCP03 MAC key */
    status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_MAC, PUF_INTRINSIC_SCP03_KEY_SIZE, pufKeyStore->scp03.mac_key_code, sizeof(pufKeyStore->scp03.mac_key_code));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: setting MAC intrinsic key!\r\n");
        return kStatus_SSS_Fail;
    }

    /* Generate new intrinsic key with index 4 for SCP03 DEC key */
    status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_DEK, PUF_INTRINSIC_SCP03_KEY_SIZE, pufKeyStore->scp03.dek_key_code, sizeof(pufKeyStore->scp03.dek_key_code));
    if (status != kStatus_Success)
    {
        PRINTF("\r\nERROR: setting DEK intrinsic key!\r\n");
        return kStatus_SSS_Fail;
    }

	/* Generate key code from aes key for SE051 policies */
	status = PUF_SetUserKey(PUF, PUF_KEY_INDEX_POLICY, aesPolicyKey, PUF_AES_POLICY_KEY_SIZE, pufKeyStore->scp03.aes_policy_key_code, sizeof(pufKeyStore->scp03.aes_policy_key_code));
	if (status != kStatus_Success)
	{
		PRINTF("\r\nERROR: setting NONCE for AES LOG failed!\r\n");
		return kStatus_SSS_Fail;
	}

    /* Generate new intrinsic key with index 6 for Logging function */
	status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_LOG_1, PUF_INTRINSIC_LOG_KEY_SIZE, pufKeyStore->log.aesKeyConfig, sizeof(pufKeyStore->log.aesKeyConfig));
	if (status != kStatus_Success)
	{
		PRINTF("\r\nERROR: setting Log-1 AES 256 bit intrinsic key!\r\n");
		return kStatus_SSS_Fail;
	}

    /* Generate new intrinsic Nonce with index 7 for Logging function */
	status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_LOG_NONCE_1, PUF_INTRINSIC_LOG_NONCE_SIZE, pufKeyStore->log.nonceConfig, sizeof(pufKeyStore->log.nonceConfig));
	if (status != kStatus_Success)
	{
		PRINTF("\r\nERROR: setting NONCE-1 for AES LOG failed!\r\n");
		return kStatus_SSS_Fail;
	}

    /* Generate new intrinsic key with index 6 for Logging function */
	status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_LOG_2, PUF_INTRINSIC_LOG_KEY_SIZE, pufKeyStore->log.aesKeyLog, sizeof(pufKeyStore->log.aesKeyLog));
	if (status != kStatus_Success)
	{
		PRINTF("\r\nERROR: setting Log-2 AES 256 bit intrinsic key!\r\n");
		return kStatus_SSS_Fail;
	}

    /* Generate new intrinsic Nonce with index 7 for Logging function */
	status = PUF_SetIntrinsicKey(PUF, PUF_KEY_INDEX_LOG_NONCE_2, PUF_INTRINSIC_LOG_NONCE_SIZE, pufKeyStore->log.nonceLog, sizeof(pufKeyStore->log.nonceLog));
	if (status != kStatus_Success)
	{
		PRINTF("\r\nERROR: setting NONCE-2 for AES LOG failed!\r\n");
		return kStatus_SSS_Fail;
	}

    return kStatus_SSS_Success;

}

/* Program SCP03 PUF Key Store into ext. flash. */
sss_status_t QMC2_PUF_ProgramKeyStore(uint32_t address, puf_key_store_t *pufKeyStore)
{
	sss_status_t status = kStatus_SSS_Fail;
	uint8_t *pgmBuffer = NULL;
	uint32_t pgmBufferSize = 0;

	assert(pufKeyStore != NULL);

    /* Erase the context we have programmed before*/
	pgmBufferSize = sizeof(puf_key_store_t) + (PGM_PAGE_SIZE - (sizeof(puf_key_store_t) % PGM_PAGE_SIZE));

	status = QMC2_FLASH_Erase(address, QSPI_FLASH_ERASE_SECTOR_SIZE);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the SBL PUF key store failed.!\r\n");
		return status;
	}

	pgmBuffer = malloc(pgmBufferSize);
	if (pgmBuffer == NULL)
	{
		PRINTF("\r\nERROR: Malloc failed!\r\n");
		return kStatus_SSS_InvalidArgument;
	}

	memcpy((void *)pgmBuffer, (void *)pufKeyStore, sizeof(puf_key_store_t));
	status = QMC2_FLASH_ProgramPages(address, (const void *)pgmBuffer, pgmBufferSize);
	free(pgmBuffer);

	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: in programming SBL PUF key store!\r\n");
		return status;
	}

	if (memcmp((void*)address, (void*)pufKeyStore, sizeof(puf_key_store_t)) != 0)
	{
		return kStatus_SSS_Fail;
	}

	return status;
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "qmc2_boot.h"
#include "resource_config.h"
#include "fsl_rdc.h"
#include "mcmgr.h"
#include <ex_sss_auth.h>
#include "board.h"
#include "fsl_snvs_hp.h"
#include "fsl_snvs_lp.h"
#include <qmc2_flash.h>
#include <qmc2_fwu.h>
#include <qmc2_log.h>
#include <qmc2_lpgpr.h>
#include <qmc2_puf.h>
#include <qmc2_sd_fatfs.h>
#include <qmc2_se.h>
#include "init_rpc.h"
#include "ax_reset.h"
#include "fsl_soc_src.h"
#include "verify_config.h"

#ifdef SECURE_SBL
static uint8_t defaultScp03PlatformKeyEnc[] = EX_SSS_AUTH_SE05X_KEY_ENC;
static uint8_t defaultScp03PlatformKeyMac[] = EX_SSS_AUTH_SE05X_KEY_MAC;
static uint8_t defaultScp03PlatformKeyDek[] = EX_SSS_AUTH_SE05X_KEY_DEK;
#endif

uint8_t rdcSha512App[] = RDC_SHA512_APP;

/*! @brief SNVS LP SRTC configuration */
static const snvs_lp_srtc_config_t kSnvsLpConfig = {.srtcCalEnable = false, .srtcCalValue = 0U};
/*!
 * @brief Application-specific implementation of the SystemInitHook() weak function.
 */
static sss_status_t SBL_Cm4Release(uint32_t bootAddress);
static sss_status_t SBL_LoadCm4FwImage(fw_header_t *currFwHeader);
static sss_status_t RPC_InitSecureWatchdog(const uint8_t *seed, size_t seedLen, const uint8_t *key, size_t keyLen);
static sss_status_t SBL_GetSignatureSize(uint32_t signAddr, uint32_t *signSize);
static sss_status_t RPC_IsSecureWatchdogReady(uint32_t attempts);

/* Wait for Secure WDOG ready flag */
static sss_status_t RPC_IsSecureWatchdogReady(uint32_t attempts)
{
    rpc_secwd_init_data_t *pInitData = (rpc_secwd_init_data_t *) RPC_SECWD_INIT_DATA_ADDRESS;

    while(attempts--)
    {
    	if(pInitData->ready == SECURE_WDOG_READY)
    		return kStatus_SSS_Success;

    	SDK_DelayAtLeastUs(10000, (uint32_t)BOARD_BOOTCLOCKRUN_CORE_CLOCK);
    }

    return kStatus_SSS_Fail;
}

/* RPC Init - passing seed and key*/
static sss_status_t RPC_InitSecureWatchdog(const uint8_t *seed, size_t seedLen, const uint8_t *key, size_t keyLen)
{
	sss_status_t status = kStatus_SSS_Fail;

	assert(seed != NULL);
	assert(key != NULL);

    if((seedLen > RPC_SECWD_MAX_RNG_SEED_SIZE) || (keyLen >  RPC_SECWD_MAX_PK_SIZE))
    {
    	status = kStatus_SSS_InvalidArgument;
    }
    else
    {
        rpc_secwd_init_data_t *pInitData = (rpc_secwd_init_data_t *) RPC_SECWD_INIT_DATA_ADDRESS;
        (void)memcpy(pInitData->rngSeed, seed, seedLen);
        pInitData->rngSeedLen = seedLen;
        (void)memcpy(pInitData->pk, key, keyLen);
        pInitData->pkLen = keyLen;

        status = kStatus_SSS_Success;
    }

    return status;
}
/* Execute CM4 app. */
/* Jump to CM7 application */
//static sss_status_t SBL_LoadCm4FwImage(uint32_t srcAddress, uint32_t dstAddress, uint32_t size)
static sss_status_t SBL_LoadCm4FwImage(fw_header_t *currFwHeader)
{
		/* This section ensures the secondary core image is copied from flash location to the target RAM memory.
		   It consists of several steps: image size calculation, image copying and cache invalidation (optional for some
		   platforms/cases). These steps are not required on MCUXpresso IDE which copies the secondary core image to the
		   target memory during startup automatically. */

		/* Copy Secondary core application from FLASH to the target memory. */
		(void)memcpy((void *)(char *)currFwHeader->hdr.cm4BootAddr, (void *)currFwHeader->hdr.cm4FwDataAddr, currFwHeader->hdr.cm4FwDataLength);

		if(memcmp((void *)(char *)currFwHeader->hdr.cm4BootAddr, (void *)currFwHeader->hdr.cm4FwDataAddr, currFwHeader->hdr.cm4FwDataLength) != 0)
		{
			return kStatus_SSS_Fail;
		}

		return kStatus_SSS_Success;
}


sss_status_t QMC2_BOOT_ExecuteCm4Cm7Fws(fw_header_t *currFwHeader)
{
	static void (*farewell_bootloader)(void) = 0;

	assert(currFwHeader != NULL);

	if(currFwHeader->hdr.magic != MAIN_FW_HEADER_MAGIC)
	{
		return kStatus_SSS_InvalidArgument;
	}

    /* Check boundaries to makes sure that data which going to be executed are inside signature verification range. */
    if(((currFwHeader->hdr.cm7VectorTableAddr) < (currFwHeader->hdr.fwDataAddr)) && ((currFwHeader->hdr.cm7VectorTableAddr) >= (currFwHeader->hdr.fwDataAddr + currFwHeader->hdr.fwDataLength)) &&
    ((*((uint32_t *)(currFwHeader->hdr.cm7VectorTableAddr+4))) < (currFwHeader->hdr.fwDataAddr)) && ((*((uint32_t *)(currFwHeader->hdr.cm7VectorTableAddr+4))) >= (currFwHeader->hdr.fwDataAddr + currFwHeader->hdr.fwDataLength)) &&
    (((currFwHeader->hdr.cm4FwDataAddr) < (currFwHeader->hdr.fwDataAddr)) && ((currFwHeader->hdr.cm4FwDataAddr + currFwHeader->hdr.cm4FwDataLength) >= (currFwHeader->hdr.fwDataAddr + currFwHeader->hdr.fwDataLength))))
    {
    	return kStatus_SSS_Fail;
    }

	if(SBL_LoadCm4FwImage(currFwHeader) != kStatus_SSS_Success)
	{
		return kStatus_SSS_Fail;
	}

   	/* Data Synchronization Barrier, Instruction Synchronization Barrier, and Data Memory Barrier. */
    __DSB();
    __ISB();
    __DMB();

    farewell_bootloader = (void (*)(void)) (*((uint32_t *)(currFwHeader->hdr.cm7VectorTableAddr+4)));

    /* Data Synchronization Barrier, Instruction Synchronization Barrier, and Data Memory Barrier. */
    __DSB();
    __ISB();
    __DMB();

    /* Boot Secondary core application */
	if(SBL_Cm4Release(currFwHeader->hdr.cm4BootAddr) != kStatus_SSS_Success)
	{
		return kStatus_SSS_Fail;
	}

	/* Configure RDC vie TEE tool. Must be executed before jump to CM7 main FW*/
	BOARD_InitTEE();

    /* Data Synchronization Barrier, Instruction Synchronization Barrier, and Data Memory Barrier. */
    __DSB();
    __ISB();
    __DMB();

    if(Verify_Rdc_Config(rdcSha512App) != kStatus_SSS_Success)
	{
		return kStatus_SSS_Fail;
	}

    /* Set Stack pointer */
    __set_MSP((*((uint32_t *)(currFwHeader->hdr.cm7VectorTableAddr))));

    /* Check boundaries to makes sure that data which going to be executed are inside signature verification range. */
    if((((uint32_t)(*farewell_bootloader)) < (currFwHeader->hdr.fwDataAddr)) && (((uint32_t)(*farewell_bootloader)) >= (currFwHeader->hdr.fwDataAddr + currFwHeader->hdr.fwDataLength)))
    {
    	return kStatus_SSS_Fail;
    }

    if(Verify_Rdc_Config(rdcSha512App) != kStatus_SSS_Success)
	{
		return kStatus_SSS_Fail;
	}
    else
    {
    	 ARM_MPU_Disable();

   	    /* Data Synchronization Barrier, Instruction Synchronization Barrier, and Data Memory Barrier. */
   	    __DSB();
   	    __ISB();
   	    __DMB();

   	    /* Jump to the application. */
   	    farewell_bootloader();

   	    /* Data Synchronization Barrier, Instruction Synchronization Barrier. */
   	    __DSB();
   	    __ISB();
    }

    // the code should never reach here
    return kStatus_SSS_Fail;
}

static sss_status_t SBL_Cm4Release(uint32_t bootAddress)
{
	IOMUXC_LPSR_GPR->GPR0 = ((uint32_t)(0x0000FFFF&(uint32_t)bootAddress));
	IOMUXC_LPSR_GPR->GPR1 = IOMUXC_LPSR_GPR_GPR1_CM4_INIT_VTOR_HIGH((uint32_t)bootAddress >> 16);

	SRC->CTRL_M4CORE = SRC_CTRL_M4CORE_SW_RESET_MASK;
	SRC->SCR |= SRC_SCR_BT_RELEASE_M4_MASK;

    SDK_DelayAtLeastUs(20000, (uint32_t)BOARD_BOOTCLOCKRUN_CORE_CLOCK);

    return RPC_IsSecureWatchdogReady(10);
}

sss_status_t QMC2_BOOT_Cleanup(boot_data_t *boot)
{
	SCB_DisableICache();
	status_t status = kStatus_Fail;

    if(boot->sdCard.isInserted)
    {
    	(void)QMC2_SD_FATFS_DeInit();
    }

	memset((void *)&boot->fwuManifest, 0, (sizeof(boot_data_t) - sizeof(fw_header_t)));

#if (defined(ROM_ENCRYPTED_XIP_ENABLED) && (ROM_ENCRYPTED_XIP_ENABLED==1))
	status = PUF_Zeroize(PUF);
    if(status != kStatus_Success)
    	return kStatus_Fail;
#endif

    status = DbgConsole_Deinit();
    if(status != kStatus_Success)
    	return kStatus_SSS_Fail;

    QMC2_FLASH_CleanFlexSpiData();

    SysTick->CTRL = 0;

    __disable_irq();

    axReset_PowerDown();

    // Init OCtal Ram here as there are data to be copied in the main FW statup.
    (void)QMC2_FLASH_OctalRamInit();

    /* Slice SW reset and PGMC config moved from the main FW to disable access to SRC by RDC. */
    SRC_AssertSliceSoftwareReset(SRC, kSRC_DisplaySlice);
    /* 1. Power on and isolation off. */
    PGMC_BPC4->BPC_POWER_CTRL |= (PGMC_BPC_BPC_POWER_CTRL_PSW_ON_SOFT_MASK | PGMC_BPC_BPC_POWER_CTRL_ISO_OFF_SOFT_MASK);

    return kStatus_SSS_Success;
}

#ifdef SECURE_SBL
sss_status_t QMC2_BOOT_RotateMandateSCP03Keys(PUF_Type *base, boot_data_t *boot, log_event_code_t *log)
{
	sss_status_t status = kStatus_SSS_Fail;
	scp03_keys_t defaultScp03 = {0};
	puf_key_store_t pufKeyStore = {0};
	uint8_t aesPolicyKey[PUF_AES_POLICY_KEY_SIZE] = {0};

	*log = LOG_EVENT_Scp03KeyRotationFailed;

	assert(base != NULL);
	assert(boot != NULL);
	assert(log != NULL);

	memcpy(defaultScp03.enc_key, defaultScp03PlatformKeyEnc, 16);
	memcpy(defaultScp03.mac_key, defaultScp03PlatformKeyMac, 16);
	memcpy(defaultScp03.dek_key, defaultScp03PlatformKeyDek, 16);

	/* Do not continue if device is decommissioned or AES key is missing! */
	status = QMC2_SE_ReadPolicyAesKey(&defaultScp03, aesPolicyKey);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Device is not commissioned. AES key is missing!\r\n");
		*log = LOG_EVENT_DeviceDecommissioned;
		return status;
	}
	PRINTF("Device is commisioned! Policy AES key is present.\r\n");

	/* Clear pufKeyStore */
	memset((void*) &pufKeyStore, 0, sizeof(puf_key_store_t));

	/* Generate Intrinsic key set for SCP03 */
	status = QMC2_PUF_GeneratePufKeyCodes(base, &pufKeyStore, aesPolicyKey);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: SCP03 key generation failed!\r\n");
		return status;
	}
	PRINTF("SCP03 key generation successful!\r\n");

	/* Program SBL PUF Key Store */
	status = QMC2_PUF_ProgramKeyStore((uint32_t) PUF_SBL_KEY_STORE_ADDRESS, &pufKeyStore);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: SCP03 KeyStore programming failed!\r\n");
		return status;
	}
	PRINTF("SCP03 KeyStore programming successful!\r\n");

	/* Try to reconstruct SCP03 keys from the key codes. */
	status = QMC2_PUF_ReconstructKeys(PUF, &boot->bootKeys);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: SCP03 Key Set reconstruction failed!\r\n");
		return status;
	}
	PRINTF("SCP03 Keys reconstructed successfully!\r\n");

	if(memcmp(aesPolicyKey, boot->bootKeys.scp03.aes_policy_key, PUF_AES_POLICY_KEY_SIZE) != 0)
	{
		PRINTF("\r\nERROR: AES memcmp failed!\r\n");
		memset(&aesPolicyKey, 0, PUF_AES_POLICY_KEY_SIZE);
		return kStatus_SSS_Fail;
	}

	status = QMC2_SE_EraseObj(&defaultScp03, (uint32_t)SE_AES_POLICY_KEY_SBL_ID);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: AES key erasing failed!\r\n");
		return status;
	}
	PRINTF("AES key was erased successfully!\r\n");
	memset(aesPolicyKey, 0, PUF_AES_POLICY_KEY_SIZE);

#ifdef RELEASE_SBL
	/* Rotate new SCP03 KeySet */
	status = QMC2_SE_RotateSCP03Keys(&defaultScp03, &boot->bootKeys.scp03);
	if (status != kStatus_SSS_Success) {
		PRINTF("\r\nERROR: Rotation of SCP03 Keys Failed!\r\n");
		return status;
	}
	PRINTF("Rotation of SCP03 Keys successful!\r\n");

	/* Mandate new SCP03 KeySet */
	status = QMC2_SE_MandateScp03Keys(&boot->bootKeys.scp03);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Mandate of SCP03 Keys Failed!\r\n");
		return status;
	}
	PRINTF("Mandate of SCP03 Keys successful!\r\n");
#endif

	/* Init SNVS LPGPR. */
	status = QMC2_LPGPR_Init(&boot->svnsLpGpr);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: SNVS LP GPR Init Failed!\r\n");
		return status;
	}
	PRINTF("SNVS LP GPR Init successful!\r\n");

	*log = LOG_EVENT_NoLogEntry;
	return status;
}
#endif

#ifdef SECURE_SBL
sss_status_t QMC2_BOOT_Decommissioning(boot_data_t *boot, log_event_code_t *log)
{
	sss_status_t status = kStatus_SSS_Fail;
	scp03_keys_t defaultScp03 = { 0 };

	assert(boot != NULL);
	assert(log != NULL);

	if (BOARD_UserBtnsPressed())
	{
		if (boot->sdCard.isInserted)
		{
			status = QMC2_SD_FATFS_Open(SD_CARD_DECOMMISSIONING_FILE);
			if ((status == kStatus_SSS_Success) && BOARD_UserBtnsPressed())
			{
				(void) QMC2_SD_FATFS_Close();

				*log = LOG_EVENT_DecommissioningFailed;

				PRINTF("Decommissioning has started!\r\n");

				status = QMC2_FLASH_Erase(SBL_BACKUP_IMAGE_ADDRESS, SBL_BACKUP_IMAGE_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Erase of the Backup Image storage failed!\r\n");
					return status;
				}
				PRINTF("Erase of the Backup Image storage successful!\r\n");

				status = QMC2_FLASH_Erase(SBL_CFGDATA_BACKUP_ADDRESS, SBL_CFGDATA_BACKUP_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR:Erase of the Backup CFGDATA storage failed!\r\n");
					return status;
				}
				PRINTF("Erase of the Backup CFGDATA storage successful!\r\n");

				status = QMC2_FLASH_Erase(SBL_MAIN_FW_ADDRESS, SBL_MAIN_FW_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Erase of the Main FW storage failed!\r\n");
					return status;
				}
				PRINTF("Erase of the Main FW storage successful!\r\n");

				status = QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, SBL_FWU_STORAGE_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Erase of the FWU storage failed!\r\n");
					return status;
				}
				PRINTF("Erase of the FWU storage successful!\r\n");

				status = QMC2_FLASH_Erase(SBL_LOG_STORAGE_ADDRESS, SBL_LOG_STORAGE_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Erase of the LOG storage failed!\r\n");
					return status;
				}
				PRINTF("Erase of the LOG storage successful!\r\n");

				status = QMC2_FLASH_Erase(SBL_CFGDATA_ADDRESS, SBL_CFGDATA_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Erase of the CFGDATA failed!\r\n");
					return status;
				}
				PRINTF("Erase of the CFGDATA successful!\r\n");

				memcpy(defaultScp03.enc_key, defaultScp03PlatformKeyEnc, PUF_INTRINSIC_SCP03_KEY_SIZE);
				memcpy(defaultScp03.mac_key, defaultScp03PlatformKeyMac, PUF_INTRINSIC_SCP03_KEY_SIZE);
				memcpy(defaultScp03.dek_key, defaultScp03PlatformKeyDek, PUF_INTRINSIC_SCP03_KEY_SIZE);
				memcpy(defaultScp03.aes_policy_key, boot->bootKeys.scp03.aes_policy_key, PUF_AES_POLICY_KEY_SIZE);

#ifdef RELEASE_SBL
				/* Rotate new SCP03 KeySet */
				status = QMC2_SE_RotateSCP03Keys(&boot->bootKeys.scp03, &defaultScp03);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Rotation of SCP03 Keys to default SCP03 platform keys Failed!\r\n");
					return status;
				}
				PRINTF("Rotation of SCP03 Keys successful!\r\n");

				/* UnMandate new SCP03 KeySet */
				status = QMC2_SE_UnMandateScp03Keys(&defaultScp03);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR:Un Mandate of SCP03 Keys Failed!\r\n");
					return status;
				}
				PRINTF("UnMandate of SCP03 Keys successful!\r\n");
#endif

				status = QMC2_SE_FactoryReset(&defaultScp03);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Factory reset failed!\r\n");
					return status;
				}
				PRINTF("Factory reset successful!\r\n");

				/*Erase PUF KeyStore */
				status = QMC2_FLASH_Erase(PUF_SBL_KEY_STORE_ADDRESS, QSPI_FLASH_ERASE_SECTOR_SIZE);
				if (status != kStatus_SSS_Success) {
					PRINTF("\r\nERROR: Erase of PUF KeyStore failed!\r\n");
					return status;
				}
				PRINTF("Erase of PUF KeyStore successful!\r\n");

				PRINTF("Decommissioning successful!\r\n");

				*log = LOG_EVENT_DeviceDecommissioned;
				// return false to trigger error trap as the device is decommisioned.
				return kStatus_SSS_Fail;
			}
		}
	}

	return kStatus_SSS_Success;
}
#endif

sss_status_t QMC2_BOOT_ProcessMainFwRequest(boot_data_t *boot, log_event_code_t *log)
{
	sss_status_t status = kStatus_SSS_Fail;
	volatile fw_state_t fwState = boot->svnsLpGpr.fwState;

	assert(boot != NULL);
	assert(log != NULL);

	if((fwState == (kFWU_Revert|kFWU_VerifyFw)) || (fwState == (kFWU_Revert|kFWU_VerifyFw|kFWU_AwdtExpired)))
	{
		PRINTF("\r\nRevert!\r\n");
		*log = LOG_EVENT_NewFWRevertFailed;

		// Erase manifeset from FWU storage.
		(void)QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, OCTAL_FLASH_SECTOR_SIZE);

		status = QMC2_FWU_RevertRecoveryImage(&boot->fwHeader);
		if(status != kStatus_SSS_Success)
		{
			return status;
		}
		PRINTF("Recovery Image reverted successfully!\r\n");
		*log = LOG_EVENT_NewFWReverted;
	}
	else if((fwState == (kFWU_Commit|kFWU_VerifyFw)) || (fwState == (kFWU_Commit|kFWU_VerifyFw|kFWU_AwdtExpired)))
	{
		PRINTF("\r\nCommit!\r\n");
		*log = LOG_EVENT_NewFWCommitFailed;
		status = QMC2_FWU_CreateRecoveryImage(&boot->fwHeader);
		if(status != kStatus_SSS_Success)
		{
			return status;
		}
		PRINTF("Recovery image created!\r\n");
		status = QMC2_SE_CommitFwVersionToSe(&boot->bootKeys.scp03, &boot->fwHeader.hdr, &boot->seData.fwVersion);
		if(status != kStatus_SSS_Success)
		{
			return status;
		}
		PRINTF("FW version was successfully committed into SE.!\r\n");

		status = QMC2_FWU_ReadManifest(&boot->fwuManifest, &boot->seData.manVersion, false);
		if (status == kStatus_SSS_Success)
		{
			// Commit new manifest version
			status = QMC2_SE_CommitManVersionToSe(&boot->bootKeys.scp03, &boot->fwuManifest.man, &boot->seData.manVersion);
			if (status != kStatus_SSS_Success)
			{
				return status;
			}
			PRINTF("FWU version was successfully committed into SE.!\r\n");
			// Erase manifest from FWU storage.
			(void)QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, OCTAL_FLASH_SECTOR_SIZE);
		}

		*log = LOG_EVENT_NewFWCommitted;
	}
	else if((fwState == kFWU_BackupCfgData) || (fwState == (kFWU_BackupCfgData|kFWU_AwdtExpired)))
	{
		PRINTF("\r\nBackUp Cfg Data!\r\n");
		*log = LOG_EVENT_CfgDataBackUpFailed;
		status = QMC2_FWU_BackUpCfgData(&boot->fwHeader.hdr);
		if(status != kStatus_SSS_Success)
		{
			return status;
		}
		PRINTF("Configuration Data were Backup-ed!\r\n");
		*log = LOG_EVENT_CfgDataBackedUp;
	}
	else if (fwState == kFWU_AwdtExpired)
	{
		PRINTF("Authenticated WDOG expired!\r\n");
		/* We have to keep fwState to inform the main FW. */
		*log = LOG_EVENT_AwdtExpired;
		(void)QMC2_LOG_CreateLogEntry(log);
		return kStatus_SSS_Success;
	}
	else if((fwState == kFWU_VerifyFw) || (fwState == (kFWU_VerifyFw|kFWU_AwdtExpired)))
	{
		PRINTF("FW verification is requested by SBL!\r\n");
		*log = LOG_EVENT_NoLogEntry;
		return kStatus_SSS_Success;
	}
	else if(fwState == kFWU_NoState)
	{
		PRINTF("No Request from Main FW.\r\n");
		return kStatus_SSS_Success;
	}
	else
	{
		PRINTF("WARNING: Unexpected FW state.\r\n");
		*log = LOG_EVENT_UnknownFWReturnStatus;
		status = kStatus_SSS_Fail;
	}

	(void)QMC2_LOG_CreateLogEntry(log);

	if(boot->svnsLpGpr.fwState & kFWU_AwdtExpired)
	{
		boot->svnsLpGpr.fwState = kFWU_AwdtExpired;
	}
	else
	{
		boot->svnsLpGpr.fwState = kFWU_NoState;
	}

	status = QMC2_LPGPR_Write(&boot->svnsLpGpr);
	if (status != kStatus_SSS_Success)
	{
		*log = LOG_EVENT_SvnsLpGprOpFailed;
		PRINTF("Compared read content isn't consistent.\r\n");
	}

	return status;
}

/* Authenticate the main FW image using SE */
sss_status_t QMC2_BOOT_AuthenticateMainFw(boot_data_t *boot, log_event_code_t *log)
{
	volatile sss_status_t status = kStatus_SSS_Fail;

	assert(boot != NULL);
	assert(log != NULL);

	status = QMC2_FWU_ReadHeader((uint32_t*)SBL_MAIN_FW_ADDRESS, &boot->fwHeader.hdr);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Read of the main FW header failed!\r\n");
		PRINTF("\r\nInstalling recovery image!\r\n");
		status = QMC2_FWU_RevertRecoveryImage(&boot->fwHeader);
		ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, LOG_EVENT_NewFWRevertFailed);

		*log = LOG_EVENT_NewFWReverted;
		(void)QMC2_LOG_CreateLogEntry(log);
	}

	status = kStatus_SSS_Fail;
	status = QMC2_BOOT_Authenticate(boot);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("Invalid FW detected!\r\n");
		if(boot->fwHeader.backupImgActive)
		{
			*log = LOG_EVENT_BackUpImgAuthFailed;
			goto exit;
		}
		else
		{
			status = kStatus_SSS_Fail;
			PRINTF("\r\nInstalling recovery image!\r\n");
			status = QMC2_FWU_RevertRecoveryImage(&boot->fwHeader);
			ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, LOG_EVENT_NewFWRevertFailed);

			*log = LOG_EVENT_NewFWReverted;
			(void)QMC2_LOG_CreateLogEntry(log);

			status = kStatus_SSS_Fail;
			status = QMC2_BOOT_Authenticate(boot);
			ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, LOG_EVENT_BackUpImgAuthFailed);
		}
	}

	return status;

exit:

	(void)QMC2_LOG_CreateLogEntry(log);
	PRINTF("\r\nERROR: Backup image installation failed. Insert SD with valid image and reset the board. \r\n");
	return kStatus_SSS_Fail;
}

static sss_status_t SBL_GetSignatureSize(uint32_t signAddr, uint32_t *signSize)
{
	sign_header_t signHeader;
	uint32_t size = 0;

	assert(signSize != NULL);

	memcpy((void *)&signHeader, (void *)signAddr, sizeof(sign_header_t));

	if(signHeader.data.B.sequenceTag != 0x30)
	{
		return kStatus_SSS_InvalidArgument;
	}

	signHeader.data.B.numOfLengthBytes = signHeader.data.B.numOfLengthBytes & 0x0F;
	size = 2; // sequence tag byte + size byte to overall length

	if(signHeader.data.B.numOfLengthBytes == 1)
	{
		size += signHeader.data.B.Length1;
		size += 1; // to add Length1 byte to overall length
	}
	else
	{
		size += signHeader.data.B.Length1;
		size += 1; // to add Length1 byte to overall length
		size += signHeader.data.B.Length2;
		size += 1; // to add Length2 byte to overall length
	}

	*signSize = size;

	return kStatus_SSS_Success;
}

sss_status_t QMC2_BOOT_Authenticate(boot_data_t *boot)
{
	volatile sss_status_t status = kStatus_SSS_Fail;
	se_verify_sign_t verifySign;

	assert(boot != NULL);

	memset(&verifySign, 0, sizeof(se_verify_sign_t));
	if (boot->fwHeader.hdr.version >= boot->seData.fwVersion)
	{
		verifySign.fwDataAddr 		= boot->fwHeader.hdr.fwDataAddr;
		verifySign.fwDataLength 	= boot->fwHeader.hdr.fwDataLength;
		verifySign.signDataAddr 	= boot->fwHeader.hdr.signDataAddr;

		status = SBL_GetSignatureSize(verifySign.signDataAddr, &verifySign.signDataLength);
		if (status != kStatus_SSS_Success)
		{
			PRINTF("\r\nERROR: Invalid FW signature header!\r\n.");
			goto cleanup;
		}

		status = kStatus_SSS_Fail;

		verifySign.keyId			= SE_FW_PUB_KEY_ID;
		verifySign.mode  			= kMode_SSS_Verify;
		verifySign.algorithm		= kAlgorithm_SSS_SHA512;
		verifySign.cipherType		= kSSS_CipherType_EC_BRAINPOOL;
		verifySign.keyPart			= kSSS_KeyPart_Public;

		status = kStatus_SSS_Fail;
		status = QMC2_SE_VerifySignature(&boot->bootKeys.scp03, &verifySign);
		if (status != (kStatus_SSS_Success + 0x0a0a))
		{
			PRINTF("\r\nERROR: FW image authentication failed!\r\n.");
			goto cleanup;
		}
	}
	else
	{
		PRINTF("\r\nERROR: Invalid FW version!\r\n");
	}

cleanup:

	memset(&verifySign, 0, sizeof(se_verify_sign_t));
	return (status - 0x0a0a);
}

/* Authenticate the FW Update image using SE */
sss_status_t QMC2_BOOT_AuthenticateFwu(boot_data_t *boot)
{
	volatile sss_status_t status = kStatus_SSS_Fail;
	se_verify_sign_t verifySign;

	assert(boot != NULL);

	memset(&verifySign, 0, sizeof(se_verify_sign_t));
	if (boot->fwuManifest.man.version > boot->seData.manVersion)
	{
		verifySign.fwDataAddr 		= boot->fwuManifest.man.fwuDataAddr;
		verifySign.fwDataLength 	= boot->fwuManifest.man.fwuDataLength;
		verifySign.signDataAddr 	= boot->fwuManifest.man.signDataAddr;

		status = SBL_GetSignatureSize(verifySign.signDataAddr, &verifySign.signDataLength);
		if (status != kStatus_SSS_Success)
		{
			PRINTF("\r\nERROR: Invalid FWU signature header!\r\n.");
			goto cleanup;
		}

		status = kStatus_SSS_Fail;

		verifySign.keyId			= SE_FWU_PUB_KEY_ID;
		verifySign.mode  			= kMode_SSS_Verify;
		verifySign.algorithm		= kAlgorithm_SSS_SHA512;
		verifySign.cipherType		= kSSS_CipherType_EC_BRAINPOOL;
		verifySign.keyPart			= kSSS_KeyPart_Public;

		status = kStatus_SSS_Fail;
		status = QMC2_SE_VerifySignature(&boot->bootKeys.scp03 ,&verifySign);

		if (status != (kStatus_SSS_Success + 0x0a0a))
		{
			PRINTF("\r\nERROR: FW image authentication failed!\r\n.");
			goto cleanup;
		}
	}
	else
	{
		PRINTF("\r\nERROR: Invalid FWU version!\r\n");
		goto cleanup;
	}

	return (status - 0x0a0a);

cleanup:

	(void)QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, boot->fwuManifest.man.fwuDataLength + SEC_SIGNATURE_BLOCK_SIZE);
	memset(&boot->fwuManifest, 0, sizeof(fwu_manifest_t));
	memset(&verifySign, 0, sizeof(se_verify_sign_t));

	return kStatus_SSS_Fail;
}

/* Authenticate the main FW image using SE */
sss_status_t QMC2_BOOT_FwUpdate(boot_data_t *boot, log_event_code_t *log)
{
	volatile sss_status_t status = kStatus_SSS_Fail;

	assert(boot != NULL);
	assert(log != NULL);

	if (boot->fwuManifest.isFwuImgOnSdCard)
	{
		PRINTF("\r\n**********************************\r\n");
		PRINTF("\r\nCopy data from SDcard to STORAGE.!\r\n");
		PRINTF("\r\n**********************************\r\n");
		status = QMC2_FWU_SdToFwuStorage(boot, log);
		if (status != kStatus_SSS_Success)
		{
			PRINTF("\r\nERROR: Programming FWU image from SD into FWU storage failed.!\r\n");
			return status;
		}
	}

	status = kStatus_SSS_Fail;
	status = QMC2_BOOT_AuthenticateFwu(boot);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: FWU Authentication failed.!\r\n");
		*log = LOG_EVENT_FwuAuthFailed;
		return status;
	}

	status = kStatus_SSS_Fail;
	status = QMC2_FWU_Program(boot, log);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: FW Update programming failed.!\r\n");
		return status;
	}

	return status;
}

/*
 * @brief Gets called when an error occurs.
 *
 * @details Print error message and trap forever.
 */
void QMC2_BOOT_ErrorTrap(log_event_code_t *log)
{

	assert(log != NULL);

	// log the event via data logger
	(void)QMC2_LOG_CreateLogEntry(log);

	/*
		LOG_EVENT_HwInitDeinitFailed		0,0,0,1
		LOG_EVENT_SdCardFailed 				0,0,1,0
		LOG_EVENT_svnsLpGprOpFailed			0,0,1,1
		LOG_EVENT_Scp03KeyRotationFailed	0,1,0,0
		LOG_EVENT_Scp03KeyReconFailed		0,1,0,1
		LOG_EVENT_Scp03ConnFailed			0,1,1,0
		LOG_EVENT_VerReadFromSeFailed		0,1,1,1
		LOG_EVENT_ExtMemOprFailed			1,0,0,0
		LOG_EVENT_FwExecutionFailed			1,0,0,1
		LOG_EVENT_NewFWRevertFailed			1,0,1,0
		LOG_EVENT_NewFWCommitFailed			1,0,1,1
		LOG_EVENT_CfgDataBackupFailed		1,1,0,0
		LOG_EVENT_BackUpImgAuthFailed		1,1,0,1
		LOG_EVENT_DecomissioningFailed		1,1,1,0
		LOG_EVENT_DeviceDecommisioned 		1,1,1,1
	*/


		switch(*log)
		{
			case LOG_EVENT_HwInitDeinitFailed:
				BOARD_ShowErrorOnLeds(0,0,0,1);
				break;
			case LOG_EVENT_SdCardFailed:
				BOARD_ShowErrorOnLeds(0,0,1,0);
				break;
			case LOG_EVENT_SvnsLpGprOpFailed:
				BOARD_ShowErrorOnLeds(0,0,1,1);
				break;
			case LOG_EVENT_Scp03KeyRotationFailed:
				BOARD_ShowErrorOnLeds(0,1,0,0);
				break;
			case LOG_EVENT_Scp03KeyReconFailed:
				BOARD_ShowErrorOnLeds(0,1,0,1);
				break;
			case LOG_EVENT_Scp03ConnFailed:
				BOARD_ShowErrorOnLeds(0,1,1,0);
				break;
			case LOG_EVENT_VerReadFromSeFailed:
				BOARD_ShowErrorOnLeds(0,1,1,1);
				break;
			case LOG_EVENT_ExtMemOprFailed:
				BOARD_ShowErrorOnLeds(1,0,0,0);
				break;
			case LOG_EVENT_FwExecutionFailed:
				BOARD_ShowErrorOnLeds(1,0,0,1);
				break;
			case LOG_EVENT_NewFWRevertFailed:
				BOARD_ShowErrorOnLeds(1,0,1,0);
				break;
			case LOG_EVENT_NewFWCommitFailed:
				BOARD_ShowErrorOnLeds(1,0,1,1);
				break;
			case LOG_EVENT_CfgDataBackUpFailed:
				BOARD_ShowErrorOnLeds(1,1,0,0);
				break;
			case LOG_EVENT_BackUpImgAuthFailed:
				BOARD_ShowErrorOnLeds(1,1,0,1);
				break;
			case LOG_EVENT_DecommissioningFailed:
				BOARD_ShowErrorOnLeds(1,1,1,0);
				break;
			case LOG_EVENT_DeviceDecommissioned:
				BOARD_ShowErrorOnLeds(1,1,1,1);
				break;
			default:
				break;
		}


    while (1) {}
}

sss_status_t QMC2_BOOT_InitLogKeys(log_keys_t *logKeys)
{
	// Rework assert to return error in release target or logs as well
	assert(logKeys != NULL);

#if NON_SECURE_SBL

	uint8_t aesLogKey[] = AES_LOG_KEY;
	uint8_t aesLogNonce[] = AES_LOG_NONCE;

	(void)memcpy((void *)(aesLogKey), (void *)(logKeys->aesKeyConfig), sizeof(logKeys->aesKeyConfig));
	(void)memcpy((void *)(aesLogNonce), (void *)(logKeys->nonceConfig), sizeof(logKeys->nonceConfig));
	(void)memcpy((void *)(aesLogKey), (void *)(logKeys->aesKeyLog), sizeof(logKeys->aesKeyLog));
	(void)memcpy((void *)(aesLogNonce), (void *)(logKeys->nonceLog), sizeof(logKeys->nonceLog));

#endif

    (void)memcpy((void *)(LOG_AES_KEY_NONCE_ADDRESS), (void *)(logKeys), sizeof(log_keys_t));

    if(memcmp((void *)LOG_AES_KEY_NONCE_ADDRESS, (void *)(logKeys), sizeof(log_keys_t)) != 0)
    {
    	 PRINTF("\r\nERROR: Log Key an Nonce Init failed!\r\n");
         return kStatus_SSS_Fail;
    }

	return kStatus_SSS_Success;
}

sss_status_t QMC2_BOOT_PassScp03Keys(scp03_keys_t *scp03)
{
	assert(scp03 != NULL);

	memcpy((void *)MAIN_FW_SCP03_KEYS_ADDRESS, (void *)scp03, (sizeof(scp03_keys_t) - PUF_AES_POLICY_KEY_SIZE));

	if(memcmp((void *)(MAIN_FW_SCP03_KEYS_ADDRESS), (void *)(scp03), (sizeof(scp03_keys_t) - PUF_AES_POLICY_KEY_SIZE)) != 0)
	{
		PRINTF("\r\nERROR: SCP03 keys main FW init failed!\r\n");
		return kStatus_SSS_Fail;
	}

	return kStatus_SSS_Success;
}

sss_status_t QMC2_BOOT_RPC_InitSecureWatchdog(scp03_keys_t *scp03)
{
	sss_status_t status = kStatus_SSS_Fail;
	rpc_shm_t gs_rpcSHM = RPC_SHM_STATIC_INIT;

#ifndef NON_SECURE_SBL
	uint8_t seed[RPC_SECWD_MAX_RNG_SEED_SIZE] = {0};
	uint8_t key[RPC_SECWD_MAX_PK_SIZE] = {0};
#else
	uint8_t seed[RPC_SECWD_MAX_RNG_SEED_SIZE] = QMC_CM4_TEST_AWDG_RNG_SEED;
	uint8_t key[RPC_SECWD_MAX_PK_SIZE] = QMC_CM4_TEST_AWDG_PK;
#endif

	assert(scp03 != NULL);

	memcpy((void *)RPC_SHM_INIT_DATA_ADDRESS, &gs_rpcSHM, sizeof(rpc_shm_t));
#ifndef NON_SECURE_SBL
	status = QMC2_SE_GetRpcSeedAndKey(scp03, seed, (size_t) RPC_SECWD_MAX_RNG_SEED_SIZE, key, (size_t) RPC_SECWD_MAX_PK_SIZE);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Getting random data an key failed!\r\n");
		goto cleanup;
	}
#endif
	status = RPC_InitSecureWatchdog(seed, (size_t)RPC_SECWD_MAX_RNG_SEED_SIZE, key,  (size_t)RPC_SECWD_MAX_PK_SIZE);

#ifndef NON_SECURE_SBL
cleanup:
#endif

	memset(seed, 0x00, RPC_SECWD_MAX_RNG_SEED_SIZE);
	memset(key, 0x00, RPC_SECWD_MAX_PK_SIZE);

	return status;
}



void  QMC2_BOOT_InitSrtc(void)
{
    /* setup low-power SNVS */
    SNVS_LP_SRTC_Init(SNVS, &kSnvsLpConfig);

    /* SRTC already running? */
    if (SNVS_LPCR_SRTC_ENV(1U) != (SNVS->LPCR & SNVS_LPCR_SRTC_ENV(1U)))
    {
        /* reset SRTC counts */
        SNVS->LPSRTCMR = 0x0U;
        SNVS->LPSRTCLR = 0x0U;
        /* start the SRTC */
        SNVS_LP_SRTC_StartTimer(SNVS);
    }
    /* lock write access to SRTC, can not replace isolation from CM7! */
    SNVS->LPLR |= SNVS_LPLR_SRTC_HL(1U);
    SNVS->LPLR |= SNVS_LPLR_LPCALB_HL(1U);
}


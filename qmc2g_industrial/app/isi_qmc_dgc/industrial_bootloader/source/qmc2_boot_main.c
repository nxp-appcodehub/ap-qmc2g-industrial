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
 * @file    main_cm7.c
 * @brief   Application entry point.
 */
#include "qmc2_boot.h"
#include "resource_config.h"
#include "fsl_rdc.h"
#include "board.h"
#include "verify_config.h"
#include <ex_sss_auth.h>
#include <qmc2_se.h>
#include <qmc2_flash.h>
#include <qmc2_log.h>
#include <qmc2_lpgpr.h>
#include <qmc2_sd_fatfs.h>
#include <qmc2_puf.h>
#include <qmc2_fwu.h>
#include <init_rpc.h>

#include "fsl_snvs_lp.h"
#include "datalogger_tasks.h"

#ifndef RELEASE_SBL
static uint8_t defaultScp03PlatformKeyEnc[] = EX_SSS_AUTH_SE05X_KEY_ENC;
static uint8_t defaultScp03PlatformKeyMac[] = EX_SSS_AUTH_SE05X_KEY_MAC;
static uint8_t defaultScp03PlatformKeyDek[] = EX_SSS_AUTH_SE05X_KEY_DEK;
#endif

extern bool isFlashDriverInitialized;
extern bool isLogKeyCtrAvailable;
extern bool isDataLoggerInitialized;

uint8_t rdcSha512Sbl[] = RDC_SHA512_SBL;
/*
 * @brief   Application entry point.
 */
int QMC2_BOOT_Main(void)
{
	volatile sss_status_t status = kStatus_SSS_Fail;
	boot_data_t boot	= {0};
	log_event_code_t log 	= LOG_EVENT_HwInitDeinitFailed;

    SNVS_LP_Init(SNVS);
    QMC2_BOOT_InitSrtc();

    status = kStatus_SSS_Fail;
    status = QMC2_FLASH_DriverInit();
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

    isFlashDriverInitialized = true;

    status = kStatus_SSS_Fail;
	status = Verify_Rdc_Config(rdcSha512Sbl);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

#ifdef SECURE_SBL
    status = kStatus_SSS_Fail;
    status = QMC2_PUF_Init();
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

    memset((void *)&boot, 0, sizeof(boot_data_t));

    status = kStatus_SSS_Fail;
	status = QMC2_SD_FATFS_Init(&boot.sdCard.isInserted);
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_SdCardFailed);
#endif

#ifdef SECURE_SBL
	/* Initialization will be done for the first boot where no PUF keystore is generated. */
	if((*((uint32_t*)PUF_SBL_KEY_STORE_ADDRESS) == 0xFFFFFFFF))
	{
		status = kStatus_SSS_Fail;
		status = QMC2_BOOT_RotateMandateSCP03Keys(PUF, &boot, &log);
		ENSURE_OR_EXIT((status == kStatus_SSS_Success));
    }

	status = kStatus_SSS_Fail;
	/* Read SNVS register. */
	status = QMC2_LPGPR_Read(&boot.svnsLpGpr);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_SvnsLpGprOpFailed);

	status = kStatus_SSS_Fail;
	/* Try to reconstruct SCP03 keys from the key codes. */
	status = QMC2_PUF_ReconstructKeys(PUF, &boot.bootKeys);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_Scp03KeyReconFailed);

	isLogKeyCtrAvailable = true;

	status = kStatus_SSS_Fail;
	/* Datalogger initialization. */
	status = DataloggerInit();
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);
	isDataLoggerInitialized = true;
#endif

#ifndef RELEASE_SBL
	memcpy(boot.bootKeys.scp03.enc_key, defaultScp03PlatformKeyEnc, PUF_INTRINSIC_SCP03_KEY_SIZE);
	memcpy(boot.bootKeys.scp03.mac_key, defaultScp03PlatformKeyMac, PUF_INTRINSIC_SCP03_KEY_SIZE);
	memcpy(boot.bootKeys.scp03.dek_key, defaultScp03PlatformKeyDek, PUF_INTRINSIC_SCP03_KEY_SIZE);
#endif

#ifdef SECURE_SBL
	status = kStatus_SSS_Fail;
	status = Verify_Rdc_Config(rdcSha512Sbl);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

	status = kStatus_SSS_Fail;
    status = QMC2_SE_CheckScp03Conn(&boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_Scp03ConnFailed);

	status = kStatus_SSS_Fail;
	status = QMC2_BOOT_Decommissioning(&boot, &log);
	ENSURE_OR_EXIT((status == kStatus_SSS_Success));

	status = kStatus_SSS_Fail;
	status = QMC2_SE_ReadVersionsFromSe(&boot.seData, &boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_VerReadFromSeFailed);

	status = kStatus_SSS_Fail;
	status = QMC2_BOOT_ProcessMainFwRequest(&boot, &log);
	ENSURE_OR_EXIT((status == kStatus_SSS_Success));

	if((boot.svnsLpGpr.fwState == kFWU_NoState) || (boot.svnsLpGpr.fwState == kFWU_AwdtExpired))
	{
		status = kStatus_SSS_Fail;
		/* Read FW Update Manifest. */
		status = QMC2_FWU_ReadManifest(&boot.fwuManifest, &boot.seData.manVersion, boot.sdCard.isInserted);
		if (status == kStatus_SSS_Success)
		{
			PRINTF("New FW update detected!\r\n");
			/* FW Update */
			status = kStatus_SSS_Fail;
			status = QMC2_BOOT_FwUpdate(&boot, &log);
			if (status != kStatus_SSS_Success)
			{
				PRINTF("\r\nERROR: FW Update failed!\r\n");
				(void)QMC2_LOG_CreateLogEntry(&log);
			}
			else
			{
				PRINTF("FW Update completed!\r\n");
			}
		}
	}

	status = kStatus_SSS_Fail;
	/* Authentication  main FW using Secure Element */
	status = QMC2_BOOT_AuthenticateMainFw(&boot, &log);
	ENSURE_OR_EXIT((status == kStatus_SSS_Success));
#endif

	status = kStatus_SSS_Fail;
	/* Authentication main FW using Secure Element */
	status = QMC2_BOOT_RPC_InitSecureWatchdog(&boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_RpcInitFailed);

	status = kStatus_SSS_Fail;
	/* Initialize keys for Logging feature */
	status = QMC2_BOOT_InitLogKeys(&boot.bootKeys.log);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

	status = kStatus_SSS_Fail;
	/* Pass SCP03 keys to the main application */
	status = QMC2_BOOT_PassScp03Keys(&boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

	status = kStatus_SSS_Fail;
	/* SBL cleanup function */
    status = QMC2_BOOT_Cleanup(&boot);
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

#ifdef NON_SECURE_SBL
    status = kStatus_SSS_Fail;
    status = QMC2_FWU_ReadHeader((uint32_t*)SBL_MAIN_FW_ADDRESS, &boot.fwHeader.hdr);
    ENSURE_OR_EXIT((status == kStatus_SSS_Success));
#endif

    status = kStatus_SSS_Fail;
	status = Verify_Rdc_Config(rdcSha512Sbl);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_HwInitDeinitFailed);

    status = kStatus_SSS_Fail;
	/* Load CM4 image and execute then execute CM7 main FW. */
    status = QMC2_BOOT_ExecuteCm4Cm7Fws(&boot.fwHeader);
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, LOG_EVENT_FwExecutionFailed);

exit:

	(void)memset((void *)RPC_SHM_INIT_DATA_ADDRESS,0, sizeof(rpc_shm_t));
	(void)memset((void *)RPC_SECWD_INIT_DATA_ADDRESS,0, sizeof(rpc_secwd_init_data_t));
	(void)memset((void *)(LOG_AES_KEY_NONCE_ADDRESS), 0, sizeof(log_keys_t));
	(void)memset((void *)(LOG_AES_KEY_NONCE_ADDRESS), 0, sizeof(log_keys_t));

	if(log == LOG_EVENT_FwExecutionFailed && !(boot.fwHeader.backupImgActive))
	{
		boot.svnsLpGpr.fwState |= (kFWU_Revert|kFWU_VerifyFw);
		if(QMC2_LPGPR_Write(&boot.svnsLpGpr) == kStatus_SSS_Success)
		{
			NVIC_SystemReset();
		}
	}

	if(log == LOG_EVENT_NewFWRevertFailed)
	{

		if(boot.svnsLpGpr.fwState & kFWU_AwdtExpired)
		{
			boot.svnsLpGpr.fwState = kFWU_AwdtExpired;
		}
		else
		{
			boot.svnsLpGpr.fwState = kFWU_NoState;
		}

		(void)QMC2_LPGPR_Write(&boot.svnsLpGpr);

		PRINTF("\r\nRevert of the backup image failed. Insert SD card with new FW and press reset!\r\n");
	}

	(void)memset(&boot, 0, sizeof(boot_data_t));

	QMC2_BOOT_ErrorTrap(&log);

    return 0;
}


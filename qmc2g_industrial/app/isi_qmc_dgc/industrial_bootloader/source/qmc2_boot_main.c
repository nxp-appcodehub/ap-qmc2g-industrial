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

#ifndef RELEASE_SBL
static uint8_t defaultScp03PlatformKeyEnc[] = EX_SSS_AUTH_SE05X_KEY_ENC;
static uint8_t defaultScp03PlatformKeyMac[] = EX_SSS_AUTH_SE05X_KEY_MAC;
static uint8_t defaultScp03PlatformKeyDek[] = EX_SSS_AUTH_SE05X_KEY_DEK;
#endif
/*
 * @brief   Application entry point.
 */
int QMC2_BOOT_Main(void)
{
	sss_status_t status = kStatus_SSS_Fail;
	boot_data_t boot	= {0};
	log_entry_t log 	= kLOG_HwInitDeinitFailed;

    SNVS_LP_Init(SNVS);
    QMC2_BOOT_InitSrtc();

    status = QMC2_FLASH_DriverInit();
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_HwInitDeinitFailed);

#ifdef SECURE_SBL
    status = QMC2_PUF_Init();
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_HwInitDeinitFailed);

    memset((void *)&boot, 0, sizeof(boot_data_t));

	status = QMC2_SD_FATFS_Init(&boot.sdCard.isInserted);
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_SdCardFailed);
#else
	SNVS->LPGPR[0] = 0;
	SNVS->LPGPR[1] = 0;
	SNVS->LPGPR[2] = 0;
	SNVS->LPGPR[3] = 0;
#endif

#ifdef SECURE_SBL
	/* Initialization will be done for the first boot where no PUF keystore is generated. */
	if((*((uint32_t*)PUF_SBL_KEY_STORE_ADDRESS) == 0xFFFFFFFF))
	{
		status = QMC2_BOOT_RotateMandateSCP03Keys(PUF, &boot, &log);
		ENSURE_OR_EXIT((status == kStatus_SSS_Success));
    }

	/* Read SNVS register. */
	status = QMC2_LPGPR_Read(&boot.svnsLpGpr);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_SvnsLpGprOpFailed);

	/* Try to reconstruct SCP03 keys from the key codes. */
	status = QMC2_PUF_ReconstructKeys(PUF, &boot.bootKeys);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_Scp03KeyReconFailed);

#ifndef RELEASE_SBL
	memcpy(boot.bootKeys.scp03.enc_key, defaultScp03PlatformKeyEnc, 16);
	memcpy(boot.bootKeys.scp03.mac_key, defaultScp03PlatformKeyMac, 16);
	memcpy(boot.bootKeys.scp03.dek_key, defaultScp03PlatformKeyDek, 16);
#endif

    status = QMC2_SE_CheckScp03Conn(&boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_Scp03ConnFailed);

	status = QMC2_BOOT_Decommissioning(&boot, &log);
	ENSURE_OR_EXIT((status == kStatus_SSS_Success));

	status = QMC2_SE_ReadVersionsFromSe(&boot.seData, &boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_VerReadFromSeFailed);

	status = QMC2_BOOT_ProcessMainFwRequest(&boot, &log);
	ENSURE_OR_EXIT((status == kStatus_SSS_Success));

	if(boot.svnsLpGpr.fwState == kFWU_NoState)
	{
		/* Read FW Update Manifest. */
		status = QMC2_FWU_ReadManifest(&boot.fwuManifest, &boot.seData.manVersion, boot.sdCard.isInserted);
		if (status == kStatus_SSS_Success)
		{
			PRINTF("New FW update detected!\r\n");
			/* FW Update */
			status = QMC2_BOOT_FwUpdate(&boot, &log);
			if (status != kStatus_SSS_Success)
			{
				PRINTF("\r\nERROR: FW Update failed!\r\n");
				QMC2_LOG_CreateLogEntry(&log);
			}
			else
			{
				PRINTF("FW Update completed!\r\n");
			}
		}
	}

	/* Authentication  main FW using Secure Element */
	status = QMC2_BOOT_AuthenticateMainFw(&boot, &log);
	ENSURE_OR_EXIT((status == kStatus_SSS_Success));
#endif

	/* Authentication main FW using Secure Element */
	status = QMC2_BOOT_RPC_InitSecureWatchdog(&boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_RpcInitFailed);

	/* Initialize keys for Logging feature */
	status = QMC2_BOOT_InitLogKeys(&boot.bootKeys.log);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_HwInitDeinitFailed);

	/* Pass SCP03 keys to the main application */
	status = QMC2_BOOT_PassScp03Keys(&boot.bootKeys.scp03);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_HwInitDeinitFailed);

	/* SBL cleanup function */
    status = QMC2_BOOT_Cleanup(&boot);
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_HwInitDeinitFailed);

#ifdef NON_SECURE_SBL
    status = QMC2_FWU_ReadHeader((uint32_t*)SBL_MAIN_FW_ADDRESS, &boot.fwHeader.hdr);
    ENSURE_OR_EXIT((status == kStatus_SSS_Success));
#endif

	/* Load CM4 image and execute then execute CM7 main FW. */
    status = QMC2_BOOT_ExecuteCm4Cm7Fws(&boot.fwHeader);
    ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), log, kLOG_FwExecutionFailed);

exit:

	(void)memset((void *)RPC_SHM_INIT_DATA_ADDRESS,0, sizeof(rpc_shm_t));
	(void)memset((void *)RPC_SECWD_INIT_DATA_ADDRESS,0, sizeof(rpc_secwd_init_data_t));
	(void)memset((void *)(LOG_AES_KEY_NONCE_ADDRESS), 0, sizeof(log_keys_t));
	(void)memset((void *)(LOG_AES_KEY_NONCE_ADDRESS), 0, sizeof(log_keys_t));

	if(log == kLOG_FwExecutionFailed && !(boot.fwHeader.backupImgActive))
	{
		boot.svnsLpGpr.fwState = (kFWU_Revert|kFWU_VerifyFw);
		if(QMC2_LPGPR_Write(&boot.svnsLpGpr) == kStatus_SSS_Success)
		{
			NVIC_SystemReset();
		}
	}

	(void)memset(&boot, 0, sizeof(boot_data_t));
	QMC2_BOOT_ErrorTrap(&log);
    return 0;
}


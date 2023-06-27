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
 * @file    sbl_fwu.c
 * @brief   Firmware Update API.
 */

#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include <qmc2_se.h>
#include <qmc2_lpgpr.h>
#include <qmc2_fwu.h>
#include <qmc2_sd_fatfs.h>

/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/
/* Program FWU image from SD to FWU storage. */
sss_status_t QMC2_FWU_SdToFwuStorage(boot_data_t *boot, log_entry_t *log)
 {
	sss_status_t status = kStatus_SSS_Fail;
	uint32_t offset = FW_HEADER_BLOCK_SIZE;
	uint32_t length = 0;
	uint8_t buffer[PGM_PAGE_SIZE] = {0};
	uint32_t pgmAlignedSize = 0;
	uint32_t unalignedChunk = 0;

	assert(boot != NULL);
	assert(log != NULL);

	/* Aling the size to pgm page */
	unalignedChunk = boot->fwuManifest.man.fwuDataLength % PGM_PAGE_SIZE;

	if(unalignedChunk)
	{
		pgmAlignedSize = boot->fwuManifest.man.fwuDataLength + (PGM_PAGE_SIZE - (unalignedChunk));
	}
	else
	{
		pgmAlignedSize = boot->fwuManifest.man.fwuDataLength;
	}

	/* Check if alinged size fits into storage */
	if(pgmAlignedSize > SBL_FWU_STORAGE_SIZE)
	{
		(void)QMC2_SD_FATFS_Delete(SD_CARD_FW_UPDATE_FILE);
		return kStatus_SSS_Fail;
	}

	status = QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, pgmAlignedSize);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_ExtMemOprFailed);

	status = QMC2_SD_FATFS_Open(SD_CARD_FW_UPDATE_FILE);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SdCardFailed);

	length = boot->fwuManifest.man.fwuDataLength - unalignedChunk;
	while(length)
	{
		status = QMC2_SD_FATFS_Read(buffer, PGM_PAGE_SIZE, offset);
		ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SdCardFailed);

		status = QMC2_FLASH_ProgramPages(SBL_FWU_STORAGE_ADDRESS+offset,  buffer,  PGM_PAGE_SIZE);
		ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_ExtMemOprFailed);

		length -= PGM_PAGE_SIZE;
		offset += PGM_PAGE_SIZE;

		memset(buffer, 0xff, PGM_PAGE_SIZE);
	}

	if (unalignedChunk)
	{
		status = QMC2_SD_FATFS_Read(buffer, unalignedChunk, offset);
		ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SdCardFailed);

		status = QMC2_FLASH_ProgramPages(SBL_FWU_STORAGE_ADDRESS+offset,  buffer,  unalignedChunk);
		ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_ExtMemOprFailed);
	}

	status = QMC2_SD_FATFS_Read(buffer, FW_HEADER_BLOCK_SIZE, 0);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SdCardFailed);

	status = QMC2_FLASH_ProgramPages(SBL_FWU_STORAGE_ADDRESS,  buffer,  FW_HEADER_BLOCK_SIZE);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_ExtMemOprFailed);

	status = QMC2_SD_FATFS_Delete(SD_CARD_FW_UPDATE_FILE);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SdCardFailed);

	status = QMC2_SD_FATFS_Close();
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SdCardFailed);

	/* Read manifest again from FWU storage. */
	status = QMC2_FWU_ReadManifest(&boot->fwuManifest, &boot->seData.manVersion, false);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_InvalidFwuVersion);

	return status;

exit:

	(void)QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, pgmAlignedSize);
	(void)QMC2_SD_FATFS_Delete(SD_CARD_FW_UPDATE_FILE);

	memset(&boot->fwuManifest, 0, sizeof(fwu_manifest_t));

	return status;
}

/* Program FW update. */
sss_status_t QMC2_FWU_Program(boot_data_t *boot, log_entry_t *log)
 {
	sss_status_t status = kStatus_SSS_Fail;
	uint32_t pgmAlignedSize = 0;

	assert(boot != NULL);
	assert(log != NULL);

	/* Aling the size to pgm page */
	if(boot->fwuManifest.man.fwDataLength % PGM_PAGE_SIZE)
	{
		pgmAlignedSize = boot->fwuManifest.man.fwDataLength + (PGM_PAGE_SIZE - (boot->fwuManifest.man.fwDataLength % PGM_PAGE_SIZE));
	}
	else
	{
		pgmAlignedSize = boot->fwuManifest.man.fwDataLength;
	}

	/* Check if alinged size fits into storage */
	if(pgmAlignedSize > SBL_MAIN_FW_SIZE)
	{
		(void)QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, pgmAlignedSize);
		return kStatus_SSS_Fail;
	}

	status = QMC2_FLASH_Erase(SBL_MAIN_FW_ADDRESS, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the Main FW location failed.!\r\n");
		return status;
	}

	/* Program FW image onto main FW adddress. */
	status = QMC2_FLASH_ProgramPages(SBL_MAIN_FW_ADDRESS, (const void *)boot->fwuManifest.man.fwDataAddr, pgmAlignedSize);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_ExtMemOprFailed);

	status = QMC2_FLASH_encXipMemcmp(SBL_MAIN_FW_ADDRESS, boot->fwuManifest.man.fwDataAddr, pgmAlignedSize);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_ExtMemOprFailed);

	/* Set status to force the new main FW to do selfcheck. */
	//TODO double check if there can be some state which cannot be overwritten
	boot->svnsLpGpr.fwState = kFWU_VerifyFw;
	status = QMC2_LPGPR_Write(&boot->svnsLpGpr);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_SvnsLpGprOpFailed);

	// TODO double check this behaviour
	/* Commit the manifest version to prevent re-instalation. */
	status = QMC2_SE_CommitManVersionToSe(&boot->bootKeys.scp03, &boot->fwuManifest.man, &boot->seData.manVersion);
	ENSURE_OR_EXIT_WITH_LOG((status == kStatus_SSS_Success), *log, kLOG_FwuCommitFailed);

	status = QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, pgmAlignedSize);
	if (status != kStatus_SSS_Success) {
		PRINTF("\r\nERROR: of the FWU storage failed.!\r\n");
		*log = kLOG_ExtMemOprFailed;
	}

	return status;

exit:

	(void)QMC2_FLASH_Erase(SBL_MAIN_FW_ADDRESS, pgmAlignedSize);
	(void)QMC2_FLASH_Erase(SBL_FWU_STORAGE_ADDRESS, pgmAlignedSize);

	return kStatus_SSS_Fail;
}

/* Read FW Update headre. */
sss_status_t QMC2_FWU_ReadHeader(uint32_t *address, header_t *fwHeader)
{
	assert(address != NULL);
	assert(fwHeader != NULL);

	if(*address == (uint32_t)MAIN_FW_HEADER_MAGIC)
	{

		memcpy((void *)fwHeader, (void *)address, sizeof(header_t));

	    if (memcmp((void *)fwHeader, (void *)address, sizeof(header_t)) != 0)
	    {
	         return kStatus_SSS_Fail;
	    }

	    /* Check boundaries to makes sure that data which going to be executed are inside signature verification range. */
	    if( ((fwHeader->cm7VectorTableAddr) > (fwHeader->fwDataAddr)) && ((fwHeader->cm7VectorTableAddr) < (fwHeader->fwDataAddr + fwHeader->fwDataLength)) &&
	      (((fwHeader->cm4FwDataAddr) > (fwHeader->fwDataAddr)) && ((fwHeader->cm4FwDataAddr + fwHeader->cm4FwDataLength) < (fwHeader->fwDataAddr + fwHeader->fwDataLength)))	)
	    	return kStatus_SSS_Success;
	}

	return kStatus_SSS_Fail;
}

/* Read FW Update Manifest. */
sss_status_t QMC2_FWU_ReadManifest(fwu_manifest_t *fwuManifest, uint32_t *manVerInSe, bool isSdCardInserted)
{
	sss_status_t status = kStatus_SSS_Fail;
	uint32_t *manifest_loc = ((uint32_t *)SBL_FWU_STORAGE_ADDRESS); //TODO
	manifest_t manOnSdCard = {0};
	manifest_t manOnExtFlash = {0};

	assert(fwuManifest != NULL);
	assert(manVerInSe != NULL);

	memset(fwuManifest, 0, sizeof(fwu_manifest_t));

	fwuManifest->isFwuImgOnSdCard = false;
	if(isSdCardInserted)
	{
		status = QMC2_SD_FATFS_Open(SD_CARD_FW_UPDATE_FILE);
		if (status == kStatus_SSS_Success)
		{
			(void)QMC2_SD_FATFS_Read((uint8_t *)&manOnSdCard, (uint32_t)sizeof(manOnSdCard), 0);
			(void)QMC2_SD_FATFS_Close();

			if((manOnSdCard.vendorID == (uint32_t)MAIN_FW_UPDATE_VENDOR_ID))
			{
				fwuManifest->isFwuImgOnSdCard = true;
			}
			else
			{
				(void)QMC2_SD_FATFS_Delete(SD_CARD_FW_UPDATE_FILE);
			}
		}
	}

	if(fwuManifest->isFwuImgOnSdCard)
	{
		if(*manifest_loc == (uint32_t)MAIN_FW_UPDATE_VENDOR_ID)
		{
			memcpy((void *) &manOnExtFlash, (void *) manifest_loc, sizeof(manifest_t));
			if(memcmp((void *) &manOnExtFlash, (void *) manifest_loc, sizeof(manifest_t)) == 0)
			{
				if(manOnExtFlash.version >= manOnSdCard.version)
				{
					memcpy((void *) &fwuManifest->man, (void *) &manOnExtFlash, sizeof(manifest_t));
					if(memcmp((void *) &fwuManifest->man, (void *) &manOnExtFlash, sizeof(manifest_t)) == 0)
					{
						fwuManifest->isFwuImgOnSdCard = false;
						(void)QMC2_SD_FATFS_Delete(SD_CARD_FW_UPDATE_FILE);

						goto cleanup;
					}
					else
					{
						return kStatus_SSS_Fail;
					}
				}
			}
			else
			{
				return kStatus_SSS_Fail;
			}
		}

		memcpy((void *) &fwuManifest->man, (void *) &manOnSdCard, sizeof(manifest_t));
		if(memcmp((void *) &fwuManifest->man, (void *) &manOnSdCard, sizeof(manifest_t)) == 0)
		{
			goto cleanup;
		}
		else
		{
			return kStatus_SSS_Fail;
		}
	}

	if(*manifest_loc == (uint32_t)MAIN_FW_UPDATE_VENDOR_ID)
	{
		memcpy((void *) &fwuManifest->man, (void *) manifest_loc, sizeof(manifest_t));
		if(memcmp((void *) &fwuManifest->man, (void *) manifest_loc, sizeof(manifest_t)) == 0)
		{
			fwuManifest->isFwuImgOnSdCard = false;
			goto cleanup;
		}
	}

	return kStatus_SSS_Fail;

cleanup:

	/* Check the revision again. */
	//TODO also valide the addresses within the header, to make sure that
	if(fwuManifest->man.version > *manVerInSe)
	{
		return kStatus_SSS_Success;
	}

	PRINTF("\r\nERROR: Incorrect FWU version!\r\n");
	if(fwuManifest->isFwuImgOnSdCard)
	{
		(void)QMC2_SD_FATFS_Delete(SD_CARD_FW_UPDATE_FILE);
	}

	//TODO Delete FW update file or clean FWU storage + possible log the info
	memset(fwuManifest, 0, sizeof(fwu_manifest_t));

	return kStatus_SSS_Fail;
}

sss_status_t QMC2_FWU_CreateRecoveryImage(fw_header_t *fwHeader)
{
	sss_status_t status = kStatus_SSS_Fail;
	uint32_t sizeWithSignature = 0;
	uint32_t pgmAlignedSize = 0;

	assert(fwHeader != NULL);

	/* Read main FW header. */
	status = QMC2_FWU_ReadHeader((uint32_t*) SBL_MAIN_FW_ADDRESS, &fwHeader->hdr);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Read of the Fw header failed!\r\n");
		return status;
	}

	sizeWithSignature = fwHeader->hdr.fwDataLength + SEC_SIGNATURE_BLOCK_SIZE;

	/* Aling the size to pgm page */
	if(sizeWithSignature % PGM_PAGE_SIZE)
	{
		pgmAlignedSize = sizeWithSignature + (PGM_PAGE_SIZE - (sizeWithSignature % PGM_PAGE_SIZE));
	}
	else
	{
		pgmAlignedSize = sizeWithSignature;
	}

	/* Check if alinged size fits into storage */
	if(pgmAlignedSize > SBL_BACKUP_IMAGE_SIZE)
		return kStatus_SSS_Fail;

	/* Erase storage equal to aligned size */
	status = QMC2_FLASH_Erase(SBL_BACKUP_IMAGE_ADDRESS, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the Backup Image location failed!\r\n");
		return status;
	}

	status = QMC2_FLASH_ProgramPages(SBL_BACKUP_IMAGE_ADDRESS, (const void *)fwHeader->hdr.fwDataAddr, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Programming of the Backup Image location failed!\r\n");
		return status;
	}

	status = QMC2_FLASH_encXipMemcmp(fwHeader->hdr.fwDataAddr, SBL_BACKUP_IMAGE_ADDRESS, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Encrypted XiP memcpm failed !\r\n");
		return status;
	}

	/* Erase CFG data backup storage */
	status = QMC2_FLASH_Erase(SBL_CFGDATA_BACKUP_ADDRESS, SBL_CFGDATA_BACKUP_SIZE);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the CFG Data backup location failed!\r\n");
		return status;
	}

	/* Erase CFG data */
	status = QMC2_FLASH_Erase(fwHeader->hdr.cfgDataAddr, SBL_CFGDATA_BACKUP_SIZE);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("Erase of the CFG Data failed!\r\n");
		return status;
	}

	return status;
}

sss_status_t QMC2_FWU_RevertRecoveryImage(fw_header_t *fwHeader)
{
	sss_status_t status = kStatus_SSS_Fail;
	uint32_t sizeWithSignature = 0;
	uint32_t pgmAlignedSize = 0;

	assert(fwHeader != NULL);

	status = QMC2_FLASH_Erase(SBL_MAIN_FW_ADDRESS, QSPI_FLASH_ERASE_SECTOR_SIZE);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the Main FW location failed.!\r\n");
		return status;
	}

	/* Copy first page to read the FW header */
	status = QMC2_FLASH_ProgramPages(SBL_MAIN_FW_ADDRESS, (const void *)SBL_BACKUP_IMAGE_ADDRESS, FW_HEADER_BLOCK_SIZE);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: FW header programming failed!\r\n");
		return status;
	}

	/* Read main FW header. */
	status = QMC2_FWU_ReadHeader((uint32_t*) SBL_MAIN_FW_ADDRESS, &fwHeader->hdr);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Read of the Fw header failed!\r\n");
		return status;
	}

	sizeWithSignature = fwHeader->hdr.fwDataLength + SEC_SIGNATURE_BLOCK_SIZE;

	/* Aling the size to pgm page */
	if(sizeWithSignature % PGM_PAGE_SIZE)
	{
		pgmAlignedSize = sizeWithSignature + (PGM_PAGE_SIZE - (sizeWithSignature % PGM_PAGE_SIZE));
	}
	else
	{
		pgmAlignedSize = sizeWithSignature;
	}

	/* Check if alinged size fits into storage */
	if(pgmAlignedSize > SBL_MAIN_FW_SIZE)
		return kStatus_SSS_Fail;

	fwHeader->backupImgActive = true;

	status = QMC2_FLASH_Erase((SBL_MAIN_FW_ADDRESS+QSPI_FLASH_ERASE_SECTOR_SIZE), (pgmAlignedSize-QSPI_FLASH_ERASE_SECTOR_SIZE));
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the Main FW location failed.!\r\n");
		return status;
	}

	/* Program the rest of data + signature. */
	status = QMC2_FLASH_ProgramPages((SBL_MAIN_FW_ADDRESS + FW_HEADER_BLOCK_SIZE), (const void *)(SBL_BACKUP_IMAGE_ADDRESS + FW_HEADER_BLOCK_SIZE), (pgmAlignedSize - FW_HEADER_BLOCK_SIZE));
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Error in programming !\r\n");
		return status;
	}

	status = QMC2_FLASH_encXipMemcmp(SBL_MAIN_FW_ADDRESS, SBL_BACKUP_IMAGE_ADDRESS, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Encrypted XiP memcpm failed !\r\n");
		return status;
	}

	/* Aling the size to pgm page */
	if(fwHeader->hdr.cfgDataLength % PGM_PAGE_SIZE)
	{
		pgmAlignedSize = fwHeader->hdr.cfgDataLength + (PGM_PAGE_SIZE - (fwHeader->hdr.cfgDataLength % PGM_PAGE_SIZE));
	}
	else
	{
		pgmAlignedSize = fwHeader->hdr.cfgDataLength;
	}

	if(pgmAlignedSize > SBL_CFGDATA_SIZE)
		return kStatus_SSS_Fail;

	/*Erase CFG data location*/
	status = QMC2_FLASH_Erase(fwHeader->hdr.cfgDataAddr, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Erase of the CFG data location failed!\r\n");
		return status;
	}

	status = QMC2_FLASH_ProgramPages(SBL_CFGDATA_BACKUP_ADDRESS, (const void *)fwHeader->hdr.cfgDataAddr, fwHeader->hdr.cfgDataLength);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Programming of the CFG data failed!\r\n");
		return status;
	}

	if (memcmp((void*) fwHeader->hdr.cfgDataAddr, (void*) SBL_CFGDATA_BACKUP_ADDRESS, fwHeader->hdr.cfgDataLength) != 0)
	{
		return kStatus_SSS_Fail;
	}

	return status;
}

sss_status_t QMC2_FWU_BackUpCfgData(header_t *fwHeader)
{
	sss_status_t status = kStatus_SSS_Fail;
	uint32_t pgmAlignedSize = 0;
	assert(fwHeader != NULL);

	/* Read main FW header. */
	status = QMC2_FWU_ReadHeader((uint32_t*)SBL_MAIN_FW_ADDRESS, fwHeader); // TODO SBL_MAIN_FW_ADDRESS insted of simulated header
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Read of the Fw header failed!\r\n");
		return status;
	}

	/* Aling the size to pgm page */
	if(fwHeader->cfgDataLength % PGM_PAGE_SIZE)
	{
		pgmAlignedSize = fwHeader->cfgDataLength + (PGM_PAGE_SIZE - (fwHeader->cfgDataLength % PGM_PAGE_SIZE));
	}
	else
	{
		pgmAlignedSize = fwHeader->cfgDataLength;
	}

	if(pgmAlignedSize > SBL_CFGDATA_BACKUP_SIZE)
	{
		return kStatus_SSS_InvalidArgument;
	}

	status = QMC2_FLASH_Erase(SBL_CFGDATA_BACKUP_ADDRESS, pgmAlignedSize);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: of the Backup Image location failed!\r\n");
		return status;
	}

	status = QMC2_FLASH_ProgramPages(SBL_CFGDATA_BACKUP_ADDRESS, (const void *)fwHeader->cfgDataAddr, fwHeader->cfgDataLength);
	if (status != kStatus_SSS_Success)
	{
		PRINTF("\r\nERROR: Programming of the Backup Image location failed!\r\n");
		return status;
	}

	if (memcmp((void*) fwHeader->cfgDataAddr, (void*) SBL_CFGDATA_BACKUP_ADDRESS, SBL_CFGDATA_BACKUP_SIZE) != 0)
	{
		return kStatus_SSS_Fail;
	}

	return status;
}


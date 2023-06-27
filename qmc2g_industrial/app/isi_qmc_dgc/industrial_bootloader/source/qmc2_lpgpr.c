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
 * @file    qmc2_lpgpr.c
 * @brief   SNVS LP GPR API.
 */

#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include <qmc2_lpgpr.h>


/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/
/* Read persistent storage. */
sss_status_t QMC2_LPGPR_Init(svns_lpgpr_t *snvsLpGpr)
{
	assert(snvsLpGpr != NULL);
	uint32_t snvsStorage = 0;

	SNVS->LPGPR[0] = 0;
	SNVS->LPGPR[1] = 0;
	SNVS->LPGPR[2] = 0;
	SNVS->LPGPR[3] = 0;

	snvsLpGpr->fwState = kFWU_NoState;
	snvsLpGpr->wdStatus = 0;
	snvsLpGpr->wdTimerBackup = 0;

	snvsStorage = SNVS->LPGPR[SNVS_LPGPR_REG_IDX];

	if (SNVS->LPGPR[SNVS_LPGPR_REG_IDX] == snvsStorage)
	{
		return kStatus_SSS_Success;
	}
	else
	{
		return kStatus_SSS_Fail;
	}
}

/* Write persistent storage. */
sss_status_t QMC2_LPGPR_Write(svns_lpgpr_t *snvsLpGpr)
{
	assert(snvsLpGpr != NULL);

    uint32_t snvsStorage              = SNVS->LPGPR[SNVS_LPGPR_REG_IDX];
    snvsStorage                       = snvsStorage & ~SNVS_LPGPR_FWUSTATUS_SNVS_MASK;
    snvsStorage                       = snvsStorage | (snvsLpGpr->fwState << SNVS_LPGPR_FWUSTATUS_SNVS_POS);
    SNVS->LPGPR[SNVS_LPGPR_REG_IDX] = snvsStorage;

	if (SNVS->LPGPR[SNVS_LPGPR_REG_IDX] == snvsStorage)
	{
		return kStatus_SSS_Success;
	}
	else
	{
		return kStatus_SSS_Fail;
	}
}

/* Read persistent storage. */
sss_status_t QMC2_LPGPR_Read(svns_lpgpr_t *snvsLpGpr)
{
	assert(snvsLpGpr != NULL);

    uint32_t snvsStorage = SNVS->LPGPR[SNVS_LPGPR_REG_IDX];
    snvsLpGpr->fwState = (uint8_t)((snvsStorage & SNVS_LPGPR_FWUSTATUS_SNVS_MASK) >> SNVS_LPGPR_FWUSTATUS_SNVS_POS);

	if (SNVS->LPGPR[SNVS_LPGPR_REG_IDX] == snvsStorage)
	{
		return kStatus_SSS_Success;
	}
	else
	{
		return kStatus_SSS_Fail;
	}
}


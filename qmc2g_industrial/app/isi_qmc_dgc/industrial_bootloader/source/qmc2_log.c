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
 * @file    sbl_puf.c
 * @brief   PUF API.
 */

#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include <qmc2_log.h>

/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/
void QMC2_LOG_CreateLogEntry(log_entry_t *log)
{
	assert(log != NULL);

	switch(*log)
	{
		case kLOG_HwInitDeinitFailed:

			break;
		case kLOG_SdCardFailed:

			break;
		case kLOG_SvnsLpGprOpFailed:

			break;
		case kLOG_Scp03KeyRotationFailed:

			break;
		case kLOG_Scp03KeyReconFailed:

			break;
		case kLOG_Scp03ConnFailed:

			break;
		case kLOG_VerReadFromSeFailed:

			break;
		case kLOG_ExtMemOprFailed:

			break;
		case kLOG_FwExecutionFailed:

			break;
		case kLOG_NewFWRevertFailed:

			break;
		case kLOG_NewFWCommitFailed:

			break;
		case kLOG_CfgDataBackupFailed:

			break;
		case kLOG_BackUpImgAuthFailed:

			break;
		case kLOG_DecomissioningFailed:

			break;
		case kLOG_DeviceDecommisioned:

			break;
		case kLOG_NewFWReverted:

			break;
		case kLOG_NewFWCommited:

			break;
		case kLOG_AwdtExpired:

			break;
		case kLOG_MainFwAuthFailed:

			break;
		case kLOG_FwuAuthFailed:

			break;
		case kLOG_StackError:

			break;
		case kLOG_KeyRevocation:

			break;
		case kLOG_InvalidFwuVersion:

			break;
		default:
			break;
	}

}

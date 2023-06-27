/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __qmc2_fwu_h_
#define __qmc2_fwu_h_

//-----------------------------------------------------------------------
// Linker macros placing include file
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "qmc2_types.h"
//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

//
//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Typedef
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
sss_status_t QMC2_FWU_ReadManifest(fwu_manifest_t *fwuManifest, uint32_t *manVerInSe, bool isSdCardInserted);
/* Read FW Update headre. */
sss_status_t QMC2_FWU_ReadHeader(uint32_t *address, header_t *fwHeader);
sss_status_t QMC2_FWU_CreateRecoveryImage(fw_header_t *fwHeader);
sss_status_t QMC2_FWU_RevertRecoveryImage(fw_header_t *fwHeader);
//sss_status_t QMC2_SE_CommitFwVersionToSe(fw_header_t *fwHeader, uint32_t *fwVersionInSe);
sss_status_t QMC2_FWU_BackUpCfgData(header_t *fwHeader);
/* Authenticate function using SE */
//sss_status_t SBL_Authenticate(header_t *header, uint32_t *fwVerInSe);
sss_status_t QMC2_FWU_Program(boot_data_t *boot, log_entry_t *log);
sss_status_t QMC2_FWU_SdToFwuStorage(boot_data_t *boot, log_entry_t *log);

sss_status_t QMC2_FWU_SdToFwStorage(void);
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __qmc2_fwu_h_ */

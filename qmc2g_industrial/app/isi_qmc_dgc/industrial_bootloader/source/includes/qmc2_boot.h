/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __QMC2_BOOT_h_
#define __QMC2_BOOT_h_

#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------------------------
// Linker macros placing include file
//-----------------------------------------------------------------------
#include <qmc2_types.h>
#include "api_logging.h"
//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Typedef
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------     
// Function Prototypes                                                          
//------------------------------------------------------------------------------
// Callback definitions
/* Main QMC2 Bootloader flow */
int QMC2_BOOT_Main(void);
void  QMC2_BOOT_ErrorTrap(log_event_code_t *log);
#ifdef SECURE_SBL
/* Rotate and mandate new SCP03 platform keys */
sss_status_t QMC2_BOOT_RotateMandateSCP03Keys(PUF_Type *base, boot_data_t *boot, log_event_code_t *log);
#endif
sss_status_t QMC2_BOOT_AuthenticateMainFw(boot_data_t *boot, log_event_code_t *log);
sss_status_t QMC2_BOOT_AuthenticateFwu(boot_data_t *boot);
sss_status_t QMC2_BOOT_ExecuteCm4Cm7Fws(fw_header_t *currFwHeader);
#ifdef SECURE_SBL
sss_status_t QMC2_BOOT_Decommissioning(boot_data_t *boot, log_event_code_t *log);
#endif
sss_status_t QMC2_BOOT_Cleanup(boot_data_t *boot);
/* Authentiacte main FW */
sss_status_t QMC2_BOOT_Authenticate(boot_data_t *boot);
sss_status_t QMC2_BOOT_ProcessMainFwRequest(boot_data_t *boot, log_event_code_t *log);
sss_status_t QMC2_BOOT_FwUpdate(boot_data_t *boot, log_event_code_t *log);

sss_status_t QMC2_BOOT_InitLogKeys(log_keys_t *logKeys);
sss_status_t QMC2_BOOT_RPC_InitSecureWatchdog(scp03_keys_t *scp03);
sss_status_t QMC2_BOOT_PassScp03Keys(scp03_keys_t *scp03);
void  QMC2_BOOT_InitSrtc(void);

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __QMC2_BOOT_h_ */

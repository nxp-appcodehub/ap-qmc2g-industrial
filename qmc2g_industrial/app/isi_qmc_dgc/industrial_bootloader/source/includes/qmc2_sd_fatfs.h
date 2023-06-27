/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __QMC2_SD_FATFS_h_
#define __QMC2_SD_FATFS_h_

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
/* Create log entry */
sss_status_t QMC2_SD_FATFS_Init(bool *isSdCardInserted);
sss_status_t QMC2_SD_FATFS_DeInit(void);
sss_status_t QMC2_SD_FATFS_Open(const char* path);
sss_status_t QMC2_SD_FATFS_Read(uint8_t *buffer, uint32_t length, uint32_t offset);
sss_status_t QMC2_SD_FATFS_Close(void);
sss_status_t QMC2_SD_FATFS_Delete(const char* path);
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __QMC2_SD_FATFS_h_ */

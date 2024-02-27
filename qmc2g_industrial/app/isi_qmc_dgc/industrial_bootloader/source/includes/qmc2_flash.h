/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __qmc2_flash_h_
#define __qmc2_flash_h_

//-----------------------------------------------------------------------
// Linker macros placing include file
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "qmc2_types.h"
#include "fsl_flexspi.h"

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
// Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
sss_status_t QMC2_FLASH_DriverInit(void);
sss_status_t QMC2_FLASH_OctalRamInit(void);
sss_status_t QMC2_FLASH_ProgramPages(uint32_t dst, const void *src, uint32_t size);
sss_status_t QMC2_FLASH_Erase(uint32_t dst, uint32_t size);
sss_status_t QMC2_FLASH_encXipMemcmp(uint32_t encXipAddress, uint32_t address, uint32_t length);
void QMC2_FLASH_CleanFlexSpiData(void);
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __qmc2_flash_h_ */

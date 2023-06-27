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
#define OCTAL_FLASH_FLEXSPI FLEXSPI1
#define OCTAL_RAM_FLEXSPI FLEXSPI1
#define OCTAL_FLASH_SIZE_A1 0x10000
#define OCTAL_RAM_SIZE_A2 0x8000
#define OCTAL_FLASH_SIZE (OCTAL_FLASH_SIZE_A1*1024U)
#define OCTAL_FLASH_PAGE_SIZE 256U
#define	OCTAL_FLASH_SECTOR_SIZE 4096U

//LUT sequences
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA 		0
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READID 			1
#define OCTALRAM_CMD_LUT_SEQ_IDX_READDATA 			2
#define OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA 			3
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS		4
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2			6
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2		8
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2_SPI	13
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE		7
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE_SPI	10
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA 		11
#define OCTALFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR 		12
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG			9

#define CUSTOM_LUT_LENGTH 64

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

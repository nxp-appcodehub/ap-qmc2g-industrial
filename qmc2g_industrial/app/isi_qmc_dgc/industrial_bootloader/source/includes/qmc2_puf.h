/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __qmc2_puf_h_
#define __qmc2_puf_h_

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
sss_status_t QMC2_PUF_Init(void);
sss_status_t QMC2_PUF_ReconstructKeys(PUF_Type *base, boot_keys_t *bootKeys);
sss_status_t QMC2_PUF_GeneratePufKeyCodes(PUF_Type *base, puf_key_store_t *pufKeyStore, uint8_t *aes_policy_key);
sss_status_t QMC2_PUF_ProgramKeyStore(uint32_t address, puf_key_store_t *pufKeyStore);
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __qmc2_puf_h_ */

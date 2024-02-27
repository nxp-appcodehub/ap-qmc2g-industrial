/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __sbl_se_h_
#define __sbl_se_h_

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
/* Establish SCP03 connection. */
sss_status_t QMC2_SE_CheckScp03Conn(scp03_keys_t *scp03);
/* Check if device is commisioned by reading AES key for policies  */
sss_status_t QMC2_SE_ReadPolicyAesKey(scp03_keys_t *scp03, uint8_t *aesPolicyKey);
/* Mandate new SCP03 KeySet */
sss_status_t QMC2_SE_MandateScp03Keys(scp03_keys_t *scp03);
/* Un-Mandate new SCP03 KeySet */
sss_status_t QMC2_SE_UnMandateScp03Keys(scp03_keys_t *scp03);
/* Rotate new SCP03 KeySet */
sss_status_t QMC2_SE_RotateSCP03Keys(scp03_keys_t *oldScp03, scp03_keys_t *newScp03);
/* Read FW version fron SE*/
sss_status_t QMC2_SE_ReadVersionsFromSe(se_data_t *seData, scp03_keys_t *scp03);
/* Commit FW version into SE */
sss_status_t QMC2_SE_CommitFwVersionToSe(scp03_keys_t *scp03, header_t *hdr, uint32_t *fwVersionInSe);
/* Commit Manifest version into SE */
sss_status_t QMC2_SE_CommitManVersionToSe(scp03_keys_t *scp03, manifest_t *man, uint32_t *manVersionInSe);
/* Verify fcn function */
sss_status_t QMC2_SE_VerifySignature(scp03_keys_t *scp03, se_verify_sign_t *seAuth);
/* Write data */
sss_status_t QMC2_SE_WriteData(uint32_t object_id, uint8_t *seData, uint32_t length);
sss_status_t QMC2_SE_FactoryReset(scp03_keys_t *scp03);
/* Generated seed value and read key from SE to init RPC */
sss_status_t QMC2_SE_GetRpcSeedAndKey(scp03_keys_t *scp03, uint8_t *data, size_t dataLength, uint8_t *key, size_t keyLength);
/* Erase object*/
sss_status_t QMC2_SE_EraseObj(scp03_keys_t *scp03, uint32_t objectId);
/* sha512*/
sss_status_t SE_MbedtlsSha512(uint32_t input, size_t ilen, uint8_t output[64],  int is384);
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __sbl_se_h_ */

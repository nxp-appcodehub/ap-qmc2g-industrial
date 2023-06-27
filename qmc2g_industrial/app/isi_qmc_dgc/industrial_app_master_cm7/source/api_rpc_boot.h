/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_RPC_BOOT_H_
#define _API_RPC_BOOT_H_

#include "api_rpc.h"


/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Provides the RNG seed and public key to the secure watchdog implementation on the CM4.
 *
 * Other Secure Watchdog methods are not allowed until this function is completed.
 *
 * The secure watchdog implementation validates the inputs and triggers a reboot into recovery mode if they are invalid.
 *
 * @param[in] seed A byte array containing the RNG seed.
 * @param[in] seedLen Length of the RNG seed.
 * @param[in] key A byte array containing the 256bit ECC public key in ASN.1 DER format for ticket verification.
 * @param[in] keyLen Length of the 256bit ECC public key.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_ErrArgInvalid
 * The size of the seed or key exceeded the maximal expected length.
 * @retval kStatus_QMC_Ok
 * The init data was copied successfully.
 */
qmc_status_t RPC_InitSecureWatchdog(const uint8_t *seed, size_t seedLen, const uint8_t *key, size_t keyLen);

#endif /* _API_RPC_BOOT_H_ */

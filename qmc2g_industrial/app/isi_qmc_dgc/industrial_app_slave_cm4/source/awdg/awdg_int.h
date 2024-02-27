/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file awdg_int.h
 * @brief Defines internals of the authenticated watchdog timer (AWDG).
 *
 * Internally used types, do not include!
 *
 */
#ifndef _AWDG_INT_H_
#define _AWDG_INT_H_

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pk.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Context for the AWDG.
 */
typedef struct
{
    bool isRngDisabled;                   /*!< Is our RNG disabled (due to unrecoverable error(s))? */
    mbedtls_ctr_drbg_context ctrDrbg;     /*!< mbedTLS context for the CTR DRBG. */
    mbedtls_pk_context pk;                /*!< mbedTLS context for the public key, will be an ECC key in our case. */
    volatile logical_watchdog_t watchdog; /*!< The AWDGs underlying watchdog, shared with an external agent (ISR). */
    volatile logical_watchdog_unit_t watchdogUnit; /*!< The watchdog unit holding the AWDGs underlying watchdog and its
                                                        grace period watchdog, shared with an external agent (ISR). */
    uint8_t nonceTicket[AWDG_NONCE_LENGTH];        /*!< Current nonce which should be used for ticket signature check. */
    uint8_t rngSeed[AWDG_RNG_SEED_LENGTH];         /*!< Random seed buffer. */
    uint32_t rngSeedLen;                           /*!< Random seed length. */
    bool wasEntropyUsed;                           /*!< Has entropy been already requested? */
    uint8_t hashBuffer[AWDG_HASH_LENGTH];          /*!< Buffer for hash data. */
    uint8_t data[AWDG_DATA_COMBINED_LENGTH];       /*!< Buffer for timeout|nonce data. */
    bool canDefer;                                 /*!< Is an user allowed to defer the authenticated watchdog? */
    uint32_t deferralTimeoutMs;                    /*!< The next deferral time in ms.  */
} awdg_t;

#endif /* _AWDG_INT_H_ */

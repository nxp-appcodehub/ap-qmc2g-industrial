/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
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
    mbedtls_ctr_drbg_context ctrDrbg;     /*!< mbedTLS context for the CTR DRBG */
    mbedtls_pk_context pk;                /*!< mbedTLS context for the public key, will be an ECC key in our case. */
    volatile logical_watchdog_t watchdog; /*!< The AWDGs underlying watchdog, shared with an external agent (ISR). */
    volatile logical_watchdog_unit_t watchdogUnit; /*!< The watchdog unit holding the AWDGs underlying watchdog and its
                                                        grace period watchdog, shared with an external agent (ISR). */
    uint8_t nonceTicket[AWDG_NONCE_LENGTH];        /*!< Current nonce which should be in a ticket. */
    uint8_t rngSeed[AWDG_RNG_SEED_LENGTH];         /*!< Random seed buffer. */
    uint32_t rngSeedLen;                           /*!< Random seed length. */
    bool wasEntropyUsed;                           /*!< Has entropy been already requested? */
    uint8_t hashBuffer[AWDG_HASH_LENGTH];          /*!< Buffer for hash data. */
    uint8_t data[AWDG_DATA_COMBINED_LENGTH];       /*!< Buffer for timeout|nonce data. */
    bool canDefer;                                 /*!< Is an user allowed to defer the authenticated watchdog? */
    uint32_t deferralTimeoutMs;                    /*!< The next deferral time in ms.  */
} awdg_t;

#endif /* _AWDG_INT_H_ */

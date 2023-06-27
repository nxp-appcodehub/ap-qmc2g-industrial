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
 * @file awdg_api.c
 * @brief Defines the authenticated watchdog timer (AWDG).
 *
 * The authenticated watchdog timer is an extension for the LWDGU module. An authenticated
 * watchdog only allows the deferral if a fresh + valid signed ticket is provided.
 *
 * This module relies on the mbedtls library and requires at least AWDG_MBEDTLS_HEAP_MIN_STATIC_BUFFER_SIZE
 * bytes of memory. The user has to setup mbedtls to have at least this amount of heap
 * memory available!
 *
 * The AWDG has to be initialized dynamically with the AWDG_Init() function which also
 * starts the watchdog. A nonce for ticket creation can be obtained by calling the
 * AWDG_GetNonce() function. A signed ticket can be given to and verified by the AWDG
 * using the AWDG_ValidateTicket() function (takes up to 15016ms on a RT1176 CM4).
 * After a successful verification attempt, the user can defer the watchdog by executing
 * the AWDG_DeferWatchdog() function.
 *
 * Check the API descriptions of the individual functions for detailed information
 * about the concurrency assumptions.
 *
 * The ticket signature algorithm is fixed to ECDSA with a 512 bit curve, the
 * signature and key have to be in the ASN.1 DER format defined in www.secg.org/sec1-v2.pdf
 * Appendix C. The ticket is a binary blob with the following content:
 *  | new timeout (u32 LE) | ASN.1 DER 512 bit ECDSA signature |
 *   0                    3 4                               140 (max)
 *
 * The external requirements for the QMC2G project are:
 *  - The AWDG should be ticked every millisecond. This allows timeout periods
 *    up to (2^32 - 2) / 1000 / 3600 / 24 ~ 49 days.
 *  - After the AWDG expires the system has to be notified about the upcoming reset (logging)
 *    and the reset cause  has to be stored in the SNVS storage. The reset cause should not
 *    be reset during booting by the CM4 (as the CM7 application needs the contents).
 *  - The AWDG's counter value and state (running / not running) should be stored in a
 *    shadow register for the SNVS storage after each AWDG tick in the timer interrupt.
 *    The backup counter value is calculated by right shifting the current tick value by 16
 *    (+ rounding up; value approximately represents minutes). If the AWDG has not expired the
 *    backup state value matches the AWDG's isRunning flag, otherwise it is set to 0.
 *    If the AWDG did expire the state value of 0 is directly written into the SNVS
 *    storage, similar as the reset cause, to ensure the values are backed up before
 *    the system reset.
 *    The shadow register is periodically written back to the SNVS storage in the main loop.
 *    These backup values are used during initialization to restore the watchdog's state (if it was running).
 *    For the initialization the 16-bit shifted counter value is decremented by 1.
 *    The two possible saved states before an initialization are: Timer was not running at all (fresh init)
 *    and timer was running (resume with values from SNVS).
 *    The SNVS values are reset to zero at a cold boot, hence the values in the SNVS storage
 *    should be designed so that an initial value of zero is logically correct
 *    (e.g. a running flag is ok).
 *  - The before mentioned SNVS storage has to be isolated from the CM7 after the initial
 *    bootloader handed over control and must be only accessible by the CM4 afterwards.
 *  - If the AWDG should expire during the grace period of a functional watchdog unit,
 *    the reset cause shall be overwritten to reflect that the AWDG expired. This
 *    behaviour is relevant for security as the reset cause is used to trigger an
 *    boot of the recovery image. However, in the worst case this means that the log entry describing
 *    that the AWDG expired will not make it into the log file.
 *    Further, in the case a functional watchdog unit expires during the grace period of the AWDG
 *    the reset cause should not be overwritten by the functional watchdog unit!
 *  - If the AWDG initialization fails the caller has to ensure that the board reboots
 *    into recovery mode.
 *  - The QMC project needs possible timeout times of >= 7 days, this should be asserted wherever
 *    the AWDG is configured. Can not be done in this module as it would influence testability.
 *
 */
#include "awdg_api.h"
#include "awdg_int.h"
#include "utils/mem.h"
#include "utils/testing.h"

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/base64.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha512.h"
#include "mbedtls/version.h"

#include <assert.h>
#include <string.h>

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*!
 * @brief Instance of the Authenticated Watchdog
 *
 */
STATIC_TEST_VISIBLE awdg_t gs_awdg;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Function that mbedtls calls when entropy is needed for the random number generator
 *        More details here: tls.mbed.org/module-level-design-rng
 *
 * @startuml
 * start
 *  :ret = -1;
 *  if () then (gs_awdg.wasEntropyUsed == false && len <= AWDG_RNG_SEED_LENGTH)
 *     :gs_awdg.wasEntropyUsed = true
 *     output = gs_awdg.rngSeed
 *     gs_awdg.rngSeed = 0
 *     ret = 0;
 *  else ()
 *     ret = -1;
 *  endif
 * :return ret;
 * stop
 * @enduml
 *
 * tls.mbed.org/img/plantuml/VLBDJiCm3BxdAQpTnmDC58q_LLmu53WZDp7CKfT6YJjfRuzJsf6DGgLARVtzEZNR91Xbs7V6e9K-mcq87LiKxhqnGMTiEQ0NMDs_DkyFpLsz0apGFDVduqSliGz7OweAdZmBOyU9eAfCeGZhcypSb0WD89JD-Q0Fex3U9sH3YMG2pOF9QmCV97O7D5cVDEOi6NwzNl_WPwIZHUpi6TMJN8dAgRHOhL4YUjVlmG_xbk9V_GfWUnHAv_tw89C7i1UA1-pq7UrEF-WHX4YExQYdADRktf9cnxRJLA3N_tl8Y5_zPjhYFl9mOIU5LRaNyiwC7T0vpZ4rlaD129YIpalAMt1PSUmw9FQV83kw95wDaYMj9ayn4y-Mk23yCluOvH7aWjzfI-h5MPgjEkQ__Wi0
 *
 * Zeros the the gs_awdg.rngSeed buffer after the seed was consumed.
 *
 * @param pParam User-supplied parameter, in our case NULL
 * @param pOutput The data that needs to be filled with entropy
 * @param len The length of the output data to be filled
 * @return Zero if success, any other value if an error occurred.
 */
static int mbedtls_custom_entropy_func(void *pParam, unsigned char *pOutput, size_t len)
{
    int ret = -1;
    (void)pParam;

    if ((NULL != pOutput) && (false == gs_awdg.wasEntropyUsed) && (len <= AWDG_RNG_SEED_LENGTH))
    {
        gs_awdg.wasEntropyUsed = true;
        (void)memcpy(pOutput, gs_awdg.rngSeed, len);
        /* not needed anymore - zero */
        (void)memset(gs_awdg.rngSeed, 0, AWDG_RNG_SEED_LENGTH);
        ret = 0;
    }
    else
    {
        ret = -1;
    }

    return ret;
}

/*!
 * @brief Frees the allocated mbedtls objects.
 *
 */
static void AWDG_FreeMbedtlsInternal(void)
{
    mbedtls_pk_free(&gs_awdg.pk);
    mbedtls_ctr_drbg_free(&gs_awdg.ctrDrbg);
}

/*!
 * @brief Internal function for setting up the needed mbedtls objects.
 *
 * Creates + seeds the random number generator and parses the given public key.
 * The entropy for seeding the random number generator is given to the function.
 *
 * @startuml
 * start
 *      :ret = false
 *      mbedtls_ctr_drbg_init(&gs_awdg.ctrDrbg)
 *      mbedtls_pk_init(&gs_awdg.pk)
 *      gs_awdg.rngSeed = rngSeed
 *      gs_awdg.rngSeedLen = rngSeedLen;
 *      if (mbedtls_ctr_drbg_seed(&gs_awdg.ctrDrbg, mbedtls_custom_entropy_func, NULL, NULL, 0U)) then (failed)
 *         : *detailedReturn = kStatus_AWDG_InitCtrDrbgSeedError
 *         ret = false;
 *      elseif (mbedtls_pk_parse_public_key(&gs_awdg.pk, eccPublicKey, keyLen)) then (failed)
 *         : *detailedReturn = kStatus_AWDG_InitParseEccKeyError
 *         ret = false;
 *      elseif (mbedtls_pk_can_do(&gs_awdg.pk, AWDG_EXPECTED_KEY_TYPE)) then (false)
 *         : *detailedReturn = kStatus_AWDG_InitParseEccKeyError
 *         ret = false;
 *      elseif (mbedtls_pk_get_bitlen(&gs_awdg.pk) != AWDG_EXPECTED_CURVE_SIZE_BIT) then (true)
 *         : *detailedReturn = kStatus_AWDG_InitParseEccKeyError
 *         ret = false;
 *      else (else)
 *         : *detailedReturn = kStatus_AWDG_InitInitializedNew
 *         ret = true;
 *      endif
 *      if (ret == false)
 *         :AWDG_FreeMbedtlsInternal();
 *      endif
 * :return ret;
 * stop
 * @enduml
 *
 * @param pRngSeed Pointer to a byte array containing the RNG seed.
 * @param rngSeedLen Length of the RNG seed.
 * @param pEccPublicKey Pointer to a byte array containing the ECC public key.
 * @param keyLen Length of the ECC public key.
 * @param pDetailedReturn To this memory location a more detailed return value is written.
 * @return An boolean.
 * @retval true the function succeeded.
 * @retval false the function failed.
 */
static bool AWDG_SetupMbedtlsInternal(const uint8_t *const pRngSeed,
                                      const uint32_t rngSeedLen,
                                      const uint8_t *const pEccPublicKey,
                                      const uint32_t keyLen,
                                      awdg_init_status_t *const pDetailedReturn)
{
    assert(NULL != pRngSeed);
    assert(NULL != pEccPublicKey);
    assert(NULL != pDetailedReturn);
    bool ret = false;

    /* init random number generator context */
    mbedtls_ctr_drbg_init(&gs_awdg.ctrDrbg);
    mbedtls_ctr_drbg_set_entropy_len(&gs_awdg.ctrDrbg, AWDG_RNG_SEED_LENGTH);
    /* disable reseeding (because we don't update the entropy source) */
    mbedtls_ctr_drbg_set_prediction_resistance(&gs_awdg.ctrDrbg, MBEDTLS_CTR_DRBG_PR_OFF);
    mbedtls_ctr_drbg_set_reseed_interval(&gs_awdg.ctrDrbg, INT_MAX);

    /* init public key */
    mbedtls_pk_init(&gs_awdg.pk);

    /* copy seed - length was already checked */
    (void)memcpy(gs_awdg.rngSeed, pRngSeed, rngSeedLen);
    gs_awdg.rngSeedLen = rngSeedLen;

    /* seed rng */
    if (0 != mbedtls_ctr_drbg_seed(&gs_awdg.ctrDrbg, mbedtls_custom_entropy_func, NULL, NULL, 0U))
    {
        *pDetailedReturn = kStatus_AWDG_InitCtrDrbgSeedError;
        ret              = false;
    }
    /* parse public key */
    else if (0 != mbedtls_pk_parse_public_key(&gs_awdg.pk, pEccPublicKey, keyLen))
    {
        *pDetailedReturn = kStatus_AWDG_InitParseEccKeyError;
        ret              = false;
    }
    /* check if public key has right format */
    else if ((1 != mbedtls_pk_can_do(&gs_awdg.pk, AWDG_EXPECTED_KEY_TYPE)) ||
             (AWDG_EXPECTED_CURVE_SIZE_BIT != mbedtls_pk_get_bitlen(&gs_awdg.pk)))
    {
        *pDetailedReturn = kStatus_AWDG_InitParseEccKeyError;
        ret              = false;
    }
    /* everything is successful */
    else
    {
        *pDetailedReturn = kStatus_AWDG_InitInitializedNew;
        ret              = true;
    }

    /* setting up mbedtls failed - free resources*/
    if (false == ret)
    {
        AWDG_FreeMbedtlsInternal();
    }

    return ret;
}

awdg_init_status_t AWDG_Init(const uint32_t initialTimeoutMs,
                             const uint32_t gracePeriodTimeoutMs,
                             const uint32_t tickFrequencyHz,
                             const uint32_t savedTicksToTimeout,
                             const bool wasRunning,
                             const uint8_t *const pRngSeed,
                             const uint32_t rngSeedLen,
                             const uint8_t *const pEccPublicKey,
                             const uint32_t keyLen)
{
    awdg_init_status_t ret             = kStatus_AWDG_InitError;
    awdg_init_status_t setupMbedtlsRet = kStatus_AWDG_InitError;
    lwdg_status_t lwdgRet              = kStatus_LWDG_Err;
    lwdg_kick_status_t kickRet         = kStatus_LWDG_KickErr;
    (void)lwdgRet;
    (void)kickRet;

    /* zero struct */
    vmemset(&gs_awdg, 0U, sizeof(awdg_t));

    /* validate user input */
    if ((NULL == pRngSeed) || (NULL == pEccPublicKey))
    {
        ret = kStatus_AWDG_InitInvalidPointer;
    }
    else if (initialTimeoutMs < 1U)
    {
        ret = kStatus_AWDG_InitInvalidTimeout;
    }
    else if (tickFrequencyHz < 1U)
    {
        ret = kStatus_AWDG_InitInvalidTickFrequency;
    }
    else if (AWDG_RNG_SEED_LENGTH != rngSeedLen)
    {
        ret = kStatus_AWDG_InitSeedLengthError;
    }
    /* initialize underlying logical watchdog unit */
    else if (kStatus_LWDG_Ok !=
             LWDGU_Init(&gs_awdg.watchdogUnit, gracePeriodTimeoutMs, tickFrequencyHz, &gs_awdg.watchdog, 1U))
    {
        ret = kStatus_AWDG_InitLogicalWatchdogUnitError;
    }
    /* initialize underlying watchdog with its initial timeout time */
    else if (kStatus_LWDG_Ok != LWDGU_InitWatchdog(&gs_awdg.watchdogUnit, 0U, initialTimeoutMs))
    {
        ret = kStatus_AWDG_InitLogicalWatchdogUnitError;
    }
    /* setup mbedtls */
    else if (false == AWDG_SetupMbedtlsInternal(pRngSeed, rngSeedLen, pEccPublicKey, keyLen, &setupMbedtlsRet))
    {
        ret = setupMbedtlsRet;
    }
    /* set initial nonce to fresh random values so that attacker can not guess value */
    else if (0 != mbedtls_ctr_drbg_random(&gs_awdg.ctrDrbg, gs_awdg.nonceTicket, AWDG_NONCE_LENGTH))
    {
        ret = kStatus_AWDG_InitNonceRNGFailed;
    }
    /* the watchdog was running previously - restore values */
    else if (true == wasRunning)
    {
        /* something was wrong with the saved timeout ticks value */
        if (kStatus_LWDG_Ok != LWDGU_ChangeTimeoutTicksWatchdog(&gs_awdg.watchdogUnit, 0U, savedTicksToTimeout))
        {
            ret = kStatus_AWDG_InitInvalidSavedTimeoutTicks;
        }
        /* ok */
        else
        {
            kickRet = LWDGU_KickOne(&gs_awdg.watchdogUnit, 0U);
            /* if the LWDGU module works as described this can not fail */
            assert(kStatus_LWDG_KickStarted == kickRet);
            ret = kStatus_AWDG_InitInitializedResumed;
        }
    }
    /* the watchdog was not running previously - start */
    else
    {
        kickRet = LWDGU_KickOne(&gs_awdg.watchdogUnit, 0U);
        /* if the LWDGU module works as described this can not fail */
        assert(kStatus_LWDG_KickStarted == kickRet);
        ret = kStatus_AWDG_InitInitializedNew;
    }

    /* error happend - perform cleanup */
    if ((kStatus_AWDG_InitInitializedResumed != ret) && (kStatus_AWDG_InitInitializedNew != ret))
    {
        /* was mbedtls initialized ? if so cleanup */
        if (kStatus_AWDG_InitInitializedNew == setupMbedtlsRet)
        {
            AWDG_FreeMbedtlsInternal();
        }
    }

    return ret;
}

/* has to be run while interrupts are disabled */
lwdg_tick_status_t AWDG_Tick(void)
{
    /* must be that way if initialization was programmed correctly */
    assert(gs_awdg.watchdogUnit.lwdgsCount > 0U);

    return LWDGU_Tick(&gs_awdg.watchdogUnit);
}

size_t AWDG_GetNonce(uint8_t *const pNonce)
{
    size_t ret = 0U;

    /* rng is disabled (no fresh nonce) or pointer is NULL
     * we can not continue */
    if ((true == gs_awdg.isRngDisabled) || (NULL == pNonce))
    {
        ret = 0U;
    }
    else
    {
        /* we have a fresh nonce
         * created during initialization and after ticket verification attempts
         */
        (void)memcpy(pNonce, gs_awdg.nonceTicket, AWDG_NONCE_LENGTH);
        ret = AWDG_NONCE_LENGTH;
    }

    return ret;
}

awdg_validate_ticket_status_t AWDG_ValidateTicket(const uint8_t *const pTicket, const size_t ticketLen)
{
    awdg_validate_ticket_status_t ret = kStatus_AWDG_ValidateTicketError;
    /* reset deferral allowance */
    gs_awdg.canDefer = false;

    /* if RNG is disabled we can not continue (later needed to invalidate nonce) */
    if (true == gs_awdg.isRngDisabled)
    {
        ret = kStatus_AWDG_ValidateTicketRNGDisabled;
    }
    else
    {
        /* check input */
        if (NULL == pTicket)
        {
            ret = kStatus_AWDG_ValidateTicketInvalidPointer;
        }
        else if ((ticketLen < AWDG_MIN_TICKET_LENGTH) || (ticketLen > AWDG_MAX_TICKET_LENGTH))
        {
            ret = kStatus_AWDG_ValidateTicketInvalidTicketLength;
        }
        /* input seems ok, check signature */
        else
        {
            const uint32_t signatureLen    = (ticketLen - AWDG_TIMEOUT_LENGTH);
            const uint8_t *const signature = &pTicket[AWDG_TICKET_SIGNATURE_INDEX];
            /* NOTE endianness - defined as little-endian!*/
            const uint32_t timeout = unpackU32LittleEndian(pTicket);

            /* build signed data */
            (void)memcpy(gs_awdg.data, pTicket, AWDG_TIMEOUT_LENGTH);
            (void)memcpy(&gs_awdg.data[AWDG_TIMEOUT_LENGTH], gs_awdg.nonceTicket, AWDG_NONCE_LENGTH);

            /* hash data */
            if (0 != mbedtls_sha512_ret(gs_awdg.data, AWDG_DATA_COMBINED_LENGTH, gs_awdg.hashBuffer, 0))
            {
                ret = kStatus_AWDG_ValidateTicketHashingFailed;
            }
            /* signature verification failed */
            else if (0 != mbedtls_pk_verify(&gs_awdg.pk, MBEDTLS_MD_SHA512, gs_awdg.hashBuffer, AWDG_HASH_LENGTH,
                                            signature, signatureLen))
            {
                ret = kStatus_AWDG_ValidateTicketDidNotValidate;
            }
            /* signature verification successful - user is allowed to defer timer */
            else
            {
                gs_awdg.deferralTimeoutMs = timeout;
                gs_awdg.canDefer          = true;

                ret = kStatus_AWDG_ValidateTicketValidated;
            }
        }

        /* invalidate previous nonce */
        if (0 != mbedtls_ctr_drbg_random(&gs_awdg.ctrDrbg, gs_awdg.nonceTicket, AWDG_NONCE_LENGTH))
        {
            /* disable future calls to AWDG_GetNonce and AWDG_ValidateTicket
             * (no verification cycle should be possible anymore) */
            gs_awdg.isRngDisabled = true;
        }
    }

    return ret;
}

awdg_defer_watchdog_status_t AWDG_DeferWatchdog(void)
{
    awdg_defer_watchdog_status_t ret = kStatus_AWDG_DeferWatchdogError;
    lwdg_kick_status_t kickRet       = kStatus_LWDG_KickErr;
    (void)kickRet;

    /* not allowed to defer */
    if (false == gs_awdg.canDefer)
    {
        ret = kStatus_AWDG_DeferWatchdogNotAllowed;
    }
    /* set new timout */
    else if (kStatus_LWDG_Ok != LWDGU_ChangeTimeoutTimeMsWatchdog(&gs_awdg.watchdogUnit, 0U, gs_awdg.deferralTimeoutMs))
    {
        ret = kStatus_AWDG_DeferWatchdogInvalidTimeout;
    }
    /* deferral can be performed */
    else
    {
        kickRet = LWDGU_KickOne(&gs_awdg.watchdogUnit, 0U);
        /* must be true if LWDGU unit behaves as described */
        assert(kStatus_LWDG_KickKicked == kickRet);
        ret = kStatus_AWDG_DeferWatchdogOk;
    }

    /* reset deferral allowance (can only be used once) */
    gs_awdg.canDefer = false;

    return ret;
}

uint32_t AWDG_GetRemainingTicks(void)
{
    uint32_t remainingTicks = 0U;
    lwdg_status_t lwdgRet   = kStatus_LWDG_Err;
    (void)lwdgRet;

    lwdgRet = LWDGU_GetRemainingTicksWatchdog(&gs_awdg.watchdogUnit, 0U, &remainingTicks);
    /* must be true if LWDGU unit behaves as described */
    assert(kStatus_LWDG_Ok == lwdgRet);

    return remainingTicks;
}

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
 * @file awdg_api.h
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
 * The AWDG_Tick() and AWDG_GetRemainingTicks() functions can be called from interrupts,
 * other functions should not be called from interrupts.
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
#ifndef _AWDG_API_H_
#define _AWDG_API_H_

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

#include "lwdg/lwdg_unit_api.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Signature algorithm is fixed: ECDSA with a 512 bit curve */
#define AWDG_EXPECTED_KEY_TYPE       MBEDTLS_PK_ECDSA /*!< ECDSA as signature algorithm. */
#define AWDG_EXPECTED_CURVE_SIZE_BIT (512U)           /*!< We only support 512 bit ECDSA curves. */
/* crypto.stackexchange.com/questions/50019/public-key-format-for-ecdsa-as-in-fips-186-4 */
#define AWDG_NONCE_LENGTH   (32U)            /*!< Length of the nonce used within ticket data. */
#define AWDG_TIMEOUT_LENGTH sizeof(uint32_t) /*!< Length of the timeout used within ticket data. */
#define AWDG_TICKET_SIGNATURE_INDEX \
    (AWDG_TIMEOUT_LENGTH) /*!< Index at which the signature is present in the ticket data. */
/* stackoverflow.com/questions/17269238/ecdsa-signature-length
 * superuser.com/questions/1023167/can-i-extract-r-and-s-from-an-ecdsa-signature-in-bit-form-and-vica-versa */
#define AWDG_TICKET_MAX_SIGNATURE_LENGTH \
    ((AWDG_EXPECTED_CURVE_SIZE_BIT + 3U) / 4U + \
     9U) /*!< Maximum length of the ASN.1 DER encoded signature (ECDSA with a 512 bit curve) in the ticket data. */
#define AWDG_RNG_SEED_LENGTH (48U) /*!< Length of the random seed. */
#define AWDG_HASH_LENGTH     (64U) /*!< Length of the hash buffer (SHA512). */
#define AWDG_MAX_TICKET_LENGTH \
    (AWDG_TIMEOUT_LENGTH + AWDG_TICKET_MAX_SIGNATURE_LENGTH) /*!< Maximum length a ticket is expected to have. */
/* superuser.com/questions/1023167/can-i-extract-r-and-s-from-an-ecdsa-signature-in-bit-form-and-vica-versa */
#define AWDG_MIN_TICKET_LENGTH (AWDG_TIMEOUT_LENGTH + 6U) /*!< Minimum length a ticket is expected to have. */
#define AWDG_DATA_COMBINED_LENGTH \
    (AWDG_TIMEOUT_LENGTH + AWDG_NONCE_LENGTH) /*!< Length of the combined timeout and nonce data. */

/*!
 * @brief Static memory buffer size for mbedtls.
 */
#define AWDG_MBEDTLS_HEAP_MIN_STATIC_BUFFER_SIZE (15000U)

/*!
 * @brief Return values for AWDG_Init.
 *
 */
typedef enum
{
    kStatus_AWDG_InitInitializedNew           = 1,
    kStatus_AWDG_InitInitializedResumed       = 2,
    kStatus_AWDG_InitInvalidPointer           = -1,
    kStatus_AWDG_InitSeedLengthError          = -2,
    kStatus_AWDG_InitInvalidTimeout           = -3,
    kStatus_AWDG_InitCtrDrbgSeedError         = -4,
    kStatus_AWDG_InitParseEccKeyError         = -5,
    kStatus_AWDG_InitLogicalWatchdogUnitError = -6,
    kStatus_AWDG_InitInvalidTickFrequency     = -7,
    kStatus_AWDG_InitInvalidSavedTimeoutTicks = -8,
    kStatus_AWDG_InitNonceRNGFailed           = -9,
    kStatus_AWDG_InitError                    = -100
} awdg_init_status_t;

/*!
 * @brief Return values for AWDG_ValidateTicket.
 *
 */
typedef enum
{
    kStatus_AWDG_ValidateTicketValidated           = 1,
    kStatus_AWDG_ValidateTicketInvalidPointer      = -1,
    kStatus_AWDG_ValidateTicketDidNotValidate      = -2,
    kStatus_AWDG_ValidateTicketHashingFailed       = -3,
    kStatus_AWDG_ValidateTicketInvalidTicketLength = -4,
    kStatus_AWDG_ValidateTicketRNGDisabled         = -5,
    kStatus_AWDG_ValidateTicketError               = -100
} awdg_validate_ticket_status_t;

/*!
 * @brief Return values for AWDG_DeferWatchdog.
 *
 */
typedef enum
{
    kStatus_AWDG_DeferWatchdogOk             = 1,
    kStatus_AWDG_DeferWatchdogNotAllowed     = -1,
    kStatus_AWDG_DeferWatchdogInvalidTimeout = -2,
    kStatus_AWDG_DeferWatchdogError          = -100
} awdg_defer_watchdog_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initializes the authenticated watchdog timer.
 *
 * This function sets up the internal watchdog unit (timeout, grace period),
 * initializes the RNG with the given seed, loads + parses the public key needed
 * for the signature verification and starts the authenticated watchdog timer.
 * Further, the initial nonce is initialized to a fresh random value,
 * so that an attacker can not determine it.
 *
 * When starting the AWDG the defined timeout ticks (either initial or saved) are
 * increased by one as documented by the internally used LWDGU_KickOne() function.
 *
 * Note that setting up a heap allocator for mbedtls must be done by the user
 * before calling this function. AWDG_MBEDTLS_HEAP_MIN_STATIC_BUFFER_SIZE defines
 * the minimum necessary heap memory.
 *
 * If wasRunning is true, the savedTicksToTimeout value is used for
 * restoring the previous watchdog state. Note that the restoring assumes that
 * the tick frequency stayed the same, else the resumed state is not consistent
 * with the previous one! The caller of this function has to save the watchdog's
 * current ticksToTimeout value manually, if he wants to make use of the restoring
 * functionality. He can retrieve this value using the AWDG_GetRemainingTicks()
 * function.
 *
 * The AWDG_Init function has to be run once in the beginning (before any external
 * agents might use or modify the awdg_t object) and mutually exclusive
 * to all other API functions!
 *
 * Only after a successful execution of the initialization function the other
 * API functions can be used!
 *
 * @startuml
 * start
 *    :ret = kStatus_AWDG_InitError
 *    gs_awdg = 0;
 *    if () then (pRngSeed == NULL || pEccPublicKey == NULL)
 *          :ret = kStatus_AWDG_InitInvalidPointer;
 *    elseif () then (initialTimeoutMs < 1)
 *          :ret = kStatus_AWDG_InitInvalidTimeout;
 *    elseif () then (tickFrequencyHz < 1)
 *          :ret = kStatus_AWDG_InitInvalidTickFrequency;
 *    elseif () then (rngSeedLen != AWDG_RNG_SEED_LENGTH)
 *          :ret = kStatus_AWDG_InitSeedLengthError;
 *    elseif (LWDGU_Init(&gs_awdg.watchdogUnit, gracePeriodTimeoutMs, tickFrequencyHz, &gs_awdg.watchdog, 1U)) then (failed)
 *          :ret = kStatus_AWDG_InitLogicalWatchdogUnitError;
 *    elseif (LWDGU_InitWatchdog(&gs_awdg.watchdogUnit, 0U, initialTimeoutMs)) then (failed)
 *          :ret = kStatus_AWDG_InitLogicalWatchdogUnitError;
 *    elseif (AWDG_SetupMbedtlsInternal(rngSeed, rngSeedLen, eccPublicKey, keyLen, &setupMbetlsRet)) then (failed)
 *          :ret = setupMbedtlsRet;
 *    elseif (mbedtls_ctr_drbg_random(&gs_awdg.ctrDrbg, gs_awdg.nonceTicket, AWDG_NONCE_LENGTH)) then (failed)
 *          :ret = kStatus_AWDG_InitNonceRNGFailed;
 *    elseif () then (wasRunning == true)
 *          if (LWDGU_ChangeTimeoutTicksWatchdog(&gs_awdg.watchdogUnit, 0U, savedTimeoutTicks)) then (failed)
 *              :ret = kStatus_AWDG_InitInvalidSavedTimeoutTicks;
 *          else (else)
 *              :LWDGU_KickOne(&gs_awdg.watchdogUnit, 0)
 *              ret = kStatus_AWDG_InitInitializedResumed;
 *          endif
 *    else (else)
 *          :LWDGU_KickOne(&gs_awdg.watchdogUnit, 0)
 *          ret = kStatus_AWDG_InitInitializedNew;
 *    endif
 *    if () then (ret is error)
 *      if() then (mbedtls was intialized)
 *        :AWDG_FreeMbedtlsInternal();
 *      endif
 *    endif
 *    :return ret;
 * stop
 * @enduml
 *
 * @param[in] initialTimeoutMs Time period in milliseconds after which the AWDG expires if no valid ticket was provided.
 * @param[in] gracePeriodTimeoutMs Grace period in milliseconds, equals time after the watchdog expiry until system reset.
 * @param[in] tickFrequencyHz Frequency at which the AWDG is ticked.
 * @param[in] savedTicksToTimeout Previous counter value. Assumes that the tick frequency stayed the same!
 * @param[in] wasRunning Informs the module that the AWDG was running previously. If so, its internal state is restored
 * using the savedTicksToTimeout value.
 * @param[in] pRngSeed Pointer to a byte array containing the RNG seed.
 * @param[in] rngSeedLen Length of the RNG seed.
 * @param[in] pEccPublicKey Pointer to a byte array containing the ECC public key.
 * @param[in] keyLen Length of the ECC public key.
 * @return An awdg_init_status_t status code.
 * @retval kStatus_AWDG_InitInitializedNew
 * The AWDG was started successfully and the timeout was freshly initialized.
 * @retval kStatus_AWDG_InitInitializedResumed
 * The AWDG was started successfully but it was already running before
 * and therefore its previous state was restored.
 * @retval kStatus_AWDG_InitInvalidPointer
 * A given pointer was invalid (NULL).
 * @retval kStatus_AWDG_InitSeedLengthError
 * There was a problem with the provided RNG seed, in particular the length was invalid.
 * @retval kStatus_AWDG_InitInvalidTimeout
 * The provided timeout was invalid.
 * @retval kStatus_AWDG_InitCtrDrbgSeedError
 * The mbedtls_ctr_drbg_seed() function failed (probably due to mbedtls_custom_entropy_func() failure).
 * @retval kStatus_AWDG_InitParseEccKeyError
 * There was a problem with the provided ECC public key.
 * @retval kStatus_AWDG_InitLogicalWatchdogUnitError
 * LWDGU initialization or watchdog initialization failed.
 * The most probable reason is that the combination of the specified tick frequency and
 * a timeout period would have lead to an internal overflow.
 * @retval kStatus_AWDG_InitInvalidTickFrequency
 * The specified tick frequency was invalid (0?).
 * @retval kStatus_AWDG_InitInvalidSavedTimeoutTick
 * The passed saved timeout ticks were invalid.
 * @retval kStatus_AWDG_InitNonceRNGFailed
 * The generation of the initial nonce failed due to a RNG error.
 */
awdg_init_status_t AWDG_Init(const uint32_t initialTimeoutMs,
                             const uint32_t gracePeriodTimeoutMs,
                             const uint32_t tickFrequencyHz,
                             const uint32_t savedTicksToTimeout,
                             const bool wasRunning,
                             const uint8_t *const pRngSeed,
                             const uint32_t rngSeedLen,
                             const uint8_t *const pEccPublicKey,
                             const uint32_t keyLen);

/*!
 * @brief Ticks the internal watchdog and returns the result.
 *
 * Proxy for LWDGU_Tick() (see relevant documentation in the LWDGU module).
 * This function has to be run mutually exclusive with
 * AWDG_DeferWatchdog() and AWDG_GetRemainingTimoutTicks() as the underlying
 * LWDGU library requires it.
 *
 * @return A lwdg_tick_status_t status code, see the relevant documentation.
 */
lwdg_tick_status_t AWDG_Tick(void);

/*!
 * @brief Gets the current nonce.
 *
 * This function returns the current nonce. The nonce is generated during
 * initialization or after validating a ticket (successful or unsuccessful).
 *
 * Note that this function is disabled permanently if the random number generator faced an
 * error it can not recover from (no fresh nonces anymore).
 *
 * This function has to be executed mutually exclusive with the AWDG_ValidateTicket()
 * and AWDG_DeferWatchdog() function.
 *
 * @startuml
 * start
 *    :ret = 0;
 *    if () then (gs_awdg.isRngDisabled == true || pNonce == NULL)
 *       :ret = 0;
 *    else (else)
 *       :nonce = gs_awdg.nonceTicket
 *       ret = AWDG_NONCE_LENGTH;
 *    endif
 *    :return ret;
 * stop
 * @enduml
 *
 * @param[out] pNonce Pointer to result buffer for nonce (must be at least AWDG_NONCE_LENGTH bytes long).
 * @return size_t Number of bytes written to nonce. 0 if the RNG is disabled or a given pointer was invalid (NULL).
 */
size_t AWDG_GetNonce(uint8_t *const pNonce);

/*!
 * @brief Validates the ticket.
 *
 * This function verifies the signature of the provided ticket w.r.t to the the
 * current nonce. The AWDG_DeferWatchdog() function must be called directly afterwards
 * to extend the timer! The nonce is invalidated after the verification attempt,
 * so that a fresh ticket has to be put for the next verification.
 *
 * Note that this function is disabled permanently if the random number generator faced an
 * error it can not recover from (no fresh nonces anymore).
 *
 * This function has to be executed mutually exclusive with the AWDG_GetNonce() and
 * AWDG_DeferWatchdog() function!
 *
 * @startuml
 * start
 *    :ret = kStatus_AWDG_ValidateTicketError
 *    gs_awdg.canDefer = false;
 *
 *    if () then (gs_awdg.isRngDisabled == true)
 *       :ret = kStatus_AWDG_ValidateTicketRNGDisabled;
 *    else (else)
 *      if () then (pTicket == NULL)
 *          :ret = kStatus_AWDG_ValidateTicketInvalidPointer;
 *      elseif () then (gs_awdg.ticketLatchedLen not in [AWDG_MIN_TICKET_LENGTH, AWDG_MAX_TICKET_LENGTH])
 *          :ret = kStatus_AWDG_ValidateTicketInvalidTicketLength;
 *      else (else)
 *           :signatureLen = ticketLen - AWDG_TIMEOUT_LENGTH
 *          signature = &pTicket + AWDG_TICKET_SIGNATURE_INDEX
 *          timeout = first uint32_t of *pTicket
 *          gs_awdg.data = timeout | gs_awdg.nonceTicket;
 *          if (mbedtls_sha512_ret(gs_awdg.data, AWDG_DATA_COMBINED_LENGTH, gs_awdg.hashBuffer, 0)) then (failed)
 *              :ret = kStatus_AWDG_ValidateTicketHashingFailed;
 *          elseif (mbedtls_pk_verify(&gs_awdg.pk, MBEDTLS_MD_SHA512, gs_awdg.hashBuffer, AWDG_HASH_LENGTH, signature, signatureLen)) then (failed)
 *              :ret = kStatus_AWDG_ValidateTicketDidNotValidate;
 *          else (else)
 *              :gs_awdg.deferralTimeoutMs = timeout
 *              gs_awdg.canDefer = true
 *              ret = kStatus_AWDG_ValidateTicketValidated;
 *          endif
 *      endif
 *      if (mbedtls_ctr_drbg_random(&gs_awdg.ctrDrbg, gs_awdg.nonceTicket, AWDG_NONCE_LENGTH)) then (failed)
 *          :gs_awdg.isRngDisabled = true;
 *      endif
 *    endif
 *    :return ret;
 * stop
 * @enduml
 *
 * @param[in] pTicket Pointer to the buffer holding the ticket.
 * @param[in] ticketLen The ticket's size.
 * @return An awdg_validate_ticket_status_t status code.
 * @retval kStatus_AWDG_ValidateTicketValidated
 * The signature validated. The timer can be extended using AWDG_DeferWatchdog().
 * @retval kStatus_AWDG_ValidateTicketInvalidPointer
 * A given pointer was invalid (NULL).
 * @retval kStatus_AWDG_ValidateTicketDidNotValidate
 * The signature did not validate.
 * @retval kStatus_AWDG_ValidateTicketHashingFailed
 * Hashing of the data failed.
 * @retval kStatus_AWDG_ValidateTicketInvalidTicketLength
 * Ticket had an invalid length.
 * @retval kStatus_AWDG_ValidateTicketRNGDisabled
 * The function failed because the RNG has been disabled due to an unrecoverable error.
 */
awdg_validate_ticket_status_t AWDG_ValidateTicket(const uint8_t *const pTicket, const size_t ticketLen);

/*!
 * @brief Extends the timer if the previous validation attempt was successful.
 *
 * This function has to be executed mutually exclusive with the AWDG_GetNonce(),
 * AWDG_ValidateTicket(), AWDG_Tick() and AWDG_GetRemainingTimeoutTicks() function.
 *
 * @startuml
 * start
 *    :ret = kStatus_AWDG_DeferWatchdogError;
 *    if () then (gs_awdg.canDefer == false)
 *       :ret = kStatus_AWDG_DeferWatchdogNotAllowed;
 *    elseif (LWDGU_ChangeTimeoutTimeMsWatchdog(&gs_awdg.watchdogUnit, 0U, gs_awdg.deferralTimeoutMs)) then (failed)
 *       :ret = kStatus_AWDG_DeferWatchdogInvalidTimeout;
 *    else (else)
 *       :LWDGU_KickOne(&gs_awdg.watchdogUnit, 0U)
 *       ret = kStatus_AWDG_DeferWatchdogOk;
 *    endif
 *   :gs_awdg.canDefer = false
 *   return ret;
 * stop
 * @enduml
 *
 * @return An awdg_defer_watchdog_status_t status code.
 * @retval kStatus_AWDG_DeferWatchdogOk
 * Success, the watchdog was deferred.
 * @retval kStatus_AWDG_DeferWatchdogNotAllowed
 * The previous validation attempt was not successful.
 * @retval kStatus_AWDG_DeferWatchdogInvalidTimeout
 * The timeout specified in the ticket could not be processed because it would
 * have lead to an internal overflow.
 */
awdg_defer_watchdog_status_t AWDG_DeferWatchdog(void);

/*!
 * @brief Returns the remaining ticks until the AWDG expires.
 *
 * Proxy for LWDGU_GetRemainingTicksWatchdog() (see relevant LWDGU documentation).
 * This function has to be run mutually exclusive with
 * AWDG_Tick() and AWDG_DeferWatchdog() as the underlying LWDGU library requires it.
 *
 * @return uint32_t representing the remaining ticks until the AWDG expires.
 */
uint32_t AWDG_GetRemainingTicks(void);

#endif /* _AWDG_API_H_ */

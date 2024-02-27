/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "api_qmc_common.h"
#include "api_rpc.h"
#include "fsl_common.h"
#include "qmc_features_config.h"
#include "api_usermanagement.h"
#include "api_board.h"
#include "api_logging.h"
#include "api_configuration.h"
#include "webservice/constant_time.h"
#include "webservice/json_string.h"
#include "webservice/base64url.h"
#include "webservice/json_api_common.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"
#include "core_json.h"
#include "se_mbedtls/se_mbedtls.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifndef STATIC_ASSERT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)
#else
#define STATIC_ASSERT(...)
#endif

#ifdef __CDT_PARSER__
#define _Static_assert(...)
#endif
#endif

#define HISTORY_HASH_COUNT (CONFIG_MAX_VALUE_LEN / USRMGMT_USER_SECRET_LENGTH)

static int s_usermanagement_entropy_init = -1;
static mbedtls_entropy_context s_usermanagement_entropy;

/**
 * @brief Register the secure element as an entropy source
 *
 * @return mbedtls error code
 */
static inline int init_usermanagement_entropy(void)
{
    if (s_usermanagement_entropy_init)
    {
        mbedtls_entropy_init(&s_usermanagement_entropy);
        s_usermanagement_entropy_init = mbedtls_entropy_add_se_source(&s_usermanagement_entropy);
    }
    return s_usermanagement_entropy_init;
}

STATIC_ASSERT(sizeof(usrmgmt_user_config_t) <= CONFIG_MAX_VALUE_LEN, "usrmgmt_user_config_t > CONFIG_MAX_VALUE_LEN");
/*!
 * @brief Character classes for passphrase policies
 */
typedef enum _usermgmt_character_classes
{
    USRMGMT_CLASS_INVALID   = 0x00U,
    USRMGMT_CLASS_UPPERCASE = 0x01U,
    USRMGMT_CLASS_LOWERCASE = 0x02U,
    USRMGMT_CLASS_NUMBERS   = 0x04U,
    USRMGMT_CLASS_SPECIAL   = 0x08U,
    USRMGMT_CLASS_CONTROL   = 0x10U,
    USRMGMT_CLASS_NON_ASCII = 0x20U,
} usermgmt_character_classes_t;

/*
 * localy map the enum constants to names used in the configuration options
 */

#define UPPERCASE | USRMGMT_CLASS_UPPERCASE
#define LOWERCASE | USRMGMT_CLASS_LOWERCASE
#define NUMBERS   | USRMGMT_CLASS_NUMBERS
#define SPECIAL   | USRMGMT_CLASS_SPECIAL
#define CONTROL   | USRMGMT_CLASS_CONTROL
#define NON_ASCII | USRMGMT_CLASS_NON_ASCII

/*
 * Given the macros defined in the configuration, create the bit masks for checking the character classes
 */
#define USRMGMT_REQUIRED_CLASSES (0xFF & (0 USRMGMT_PASSPHRASE_REQUIRED_CLASSESS()))
#define USRMGMT_REJECTED_CLASSES (0xFF & (0 USRMGMT_PASSPHRASE_REJECTED_CLASSESS()))

#define USRMGMT_PASSWORD_REJECTED_CLASSES (USRMGMT_CLASS_SPECIAL | USRMGMT_CLASS_CONTROL | USRMGMT_CLASS_NON_ASCII)

#define JWT_HS256

#ifdef JWT_HS256
#define JWT_SIGNATURE_SIZE 32
#define JWT_TOKEN_MAC      MBEDTLS_MD_SHA256

/*!
 * define the header for the jwt.
 * the key id field is added for looking up the key used for authentication
 */
#define JWT_HEADER_FMT         "{\"alg\":\"HS256\",\"typ\":\"JWT\",\"kid\":%u}"
#define JWT_HEADER_LEN         (sizeof(JWT_HEADER_FMT) + 7)
#define JWT_ENCODED_HEADER_LEN BASE64_ENCODED_LENGTH(JWT_HEADER_LEN)

/* bytes needed in addition to payload */
#define TOKEN_RESERVE_BYTES    3
#define TOKEN_MIN_PAYLOAD_SIZE 2

#else
#error "JWT token type not defined"
#endif

/*! @brief  return max(0, v) */
#define LOWER_LIMIT_ZERO(v) ((v) & -(0 < (v)))

/* counter for remaining account unlock attempts */
/* used if an account is freshly locked to allow some trials of reauthentication */
static char usrmgmt_authentication_trial_counter[kCONFIG_Key_UserLast - kCONFIG_Key_UserFirst] = {0};

typedef struct _user_session_state
{
    unsigned char session_secret[USRMGMT_SESSION_SECRET_LENGTH]; /*!< session secret */
    usrmgmt_session_t session;
} user_session_state_t;

static user_session_state_t s_user_session[USRMGMT_MAX_SESSIONS] = {0};

/* key material buffer (passwords) */
static unsigned char s_usrmgmt_password_buffer[USRMGMT_PASSWORD_BUFFER_LENGTH] = {0};
/* static unsigned char s_usrmgmt_payload_buffer[USRMGMT_PAYLOAD_BUFFER_LENGTH]   = {0}; */

/*!
 * @brief constant time character classifier
 *
 * returns usermgmt_character_classes_t - the classification of c
 *
 * @param[in] c character to classify
 */
static usermgmt_character_classes_t ct_character_classifier(const char c)
{
    int result = USRMGMT_CLASS_INVALID;
    result |= uint8_ct_rangecheck(0, ' ' - 1, c) & USRMGMT_CLASS_CONTROL;
    result |= uint8_ct_rangecheck(' ', '/', c) & USRMGMT_CLASS_SPECIAL;
    result |= uint8_ct_rangecheck('0', '9', c) & USRMGMT_CLASS_NUMBERS;
    result |= uint8_ct_rangecheck(':', '@', c) & USRMGMT_CLASS_SPECIAL;
    result |= uint8_ct_rangecheck('A', 'Z', c) & USRMGMT_CLASS_UPPERCASE;
    result |= uint8_ct_rangecheck('[', '`', c) & USRMGMT_CLASS_SPECIAL;
    result |= uint8_ct_rangecheck('a', 'z', c) & USRMGMT_CLASS_LOWERCASE;
    result |= uint8_ct_rangecheck('{', '~', c) & USRMGMT_CLASS_SPECIAL;
    result |= uint8_ct_rangecheck(127, 127, c) & USRMGMT_CLASS_CONTROL;
    result |= uint8_ct_rangecheck(128, 255, c) & USRMGMT_CLASS_NON_ASCII;
    return result;
}

/*!
 * @brief constant time password buffer classifier
 *
 * returns a int with the usermgmt_character_classes_t classification bits set.
 * always categorizes the whole buffer of USRMGMT_PASSWORD_BUFFER_LENGTH
 * only returns the categorization up to the first null byte.
 *
 * @param[in] password password buffer array
 */

static int ct_password_classifier(unsigned char password[USRMGMT_PASSWORD_BUFFER_LENGTH])
{
    unsigned char *p = password;
    int result       = USRMGMT_CLASS_INVALID;
    int term         = 0xFF;
    while (p < password + USRMGMT_PASSWORD_BUFFER_LENGTH)
    {
        term &= (uint8_ct_rangecheck(1, 255, *p));
        result |= (term & ct_character_classifier(*p++));
    }
    return result;
}

/*!
 * @brief constant time username comparator
 *
 * returns 0 if the arrays' contents are equal up to the first nullbyte
 *
 * @param[in] first password buffer array
 * @param[in] second password buffer array
 *
 * @return 0 if the strings are equal
 */

static unsigned char ct_username_compare(unsigned char first[USRMGMT_USER_NAME_MAX_LENGTH],
                                         unsigned char second[USRMGMT_USER_NAME_MAX_LENGTH])
{
    unsigned char *f    = first;
    unsigned char *s    = second;
    unsigned int result = 0;
    unsigned int term   = 0xff;
    while (f < first + USRMGMT_USER_NAME_MAX_LENGTH)
    {
        result |= uint8_ct_rangecheck(1, 255, (*f & term) ^ (*s & term));
        term &= uint8_ct_rangecheck(1, 255, *f++) & term;
        term &= uint8_ct_rangecheck(1, 255, *s++) & term;
    }
    return result;
}

static inline config_id_t session_uid(usrmgmt_session_id sid)
{
    if (sid >= 0 && sid < ARRAY_SIZE(s_user_session))
    {
        return s_user_session[sid].session.uid;
    }
    else
    {
        return kCONFIG_KeyNone;
    }
};

/*!
 * @brief look and fetch for a user by name
 *
 * returns the config_id_t of the user object.
 * might be an empty slot id if the user is not found, or kCONFIG_KeyNone if no free slots exist.
 *
 * @param[in] name username
 * @param[out] config buffer for the usrmgmt_user_config_t data
 */

static config_id_t get_user_by_name(const unsigned char *name, usrmgmt_user_config_t *config)
{
    static unsigned char username[USRMGMT_USER_NAME_MAX_LENGTH + 1] = {0};
    config_id_t config_id[2]                                        = {kCONFIG_KeyNone, kCONFIG_Key_UserFirst};
    qmc_status_t rc;
    config_id_t sid, canditate_id = kCONFIG_KeyNone;
    unsigned char slot = 0x1;
    strncpy((char *)&username[0], (const char *)name, sizeof(username));
    for (sid = kCONFIG_Key_UserFirst; sid <= kCONFIG_Key_UserLast; sid++)
    {
        rc = CONFIG_GetBinValueById(sid, (unsigned char *)config, sizeof(*config));
        if (canditate_id == kCONFIG_KeyNone && rc == kStatus_QMC_Ok && config->role <= kUSRMGMT_RoleEmpty)
        {
            canditate_id = sid; /*nothing stored, candidate for not found */
            // timing wise this leaks the existence of a free user slot
        }
        config_id[slot] = sid;
        // this will change slots on successful comparison (return value zero)
        slot &= ct_username_compare(username, config->name);
    }
    // load the match or the last user record again.
    rc = CONFIG_GetBinValueById(config_id[1], (unsigned char *)config, sizeof(*config));

    // store any canditate_id to slot 0
    config_id[0] = canditate_id;

    // bit invert the slot for the result.
    // if slot is 0 the result is in slot 1
    // if slot is 1 the candidate is in slot 0
    slot = (~slot) & 0x1;

    // return the last value stored to slot 1 or the candidate
    return config_id[slot];
}

/**
 * @brief create the JWT payload json string
 *
 * @param dst       destination buffer pointer
 * @param size      size of destination buffer
 * @param username  username
 * @param iat       issued at time
 * @param exp       expiry time
 * @param sid       session id
 * @param role      user role
 *
 * @return length of the buffer space used
 */
static inline size_t write_claims(unsigned char *dst,
                                  size_t size,
                                  const unsigned char *username,
                                  uint64_t iat,
                                  uint64_t exp,
                                  int sid,
                                  usrmgmt_role_t role)
{
    char *const start = (char *)dst;
    char *const e     = start + size;
    char *s           = (char *)start;

    const char *deviceid = BOARD_GetDeviceIdentifier();
    const char *rolestr;

    switch (role)
    {
        case kUSRMGMT_RoleOperator:
            rolestr = "operator";
            break;
        case kUSRMGMT_RoleMaintenance:
            rolestr = "maintenance";
            break;
        default:
            /* abort otherwise, do not create a token*/
            return 0;
            break;
    }

    if (s < e)
    {
        s += snprintf(s, e - s, "{\"sid\":%d,\"iat\":\"%s\"", sid, uint64toa(iat));
    }
    if (s < e)
    {
        s += snprintf(s, e - s, ",\"exp\":\"%s\",\"role\":\"%s\",\"iss\":", uint64toa(exp), rolestr);
    }
    if (s < e)
    {
        if (deviceid)
        {
            s += json_quote_string(s, e - s, deviceid);
        }
        else
        {
            {
                s += snprintf(s, e - s, "null");
            }
        }
    }
    if (s < e)
    {
        s += snprintf(s, e - s, ",\"sub\":");
    }
    if (s < e)
    {
        {
            s += json_quote_string(s, e - s, (char *)username);
        }
    }
    if (s < e)
    {
        {
            *s++ = '}';
        }
    }
    return (s < e) ? s - start : 0;
}

/**
 * @brief calculate an hmac
 *
 * @param dst      destination of the hmac result
 * @param p        pointer to calculate the hmac for
 * @param plen     length of the data p points to
 * @param key      pointer to the key used for the hmac
 * @param keylen   length of the key material
 * @param type     hmac algorithm type
 *
 * @return  mbedtls error code or -1 in case of an error, 0 on success
 */
static int hmac_sha(unsigned char *dst,
                    unsigned const char *p,
                    uint32_t plen,
                    const unsigned char *key,
                    uint32_t keylen,
                    mbedtls_md_type_t type)
{
    int err = -1;
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);

    do
    {
        if ((err = mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(type), 1)))
        {
            {
                break;
            }
        }
        if ((err = mbedtls_md_hmac_starts(&ctx, key, keylen)))
        {
            {
                break;
            }
        }
        if ((err = mbedtls_md_hmac_update(&ctx, (const unsigned char *)p, plen)))
        {
            {
                break;
            }
        }
        if ((err = mbedtls_md_hmac_finish(&ctx, dst)))
        {
            break;
        }
        err = 0;
    } while (0);
    mbedtls_md_free(&ctx);
    return err;
}

/**
 * @brief write the signature for a jwt
 *
 * @param dst         output buffer
 * @param size        size of output buffer
 * @param data        payload pointer
 * @param len         payload length
 * @param secret      secret key pointer
 * @param secret_len  secret key length
 *
 * @return size of the written signature string
 */
size_t add_token_signature_mac(
    unsigned char *dst, size_t size, unsigned char *data, size_t len, unsigned char *secret, size_t secret_len)
{
    unsigned char signature[JWT_SIGNATURE_SIZE];
    int rc;
    size_t encoded_len;

    rc = hmac_sha(signature, data, len, secret, secret_len, JWT_TOKEN_MAC);

    if (rc == 0)
    {
        rc = base64url_encode(dst, size, &encoded_len, signature, JWT_SIGNATURE_SIZE);
    }
    return (rc ? 0 : encoded_len);
}

/**
 * @brief assemble a JWT token
 *
 * @param ptr         output buffer
 * @param size        size of output buffer
 * @param secret      pointer to the secret
 * @param secret_len  size of the secret
 * @param sid         session id
 * @param payload     pointer to the JWT payload string
 * @param payload_len size of the payload string
 *
 * @return size of the token created
 */
size_t jwt_build_token(unsigned char *ptr,
                       size_t size,
                       unsigned char *secret,
                       size_t secret_len,
                       usrmgmt_session_id sid,
                       unsigned char *payload,
                       size_t payload_len)
{
    size_t result               = 0;
    unsigned char *const header = ptr;
    unsigned char *p            = ptr;
    unsigned char *pe           = (ptr + size) - TOKEN_RESERVE_BYTES;

    if (p < pe)
    {
        size_t len;
        unsigned char jwt_header[JWT_HEADER_LEN];
        len = snprintf((char *)jwt_header, size, JWT_HEADER_FMT, sid);
        if (len <= sizeof(jwt_header))
        {
            int rc = base64url_encode(p, pe - p, &len, jwt_header, len);
            if (rc == 0)
            {
                {
                    p += len;
                }
            }
        }
    }
    if (p > header && p < pe)
    {
        *p++ = '.', pe++; /* reserved byte 1 */
        /* write plaintext payload */
        if (payload_len)
        {
            size_t encoded_len;
            if (p + BASE64_ENCODED_LENGTH(payload_len) < pe)
            {
                int rc = base64url_encode(p, pe - p, &encoded_len, payload, payload_len);
                if (rc == 0)
                {
                    p += encoded_len;
                    /* sign header + '.' + payload, but not the second '.' */
                    if (p + BASE64_ENCODED_LENGTH(JWT_SIGNATURE_SIZE) < pe)
                    {
                        size_t signature_data_len = p - header;
                        *p++                      = '.', pe++; /* reserved byte 2 */
                        p += add_token_signature_mac(p, pe - p, header, signature_data_len, secret, secret_len);
                        *p++   = '\0', pe++; /* reserved byte 3 */
                        result = p - header;
                    }
                }
            }
        }
    }
    return result;
}

/*******************************************************************************
 * API
 ******************************************************************************/

qmc_status_t USRMGMT_AddUser(usrmgmt_session_id current_session,
                             const unsigned char *name,
                             const unsigned char *passphrase,
                             usrmgmt_role_t role)
{
    usrmgmt_user_config_t config;
    config_id_t uid  = kCONFIG_KeyNone;
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    size_t passlen;
    if (init_usermanagement_entropy())
    {
        rc = kStatus_QMC_Err;
    }
    else if (name && passphrase && role > kUSRMGMT_RoleEmpty && strlen((char *)name) < USRMGMT_USER_NAME_MAX_LENGTH &&
             (passlen = strlen((char *)passphrase)) < USRMGMT_PASSWORD_BUFFER_LENGTH)
    {
        uint8_t classification;
        /* check username for invalid characters */
        strncpy((char *)s_usrmgmt_password_buffer, (char *)name, sizeof(s_usrmgmt_password_buffer));
        classification = 0xFF & ct_password_classifier(s_usrmgmt_password_buffer);
        mbedtls_platform_zeroize(s_usrmgmt_password_buffer, sizeof(s_usrmgmt_password_buffer));
        if (USRMGMT_PASSWORD_REJECTED_CLASSES & classification)
        {
            return kStatus_QMC_ErrRange;
        }

        uid = get_user_by_name(name, &config);
        if (uid == kCONFIG_KeyNone)
        {
            rc = kStatus_QMC_ErrMem;
        }
        else
        {
            /* get the password history id */
            const config_id_t hid = uid - kCONFIG_Key_UserFirst + kCONFIG_Key_UserHashesFirst;

            /* empty template to erase the history */
            static const unsigned char pw_hist_zero[HISTORY_HASH_COUNT][USRMGMT_USER_SECRET_LENGTH] = {0};

            /* set rule and name */
            config.role = role;
            strncpy((char *)&config.name[0], (const char *)name, sizeof(config.name));

            if (mbedtls_entropy_func(&s_usermanagement_entropy, config.salt, sizeof(config.salt)) == 0)
            {
                rc = CONFIG_SetBinValueById(uid, (const unsigned char *)&config, sizeof(config));
            }
            else
            {
                rc = kStatus_QMC_Err;
            }
            if (rc == kStatus_QMC_Ok)
            {
                /* clear old password history */
                (void)CONFIG_SetBinValueById(hid, (const unsigned char *)&pw_hist_zero, sizeof(pw_hist_zero));

                /* update password metadata */
                rc = USRMGMT_UpdateUser(USRMGMT_NO_CURRENT_SESSION, uid, passphrase, role);
                if (rc != kStatus_QMC_Ok)
                {
                    /* free user config  on error */
                    config = (usrmgmt_user_config_t){.role = kUSRMGMT_RoleEmpty};
                    (void)CONFIG_SetBinValueById(uid, (const unsigned char *)&config, sizeof(config));
                }
                else
                {
                    if (current_session != USRMGMT_NO_CURRENT_SESSION)
                    {
                        const log_record_t entry = {
                            .type                   = kLOG_UsrMgmt,
                            .data.usrMgmt.source    = LOG_SRC_UsrMgmt,
                            .data.usrMgmt.category  = LOG_CAT_Authentication,
                            .data.usrMgmt.eventCode = LOG_EVENT_UserCreated,
                            .data.usrMgmt.user      = session_uid(current_session),
                            .data.usrMgmt.subject   = uid,
                        };
                        LOG_QueueLogEntry(&entry, false);
                    }
                }
            }
        }
    }
    return rc;
}

qmc_status_t USRMGMT_UpdateUser(usrmgmt_session_id current_session,
                                config_id_t uid,
                                const unsigned char *passphrase,
                                usrmgmt_role_t role)
{
    usrmgmt_user_config_t config;
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    uint8_t classification;
    size_t passlen;
    qmc_timestamp_t timestamp;

    /* limit to valid user roles */
    switch (role)
    {
        case kUSRMGMT_RoleMaintenance:
            /*fallthrough*/
        case kUSRMGMT_RoleOperator:
            break;
        default:
            role = kUSRMGMT_RoleNone;
    }

    /* validate parameters */
    if (passphrase && role > kUSRMGMT_RoleEmpty &&
        (passlen = strlen((char *)passphrase)) < USRMGMT_PASSWORD_BUFFER_LENGTH)
    {
        rc = CONFIG_GetBinValueById(uid, (unsigned char *)&config, sizeof(config));
        if (rc == kStatus_QMC_Ok && config.role > kUSRMGMT_RoleEmpty)
        {
            /* copy to buffer */
            mbedtls_platform_zeroize(s_usrmgmt_password_buffer, sizeof(s_usrmgmt_password_buffer));
            strncpy((char *)s_usrmgmt_password_buffer, (char *)passphrase, sizeof(s_usrmgmt_password_buffer));
            classification = 0xFF & ct_password_classifier(s_usrmgmt_password_buffer);

            /* classes set in rules and classification */
            uint8_t in_both = USRMGMT_REQUIRED_CLASSES & classification;

            /* remove common bits from rules, leaves any bits required missing from classification and add bits that are
             * rejected */
            /* 0 if and only if all rules are matched */
            uint8_t violated = ((USRMGMT_REQUIRED_CLASSES ^ in_both) | (USRMGMT_REJECTED_CLASSES & classification));

            rc = kStatus_QMC_Err;

            if (!violated && passlen >= USRMGMT_MIN_PASSPHRASE_LENGTH)
            {
                /* passphrase matches the requirements */

                mbedtls_md_context_t hmac_md_ctx;
                const mbedtls_md_info_t *hmac_md_info;
                unsigned char pw_history[HISTORY_HASH_COUNT][USRMGMT_USER_SECRET_LENGTH];
                unsigned char hash[USRMGMT_USER_SECRET_LENGTH];

                mbedtls_md_init(&hmac_md_ctx);

                do
                {
                    int i,j;
                    uint8_t diff;
                    bool pw_reused        = false;
                    const config_id_t hid = uid - kCONFIG_Key_UserFirst + kCONFIG_Key_UserHashesFirst;
                    hmac_md_info          = mbedtls_md_info_from_type(USRMGMT_PASSPHRASE_HASH);
                    if (hmac_md_info == NULL)
                    {
                        {
                            break;
                        }
                    }
                    if (mbedtls_md_setup(&hmac_md_ctx, hmac_md_info, 1) != 0)
                    {
                        {
                            break;
                        }
                    }
                    config.iterations        = USRMGMT_MIN_PASSPHRASE_ITERATIONS;
                    config.role              = role;
                    config.lockout_timestamp = 0;
                    switch (BOARD_GetTime(&timestamp))
                    {
                        case kStatus_QMC_Ok:
                            config.validity_timestamp = timestamp.seconds + USRMGMT_PASSPHRASE_DURATION;
                            break;
                        default:
                            config.validity_timestamp = USRMGMT_PASSPHRASE_DURATION;
                            break;
                    }

                    if (mbedtls_pkcs5_pbkdf2_hmac(&hmac_md_ctx, passphrase, passlen, config.salt, sizeof(config.salt),
                                                  config.iterations, sizeof(hash), hash) != 0)
                    {
                        break;
                    }

                    rc = CONFIG_GetBinValueById(hid, (unsigned char *)pw_history, sizeof(pw_history));
                    if (rc != kStatus_QMC_Ok)
                    {
                        break;
                    }
                    for (j = 0,diff=0; j < USRMGMT_USER_SECRET_LENGTH; j++)
                    {
                        diff |= 0xff & ( hash[j] ^ config.secret[j]);
                    }
                    pw_reused = !diff;
                    for (i = 0; !pw_reused && i < HISTORY_HASH_COUNT; i++)
                    {
                        for (j = 0,diff=0; j < USRMGMT_USER_SECRET_LENGTH; j++)
                        {
                            diff |= 0xff & ( hash[j] ^ pw_history[i][j]);
                        }
                        pw_reused = !diff;
                    }
                    if (pw_reused)
                    {
                        rc = kStatus_QMC_Err;
                        break;
                    }
                    memmove(pw_history[0], pw_history[1], (HISTORY_HASH_COUNT - 1) * USRMGMT_USER_SECRET_LENGTH);
                    memcpy(pw_history[HISTORY_HASH_COUNT - 1], config.secret, USRMGMT_USER_SECRET_LENGTH);
                    memcpy(config.secret, hash, USRMGMT_USER_SECRET_LENGTH);
                    /* store user info */
                    rc = CONFIG_SetBinValueById(uid, (const unsigned char *)&config, sizeof(config));
                    if (rc == kStatus_QMC_Ok)
                    {
                        if (current_session != USRMGMT_NO_CURRENT_SESSION)
                        {
                            const log_record_t entry = {
                                .type                   = kLOG_UsrMgmt,
                                .data.usrMgmt.source    = LOG_SRC_UsrMgmt,
                                .data.usrMgmt.category  = LOG_CAT_Authentication,
                                .data.usrMgmt.eventCode = LOG_EVENT_UserUpdate,
                                .data.usrMgmt.user      = session_uid(current_session),
                                .data.usrMgmt.subject   = uid,
                            };
                            LOG_QueueLogEntry(&entry, false);
                        }
                        rc = CONFIG_SetBinValueById(hid, (const unsigned char *)&pw_history, sizeof(pw_history));
                    }
                    if (rc == kStatus_QMC_Ok)
                    {
                        rc = CONFIG_UpdateFlash();
                    }
                } while (0);

                mbedtls_md_free(&hmac_md_ctx);
                mbedtls_platform_zeroize(&hash, sizeof(hash));
                mbedtls_platform_zeroize(&pw_history, sizeof(pw_history));
                mbedtls_platform_zeroize(&config, sizeof(config));
            }
            /* wipe password buffer */
            mbedtls_platform_zeroize(s_usrmgmt_password_buffer, sizeof(s_usrmgmt_password_buffer));
        }
        else
        {
          rc = kStatus_QMC_ErrArgInvalid;
        }
    }
    return rc;
}

qmc_status_t USRMGMT_RemoveUser(usrmgmt_session_id current_session, const unsigned char *name)
{
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    config_id_t id  = kCONFIG_KeyNone;
    usrmgmt_user_config_t config;
    static const unsigned char pw_history_zero[HISTORY_HASH_COUNT][USRMGMT_USER_SECRET_LENGTH] = {0};
    id = get_user_by_name(name, &config);
    if (id != kCONFIG_KeyNone && config.role >= kUSRMGMT_RoleEmpty)
    {
        usrmgmt_session_id sid;
        const config_id_t hid = id - kCONFIG_Key_UserFirst + kCONFIG_Key_UserHashesFirst;
        for (sid = 0; sid < USRMGMT_MAX_SESSIONS; sid++)
        {
            if (s_user_session[sid].session.uid == id)
            {
                USRMGMT_EndSession(current_session, sid);
            }
        }
        memset(&config, 0, sizeof(config));
        config.role = kUSRMGMT_RoleEmpty;
        rc          = CONFIG_SetBinValueById(id, (const unsigned char *)&config, sizeof(config));
        if (rc == kStatus_QMC_Ok)
        {
            if (current_session != USRMGMT_NO_CURRENT_SESSION)
            {
                const log_record_t entry = {
                    .type                   = kLOG_UsrMgmt,
                    .data.usrMgmt.source    = LOG_SRC_UsrMgmt,
                    .data.usrMgmt.category  = LOG_CAT_Authentication,
                    .data.usrMgmt.eventCode = LOG_EVENT_UserRemoved,
                    .data.usrMgmt.user      = session_uid(current_session),
                    .data.usrMgmt.subject   = id,
                };
                LOG_QueueLogEntry(&entry, false);
            }
            rc = CONFIG_SetBinValueById(hid, (const unsigned char *)&pw_history_zero, sizeof(pw_history_zero));
        }
        if (rc == kStatus_QMC_Ok)
        {
            rc = CONFIG_UpdateFlash();
        }
    }
    return rc;
}

/*!
 * @brief Check session timeouts, end session if timed out
 *
 * @param[in] sid id of the session
 */
static void USRMGMT_TimeoutSession(usrmgmt_session_id sid, qmc_timestamp_t *timestamp)
{
    if (s_user_session[sid].session.role > kUSRMGMT_RoleEmpty &&
        (s_user_session[sid].session.exp <= timestamp->seconds || s_user_session[sid].session.iat > timestamp->seconds))
    {
        USRMGMT_EndSession(-1, sid);
        const log_record_t entry = {
            .type                   = kLOG_UsrMgmt,
            .data.usrMgmt.source    = LOG_SRC_UsrMgmt,
            .data.usrMgmt.category  = LOG_CAT_Authentication,
            .data.usrMgmt.eventCode = LOG_EVENT_SessionTimeout,
            .data.usrMgmt.user      = kCONFIG_KeyNone,
            .data.usrMgmt.subject   = sid,
        };
        LOG_QueueLogEntry(&entry, false);
    }
}

qmc_status_t USRMGMT_LockUser(const unsigned char *name, uint64_t reactivation_timestamp)
{
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    config_id_t uid  = kCONFIG_KeyNone;
    usrmgmt_user_config_t config;
    qmc_timestamp_t timestamp;
    uid = get_user_by_name(name, &config);
    if (uid != kCONFIG_KeyNone && config.role >= kUSRMGMT_RoleEmpty)
    {
        switch (BOARD_GetTime(&timestamp))
        {
            case kStatus_QMC_Ok:
                if (reactivation_timestamp < timestamp.seconds)
                {
                    {
                        reactivation_timestamp = timestamp.seconds;
                    }
                }
                break;
            default:
                break;
        }
        bool is_already_locked   = config.lockout_timestamp >= timestamp.seconds;
        config.lockout_timestamp = reactivation_timestamp;
        rc                       = CONFIG_SetBinValueById(uid, (const unsigned char *)&config, sizeof(config));
        if (rc == kStatus_QMC_Ok)
        {
            rc = CONFIG_UpdateFlash();
        }
        const log_record_t entry = {
            .type                       = kLOG_DefaultData,
            .data.defaultData.source    = LOG_SRC_UsrMgmt,
            .data.defaultData.category  = LOG_CAT_Authentication,
            .data.defaultData.eventCode = LOG_EVENT_AccountSuspended,
            .data.defaultData.user      = uid,
        };
        if (!is_already_locked)
        {
            LOG_QueueLogEntry(&entry, false);
        }
    }
    return rc;
}

qmc_status_t USRMGMT_UnlockUser(const unsigned char *name)
{
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    config_id_t uid  = kCONFIG_KeyNone;
    usrmgmt_user_config_t config;
    uid = get_user_by_name(name, &config);
    if (uid != kCONFIG_KeyNone && config.role >= kUSRMGMT_RoleEmpty && config.lockout_timestamp > 0)
    {
        config.lockout_timestamp = 0;
        rc                       = CONFIG_SetBinValueById(uid, (const unsigned char *)&config, sizeof(config));
        if (rc == kStatus_QMC_Ok)
        {
            rc = CONFIG_UpdateFlash();
        }
        const log_record_t entry = {
            .type                       = kLOG_DefaultData,
            .data.defaultData.source    = LOG_SRC_UsrMgmt,
            .data.defaultData.category  = LOG_CAT_Authentication,
            .data.defaultData.eventCode = LOG_EVENT_AccountResumed,
            .data.defaultData.user      = uid,
        };
        LOG_QueueLogEntry(&entry, false);
    }
    return rc;
}
#if FEATURE_SECURE_WATCHDOG  
#define IS_SECURE_WATCHDOG_EXPIRED (fw_update_state == kFWU_AwdtExpired)
#else
#define IS_SECURE_WATCHDOG_EXPIRED false
#endif
qmc_status_t USRMGMT_CreateSession(unsigned char *token,
                                   size_t *token_size,
                                   usrmgmt_session_id *sid,
                                   usrmgmt_session_t **session,
                                   const unsigned char *name,
                                   const unsigned char *passphrase)
{
    qmc_status_t rc = kStatus_QMC_Err;
    config_id_t uid = kCONFIG_KeyNone;
#if FEATURE_SECURE_WATCHDOG  
    qmc_fw_update_state_t fw_update_state;
#endif
    usrmgmt_user_config_t config;
    qmc_timestamp_t timestamp = {0};
    usrmgmt_session_id id     = -1;
    int idx;
    if (sid)
    {
        *sid = -1;
    }
    if (session)
    {
        *session = NULL;
    }

    uid = get_user_by_name(name, &config);
    if (uid != kCONFIG_KeyNone && config.role >= kUSRMGMT_RoleEmpty)
    {
        switch (BOARD_GetTime(&timestamp))
        {
            case kStatus_QMC_Ok:
                break;
            default:
                /* clock not initialized deny non-supervisor users */
                if (config.role != kUSRMGMT_RoleMaintenance)
                {
                    config.lockout_timestamp = ~0;
                }
                break;
        }
#if FEATURE_SECURE_WATCHDOG  
        if (kStatus_QMC_Ok != RPC_GetFwUpdateState(&fw_update_state))
        {
            fw_update_state = kFWU_AwdtExpired;
        }
#endif
        /* check if account has expired / supervisor accounts work even when expired */
        if ((config.validity_timestamp >= timestamp.seconds && !IS_SECURE_WATCHDOG_EXPIRED) ||
            config.role == kUSRMGMT_RoleMaintenance)
        {
            /* reserve superuser sessions */
            idx = (config.role == kUSRMGMT_RoleMaintenance) ? 0 : USRMGMT_RESERVED_SESSIONS;
            for (; idx < USRMGMT_MAX_SESSIONS; idx++)
            {
                user_session_state_t *uss = &s_user_session[idx];
                USRMGMT_TimeoutSession(idx, &timestamp);
                if (uss->session.uid == kCONFIG_KeyNone && id < 0)
                {
                    id = idx;
                }
                if (uss->session.uid == uid)
                {
                    id = idx;
                    break;
                }
            }
        }

        /* there is a suitable session slot, and the user may current_session */
        /* (account unlocked or has tries remaining)*/
        /* accounts get locked after the first invalid authentication request*/

        if (id >= 0 && ((config.lockout_timestamp <= timestamp.seconds) ||
                        (usrmgmt_authentication_trial_counter[uid - kCONFIG_Key_UserFirst] > 0)))
        {
            mbedtls_md_context_t hmac_md_ctx;
            const mbedtls_md_info_t *hmac_md_info;
            usrmgmt_authentication_trial_counter[uid - kCONFIG_Key_UserFirst] =
                MAX(0, usrmgmt_authentication_trial_counter[uid - kCONFIG_Key_UserFirst] - 1);
            mbedtls_md_init(&hmac_md_ctx);

            unsigned char secret[USRMGMT_SESSION_SECRET_LENGTH];

            do
            {
                hmac_md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
                if (hmac_md_info == NULL)
                {
                    break;
                }
                if (mbedtls_md_setup(&hmac_md_ctx, hmac_md_info, 1) != 0)
                {
                    break;
                }

                if (mbedtls_pkcs5_pbkdf2_hmac(&hmac_md_ctx, passphrase, strlen((char *)passphrase), config.salt,
                                              sizeof(config.salt), config.iterations, sizeof(secret), secret) != 0)
                {
                    break;
                }

                int i;
                unsigned int diff = 0;
                /* compare hashes (pbkdf2 keys)*/
                for (i = 0; i < sizeof(secret); i++)
                {
                    diff |= 0xff & (secret[i] ^ config.secret[i]);
                }
                mbedtls_platform_zeroize(secret, sizeof(secret));

                if (diff == 0)
                {
                    /* copy session data */
                    user_session_state_t *uss = &s_user_session[id];
                    /* if this is a new session generate a secret */
                    if (uss->session.uid != uid)
                    {
                        if (init_usermanagement_entropy())
                        {
                            rc = kStatus_QMC_Err;
                            break;
                        }
                        /*generate session secret*/
                        if (mbedtls_entropy_func(&s_usermanagement_entropy, uss->session_secret,
                                                 sizeof(uss->session_secret)) != 0)
                        {
                            break;
                        }
                    }
                    
                    if (config.lockout_timestamp)
                    {
                        USRMGMT_UnlockUser(config.name);
                        CONFIG_GetBinValueById(uid, (unsigned char*)&config, sizeof(config));
                    }

                    rc                = kStatus_QMC_Ok;
                    uss->session.uid  = uid;
                    uss->session.sid  = id;
                    uss->session.role = config.role;
                    uss->session.exp  = timestamp.seconds + USRMGMT_SESSION_DURATION;
                    uss->session.iat  = timestamp.seconds;
                    if (sid)
                    {
                        *sid = id;
                    }
                    if (session)
                    {
                        *session = &uss->session;
                    }
                    if (token_size && token)
                    {
                        size_t clen;
                        clen = write_claims(uss->session.token_payload, sizeof(uss->session.token_payload), name,
                                            uss->session.iat, uss->session.exp, id, config.role);
                        size_t tlen;
                        if (!(tlen =
                                  jwt_build_token(token, *token_size, uss->session_secret,
                                                  USRMGMT_SESSION_SECRET_LENGTH, id, uss->session.token_payload, clen)))
                        {
                            USRMGMT_EndSession(-1, id);
                            rc = kStatus_QMC_ErrMem;
                        }
                        *token_size              = tlen;
                        const log_record_t entry = {
                            .type                       = kLOG_DefaultData,
                            .data.defaultData.source    = LOG_SRC_UsrMgmt,
                            .data.defaultData.category  = LOG_CAT_Authentication,
                            .data.defaultData.eventCode = LOG_EVENT_UserLogin,
                            .data.defaultData.user      = uid,
                        };
                        LOG_QueueLogEntry(&entry, false);
                    }
                }
            } while (0);
            mbedtls_md_free(&hmac_md_ctx);
        }
    }
    if (id > 0 && rc != kStatus_QMC_Ok)
    {
        const log_record_t entry = {
            .type                       = kLOG_DefaultData,
            .data.defaultData.source    = LOG_SRC_UsrMgmt,
            .data.defaultData.category  = LOG_CAT_Authentication,
            .data.defaultData.eventCode = LOG_EVENT_LoginFailure,
            .data.defaultData.user      = uid,
        };
        /* but allow some attempts to reauthenticate if locked account was not already locked */
        if (config.lockout_timestamp < timestamp.seconds)
        {
            usrmgmt_authentication_trial_counter[uid - kCONFIG_Key_UserFirst] = USRMGMT_AUTHENTICATION_ATTEMPTS;
            /* the login failure is only logged if ther is no lock on the account already */
            LOG_QueueLogEntry(&entry, false);
        }
        /* lock the account after the first failed authentication, update timeout otherwise */
        USRMGMT_LockUser(name, timestamp.seconds + USRMGMT_LOCKOUT_DURATION);
    }
    return rc;
}

qmc_status_t USRMGMT_EndSession(usrmgmt_session_id current_session, usrmgmt_session_id sid)
{
    qmc_status_t rc = kStatus_QMC_Err;
    if (sid >= 0 && sid < USRMGMT_MAX_SESSIONS)
    {
        config_id_t uid = s_user_session[sid].session.uid;
        mbedtls_platform_zeroize(&s_user_session[sid], sizeof(s_user_session[0]));
        rc = kStatus_QMC_Ok;
        if (uid)
        {
            log_record_t entry;
            if (current_session == sid)
            {
                entry = (log_record_t){
                    .type                       = kLOG_DefaultData,
                    .data.defaultData.source    = LOG_SRC_UsrMgmt,
                    .data.defaultData.category  = LOG_CAT_Authentication,
                    .data.defaultData.eventCode = LOG_EVENT_UserLogout,
                    .data.defaultData.user      = uid,
                };
            }
            else
            {
                entry = (log_record_t){
                    .type                   = kLOG_UsrMgmt,
                    .data.usrMgmt.source    = LOG_SRC_UsrMgmt,
                    .data.usrMgmt.category  = LOG_CAT_Authentication,
                    .data.usrMgmt.eventCode = LOG_EVENT_TerminateSession,
                    .data.usrMgmt.subject   = uid,
                    .data.usrMgmt.user      = session_uid(current_session),
                };
            }
            LOG_QueueLogEntry(&entry, false);
        };
    }
    return rc;
}

qmc_status_t USRMGMT_IterateUsers(int *count, config_id_t *uid, usrmgmt_user_config_t *pconfig)
{
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    if (count && uid && pconfig)
    {
        if (*count == 0)
        {
            *uid = kCONFIG_Key_UserFirst;
        }
        else
        {
            *uid = *uid + 1;
        }
        *count = *count + 1;
        while (*uid <= kCONFIG_Key_UserLast)
        {
            rc = CONFIG_GetBinValueById(*uid, (unsigned char *)pconfig, sizeof(*pconfig));
            if (rc == kStatus_QMC_Ok)
            {
                break;
            }
            rc     = kStatus_QMC_ErrRange;
            *count = *count + 1;
            *uid   = *uid + 1;
        }
    }
    return rc;
}

qmc_status_t USRMGMT_IterateSessions(int *count, usrmgmt_session_id *sid, const usrmgmt_session_t **session)
{
    qmc_timestamp_t timestamp;
    qmc_status_t rc = kStatus_QMC_ErrArgInvalid;
    if (BOARD_GetTime(&timestamp) == kStatus_QMC_Ok)
    {
        if (count && sid && session)
        {
            if (*count == 0)
            {
                *sid = 0;
            }
            else
            {
                *sid += 1;
            }
            *count += 1;
            while (*sid < USRMGMT_MAX_SESSIONS)
            {
                USRMGMT_TimeoutSession(*sid, &timestamp);
                if (s_user_session[*sid].session.uid != kCONFIG_KeyNone)
                {
                    rc       = kStatus_QMC_Ok;
                    *session = &s_user_session[*sid].session;
                    break;
                }
                rc = kStatus_QMC_ErrRange;
                *count += 1;
                *sid += 1;
            }
        }
    }
    return rc;
}

#define USRMGMT_KID_QUERY "kid"
#define USRMGMT_SID_QUERY "id"
#define USRMGMT_EXP_QUERY "exp"

qmc_status_t USRMGMT_ValidateSession(unsigned char *token,
                                     size_t token_size,
                                     usrmgmt_session_id *sid,
                                     const usrmgmt_session_t **session)
{
    static unsigned char signature[JWT_SIGNATURE_SIZE];
    static unsigned char jwt_header[JWT_HEADER_LEN];
    static unsigned char hmac[JWT_SIGNATURE_SIZE];
    qmc_status_t result = kStatus_QMC_Err;

    unsigned char *pe      = token + token_size;
    unsigned char *header  = token;
    unsigned char *payload = NULL;
    unsigned char *sig     = NULL;
    size_t size            = 0;
    
    if(sid)
    {
      *sid                          = -1;
    }
    usrmgmt_session_id session_id = -1;
    do
    {
        unsigned char c;
        unsigned char *dot = header;
        int i;
        uint8_t diff = 0;
        JSONTypes_t jtype;
        char *value_str;
        size_t value_len;
        user_session_state_t *uss;
        unsigned long long exp;
        qmc_timestamp_t timestamp;

        /* locate next dot separator */
        while ((c = *(++dot)) && c && c != '.' && dot < pe)
        {
            ;
        }
        if (*dot != '.')
        {
            break;
        }
        payload = ++dot;
        while ((c = *(++dot)) && c && c != '.' && dot < pe)
        {
            ;
        }
        if (*dot != '.')
        {
            break;
        }
        sig = ++dot;
        if (sig >= pe)
        {
            break;
        }
        if (base64url_decode(signature, sizeof(signature), &size, sig, (pe - sig)) != 0)
        {
            break;
        }
        if (size != sizeof(signature))
        {
            break;
        }
        /* decode header */
        if (base64url_decode(jwt_header, sizeof(jwt_header), &size, header, (payload - header) - 1) != 0)
        {
            break;
        }
        if (JSONSuccess != JSON_Validate((char *)jwt_header, size))
        {
            break;
        }
        /* lookup the kid */
        if (JSONSuccess !=
            JSON_SearchT((char *)jwt_header, size, SIZED(USRMGMT_KID_QUERY), &value_str, &value_len, &jtype))
        {
            break;
        }
        if (jtype != JSONNumber)
        {
            break;
        }
        if (value_len > 5)
        {
            break;
        }
        if (!value_str)
        {
            break;
        }
        if (*value_str == '-')
        {
            break;
        }
        session_id = 0;

        /* get the key/session id */
        while (value_len--)
        {
            session_id = session_id * 10 + (*value_str++ - '0');
        }
        if (session_id < 0 || session_id >= USRMGMT_MAX_SESSIONS)
        {
            break;
        }
        uss = &s_user_session[session_id];
        /* calculate hmac */
        if (hmac_sha(hmac, header, (sig - header) - 1, uss->session_secret, sizeof(uss->session_secret), JWT_TOKEN_MAC))
        {
            break;
        }

        /* sum(or) up hmac differences */
        for (i = 0; i < sizeof(hmac); i++)
        {
            diff |= signature[i] ^ hmac[i];
        }
        /* get rid of the hash */
        mbedtls_platform_zeroize(hmac, sizeof(hmac));
        /* if diff != 0 there was a difference */
        if (diff != 0)
        {
            break;
        }
        /* if not, authentication is successful */
        if (base64url_decode(uss->session.token_payload, sizeof(uss->session.token_payload), &size, payload,
                             (sig - payload) - 1) != 0)
        {
            break;
        }
        if (size >= sizeof(uss->session.token_payload))
        {
            break;
        }
        if (JSONSuccess != JSON_Validate((char *)uss->session.token_payload, size))
        {
            break;
        }
        /* lookup the ext time */
        if (JSONSuccess != JSON_SearchT((char *)uss->session.token_payload, size, SIZED(USRMGMT_EXP_QUERY), &value_str,
                                        &value_len, &jtype))
        {
            break;
        }
        if (jtype != JSONString)
        {
            break;
        }
        if (!value_str)
        {
            break;
        }
        if (*value_str == '-')
        {
            break;
        }
        exp = strntoull(value_str, value_len, NULL, 10);
        if (kStatus_QMC_Ok != BOARD_GetTime(&timestamp))
        {
            break;
        }
        if (exp < timestamp.seconds)
        {
            break;
        }
        result = kStatus_QMC_Ok;
        /* optionally return the session */
        if (sid)
        {
            *sid = session_id;
        }
        if (session)
        {
            *session = &uss->session;
        }
    } while (0);
    return result;
}

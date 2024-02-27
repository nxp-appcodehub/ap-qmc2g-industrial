/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_USRMGMT_H_
#define _API_USRMGMT_H_

#include "api_configuration.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief value for the current_session arguments, if no log message is to be created.
 */
#define USRMGMT_NO_CURRENT_SESSION (-1)


/*!
 * @brief Session ID type; derived from int.
 */
typedef int usrmgmt_session_id;

/*!
 * @brief User roles for role-based access control.
 */
typedef enum _usrmgmt_role
{
    kUSRMGMT_RoleNone  = 0U, /*!< User doesn't have a role e.g. user isn't registered or authentication failed. */
    kUSRMGMT_RoleEmpty = 1U, /*!< Pseudo role used by the user management to indicate free user configuration slots. */
    kUSRMGMT_RoleMaintenance = 0x555AU, /*!< Maintenance Head */
    kUSRMGMT_RoleOperator    = 0x5A55U, /*!< Operator */
    kUSRMGMT_RoleLocalSd =
        0xAAA5U, /*!< User that is not authenticated cryptographically, but via a mechanical key. This role is used for
                    logging SD card related activities e.g. insert/remove SD card or open the SD card lid. */
    kUSRMGMT_RoleLocalButton =
        0xAA5AU, /*!< User that is not authenticated cryptographically, but via a mechanical key. This role is used for
                    logging activities related to the user buttons e.g. starting/stopping a motor. */
    kUSRMGMT_RoleLocalEmergency = 0xA5AAU, /*!< Used for logging emergency events (emergency stop button). User has not
                                              undergone authentication. */
} usrmgmt_role_t;

/*!
 * @brief Represents an active user session
 */
typedef struct _usrmgmt_session
{
    usrmgmt_session_id sid;                                     /*!< user session id */
    config_id_t    uid;                                         /*!< user configuration id */
    usrmgmt_role_t role;                                        /*!< session role, string in JWT token */
    uint64_t iat;                                               /*!< issued at time */
    uint64_t exp;                                               /*!< expiration time */
    unsigned char token_payload[USRMGMT_PAYLOAD_BUFFER_LENGTH]; /*!< token payload */
} usrmgmt_session_t;

/*!
 * @brief Represents user specific configuration data
 */
typedef struct _usrmgmt_user_config
{
    unsigned char name[USRMGMT_USER_NAME_MAX_LENGTH]; /*!< user name */
    usrmgmt_role_t role;                              /*!< session role */
    uint64_t lockout_timestamp;                       /*!< timestamp of automatic reactivation */
    unsigned int iterations;                          /*!< PBKDF2 iteration count for this entry */
    unsigned char salt[USRMGMT_SALT_LENGTH];          /*!< passphrase salt, length is tunable */
    unsigned char secret[USRMGMT_USER_SECRET_LENGTH]; /*!< password hash, length is tunable */
    uint64_t validity_timestamp;                      /*!< timestamp end of account validity*/
} usrmgmt_user_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Adds a new user to the system.
 *
 * May return kStatus_QMC_ErrMem, if no memory for a new user is available or kStatus_QMC_ErrArgInvalid, if name already
 * exists and kStatus_QMC_ErrRange if the username contains invalid characters. If adding the user was successful.
 * configuration in flash will be updated.
 *
 * @param[in] current_session active user session 
 * @param[in] name Pointer to the user name. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be read.
 * @param[in] passphrase Pointer to the passphrase. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be
 * read.
 * @param[in] role Role of the new user
 */
qmc_status_t USRMGMT_AddUser(usrmgmt_session_id current_session,const unsigned char *name, const unsigned char *passphrase, usrmgmt_role_t role);

/*!
 * @brief update users passphrase and role
 *
 * May return kStatus_QMC_ErrArgInvalid, uid is not valid.
 *
 * @param[in] current_session active user session 
 * @param[in] uid the user id
 * @param[in] passphrase Pointer to the passphrase. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be
 * read.
 * @param[in] role Role of the new user
 * configuration in flash will be updated.
 */
qmc_status_t USRMGMT_UpdateUser(usrmgmt_session_id current_session,config_id_t uid, const unsigned char *passphrase, usrmgmt_role_t role);


/*!
 * @brief Removes a user from the system.
 *
 * May return kStatus_QMC_ErrArgInvalid if the user indicated by name doesn't exist. If removing the user was
 * successful, configuration in flash must be updated to prevent further access to the system. Also, the corresponding
 * user role entry of the configuration shall be set to kUSRMGMT_RoleEmpty, to indicate that this slot is available for
 * a new user registration.
 *
 * @param[in] current_session active user session 
 * @param[in] name Pointer to the user name. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be read.
 */
qmc_status_t USRMGMT_RemoveUser(usrmgmt_session_id current_session,const unsigned char *name);

/*!
 * @brief Lock a user for a given time. The user credentials cannot be used for current_session during this time.
 *
 * @param[in] name name of the user to lock
 * @param[in] reactivation_timestamp time until the user should remain in locked state
 */
qmc_status_t USRMGMT_LockUser(const unsigned char *name, uint64_t reactivation_timestamp);

/*!
 * @brief Unlock user credentials that are in the locked state.
 *
 * @param[in] name name of the user to unlock
 */
qmc_status_t USRMGMT_UnlockUser(const unsigned char *name);

/*!
 * @brief Create a new session.
 *
 * At least one of token, sid must be provided to create a session. If not, only the passphrase is checked, no session
 * is created.
 *
 * @param[out]    token_buffer: Buffer for encoded JWT token string, null terminated - optional, may be NULL.
 * @param[in,out] token_size in: token_buffer size, out: used size not including null byte, optional may be NULL.
 * @param[out]    sid pointer to session id variable - optional, may be NULL.
 * @param[out]    session pointer to session struct pointer - optional, may be NULL.
 * @param[in]     name: the user to authenticate
 * @param[in]     passphrase: passphrase of the user to authenticate
 * @return kStatus_QMC_ErrNoBufs if token space is to small
 * @return kStatus_QMC_Ok if session is created
 * @return kStatus_QMC_Err otherwise
 */
qmc_status_t USRMGMT_CreateSession(unsigned char *token_buffer,
                                   size_t *token_size,
                                   usrmgmt_session_id *sid,
                                   usrmgmt_session_t **session,
                                   const unsigned char *name,
                                   const unsigned char *passphrase);

/*!
 * @brief Terminate an active session.
 *
 * @param[in] current_session active user session 
 * @param[in] sid ID of the session to terminate
 * @return kStatus_QMC_Ok if deleted
 * @return kStatus_QMC_Err otherwise
 */
qmc_status_t USRMGMT_EndSession(usrmgmt_session_id current_session,usrmgmt_session_id sid);

/*!
 * @brief Validate a session using a token.
 *
 * the pointer to the decoded payload is only valid to the next call of this function.
 *
 * @param[in] token Pointer to session token. 
 * @param[in] token_size in: token size.
 * @param[out]    sid pointer to write session id to
 * @param[out]    session pointer to a usrmgmt_session_t pointer, for access to the session struct.
 * @return kStatus_QMC_Ok if session was successfully validated using the given token
 * @return kStatus_QMC_Err otherwise
 */
qmc_status_t USRMGMT_ValidateSession(unsigned char *token,
                                     size_t token_size,
                                     usrmgmt_session_id *sid,
                                     const usrmgmt_session_t **session);
/*!
 * @brief iterate users
 *
 * @param[in,out] pcount pointer to a iteration counter initialized to 0
 * @param[out] uid pointer to config id,
 * @param[out] uid pointer to a usrmgmt_user_config_t struct
 * @return kStatus_QMC_Ok if a session was found
 * @return kStatus_QMC_ErrRange if no further session exists
 */
qmc_status_t USRMGMT_IterateUsers(int *count, config_id_t *uid, usrmgmt_user_config_t *config);

/*!
 * @brief iterate active sessions
 *
 * @param[in,out] count pointer to a iteration counter initialized to 0
 * @param[out] sid pointer to session id,
 * @param[out] session pointer to session
 * @return kStatus_QMC_Ok if a session was found
 * @return kStatus_QMC_ErrRange if no further session exists
 */
qmc_status_t USRMGMT_IterateSessions(int *count, usrmgmt_session_id *sid, const usrmgmt_session_t **session);
#endif /* _API_USRMGMT_H_ */

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _API_USRMGMT_H_
#define _API_USRMGMT_H_

#include "api_configuration.h"

#define USRMGMT_SESSION_SECRET_LENGTH   (32)
#define USRMGMT_USER_SECRET_LENGTH      (32)
#define USRMGMT_SALT_LENGTH             (16)
#define USRMGMT_USER_NAME_MAX_LENGTH    (32)
#define USRMGMT_PASSWORD_BUFFER_LENGTH (255)


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief User roles for role-based access control.
 */
typedef enum _usrmgmt_role
{
    kUSRMGMT_RoleNone           = 0U,      /*!< User doesn't have a role e.g. user isn't registered or authentication failed. */
    kUSRMGMT_RoleEmpty          = 1U,      /*!< Pseudo role used by the user management to indicate free user configuration slots. */
    kUSRMGMT_RoleMaintenance    = 0x555AU, /*!< Maintenance Head */
    kUSRMGMT_RoleSupervisor     = 0x55A5U, /*!< Production Supervisor */
    kUSRMGMT_RoleOperator       = 0x5A55U, /*!< Operator */
    kUSRMGMT_RoleLocalSd        = 0xAAA5U, /*!< User that is not authenticated cryptographically, but via a mechanical key. This role is used for logging SD card related activities e.g. insert/remove SD card or open the SD card lid. */
    kUSRMGMT_RoleLocalButton    = 0xAA5AU, /*!< User that is not authenticated cryptographically, but via a mechanical key. This role is used for logging activities related to the user buttons e.g. starting/stopping a motor. */
    kUSRMGMT_RoleLocalEmergency = 0xA5AAU, /*!< Used for logging emergency events (emergency stop button). User has not undergone authentication. */
} usrmgmt_role_t;


/*!
 * @brief Character classes for passphrase policies
 */
typedef enum _usermgmt_character_classes
{
	IAM_CLASS_INVALID   = 0x00U,
	IAM_CLASS_UPPERCASE = 0x01U,
	IAM_CLASS_LOWERCASE = 0x02U,
	IAM_CLASS_NUMBERS   = 0x04U,
	IAM_CLASS_SPECIAL   = 0x08U,
	IAM_CLASS_CONTROL   = 0x10U,
	IAM_CLASS_NON_ASCII = 0x20U,
} usermgmt_character_classes_t;

/*!
 * @brief Represents an active user session
 */
typedef struct _usrmgmt_session
{
	unsigned char session_secret[USRMGMT_SESSION_SECRET_LENGTH]; /*!< session secret */
	usrmgmt_role_t role;                                         /*!< session role, string in JWT token */
	uint64_t       iat;                                          /*!< issued at time */
	uint64_t       exp;                                          /*!< expiration time */
} usrmgmt_session_t;

/*!
 * @brief Represents user specific configuration data
 */
typedef struct _usrmgmt_user_config
{
	unsigned char  name[USRMGMT_USER_NAME_MAX_LENGTH]; /*!< user name */
	usrmgmt_role_t role;                               /*!< session role */
	uint64_t       lockout_timestamp;                  /*!< timestamp of account reactivation */
	unsigned int   iterations;                         /*!< PBKDF2 iteration count for this entry */
	unsigned char  salt[USRMGMT_SALT_LENGTH];          /*!< passphrase salt, length is tunable */
	unsigned char  secret[USRMGMT_USER_SECRET_LENGTH]; /*!< password hash, length is tunable */
} usrmgmt_user_config_t;

/*!
 * @brief Session ID type; derived from integer.
 */
typedef int qmc_session_id;



/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Adds a new user to the system.
 *
 * May return kStatus_QMC_ErrMem, if no memory for a new user is available or kStatus_QMC_ErrArgInvalid, if name already exists. If adding the user was successful, configuration in flash should be updated.
 *
 * @param[in] passphrase_rules Passphrase rules to check the supplied passphrase against.
 * @param[in] name Pointer to the user name. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be read.
 * @param[in] passphrase Pointer to the passphrase. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be read.
 * @param[in] role Role of the new user
 */
qmc_status_t USRMGMT_AddUser(uint8_t passphrase_rules, const unsigned char* name, const unsigned char* passphrase, usrmgmt_role_t role);

/*!
 * @brief Removes a user from the system.
 *
 * May return kStatus_QMC_ErrArgInvalid if the user indicated by name doesn't exist. If removing the user was successful,
 * configuration in flash must be updated to prevent further access to the system. Also, the corresponding user role entry
 * of the configuration shall be set to kUSRMGMT_RoleEmpty, to indicate that this slot is available for a new user registration.
 *
 * @param[in] name Pointer to the user name. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN will be read.
 */
qmc_status_t USRMGMT_RemoveUser(const unsigned char* name);

/*!
 * @brief Lock a user for a given time. The user credentials cannot be used for login during this time.
 *
 * @param[in] name name of the user to lock
 * @param[in] reactivation_timestamp time until the user should remain in locked state
 */
qmc_status_t USRMGMT_LockUser(unsigned char* name, uint64_t reactivation_timestamp);

/*!
 * @brief Unlock user credentials that are in the locked state.
 *
 * @param[in] name name of the user to unlock
 */
qmc_status_t USRMGMT_UnlockUser(unsigned char* name);

/*!
 * @brief Create a new session.
 *
 * At least one of token, sid must be provided to create a session. If not, only the passphrase is checked, no session is created.
 *
 * @param[out]    token Buffer for encoded JWT token string, null terminated - optional, may be NULL.
 * @param[in,out] token_size in: buffer size, out: used size not including null byte, may be NULL.
 * @param[out]    sid pointer to session id variable - optional, may be NULL.
 * @param[in]     name user name
 * @param[in]     passphrase user passphrase
 * @return kStatus_QMC_ErrNoBufs if token space is to small
 * @return kStatus_QMC_Ok if session is created
 * @return kStatus_QMC_Err otherwise
 */
qmc_status_t USRMGMT_CreateSession(unsigned char* token, size_t *token_size, qmc_session_id *sid, const unsigned char* name, const unsigned char* passphrase);

/*!
 * @brief Terminate an active session.
 *
 * @param[in] sid ID of the session to terminate
 * @return kStatus_QMC_Ok if deleted
 * @return kStatus_QMC_Err otherwise
 */
qmc_status_t USRMGMT_EndSession(qmc_session_id sid);

/*!
 * @brief Validate a session using a token.
 *
 * @param[in,out] token in: Token to be used for session validation. out: token payload
 * @param[in,out] token_size in: token size. out: payload size
 * @param[out]    sid pointer to write session id to
 * @return kStatus_QMC_Ok if session was successfully validated using the given token
 * @return kStatus_QMC_Err otherwise
 */
qmc_status_t USRMGMT_ValidateSession(unsigned char* token, size_t *token_size, qmc_session_id *sid);

#endif /* _API_USRMGMT_H_ */

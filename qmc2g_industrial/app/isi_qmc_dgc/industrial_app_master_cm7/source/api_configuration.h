/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_CONFIG_H_
#define _API_CONFIG_H_

#include "api_qmc_common.h"
#include "stdbool.h"
#include "flash_recorder.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief List of valid configuration keys (IDs)
 */
typedef enum _config_id
{
    kCONFIG_KeyNone              = 0x00U, /*!< Represents an invalid configuration key. */
    kCONFIG_Key_Cloud1Parameters = 0x01U, /*!< Connection string for Cloud Service 1. */
    kCONFIG_Key_Cloud2Parameters = 0x02U, /*!< Connection string for Cloud Service 2. */
    kCONFIG_Key_Ip = 0x03U,
    kCONFIG_Key_Ip_mask = 0x04U,
    kCONFIG_Key_Ip_GW = 0x05U,
    kCONFIG_Key_Ip_DNS = 0x06U,
    kCONFIG_Key_MAC_address = 0x07U,
    kCONFIG_Key_VLAN_ID = 0x08U,
    kCONFIG_Key_TSN_RX_STREAM_MAC_ADDR = 0x09U,
    kCONFIG_Key_TSN_TX_STREAM_MAC_ADDR = 0x0AU,
	kCONFIG_Key_CloudAzureHubName = 0x0BU,
	kCONFIG_Key_CloudGenericHostName = 0x0DU,
	kCONFIG_Key_CloudGenericUserName = 0x0EU,
	kCONFIG_Key_CloudGenericPassword = 0x0FU,
	kCONFIG_Key_CloudGenericDeviceName = 0x10U,
	kCONFIG_Key_CloudGenericPort = 0x11U,

    kCONFIG_Key_MOTD              = 0x70U, /*!< System Usage Message */
    kCONFIG_Key_User1              = 0x80U, /*!< first user config */
    kCONFIG_Key_User2              = 0x81U, 
    kCONFIG_Key_User3              = 0x82U, 
    kCONFIG_Key_User4              = 0x83U,
    kCONFIG_Key_User5              = 0x84U,
    kCONFIG_Key_User6              = 0x85U,
    kCONFIG_Key_User7              = 0x86U,
    kCONFIG_Key_User8              = 0x87U,
    kCONFIG_Key_User9              = 0x88U,
    kCONFIG_Key_User10             = 0x89U,
    kCONFIG_Key_UserHashes1              = 0x90U, /*!< first user config */
    kCONFIG_Key_UserHashes2              = 0x91U, 
    kCONFIG_Key_UserHashes3              = 0x92U, 
    kCONFIG_Key_UserHashes4              = 0x93U,
    kCONFIG_Key_UserHashes5              = 0x94U,
    kCONFIG_Key_UserHashes6              = 0x95U,
    kCONFIG_Key_UserHashes7              = 0x96U,
    kCONFIG_Key_UserHashes8              = 0x97U,
    kCONFIG_Key_UserHashes9              = 0x98U,
    kCONFIG_Key_UserHashes10             = 0x99U,
} config_id_t;


#define kCONFIG_Key_UserFirst kCONFIG_Key_User1
#define kCONFIG_Key_UserLast  kCONFIG_Key_User10
#define kCONFIG_Key_UserHashesFirst kCONFIG_Key_UserHashes1
#define kCONFIG_Key_UserHashesLast  kCONFIG_Key_UserHashes10

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Get the value indicated by "key".
 *
 * If key doesn't exist, return kStatus_QMC_ErrRange. If the configuration section does not store a changed value, the default value shall be used instead.
 *
 * @param[in]  key Pointer to the key string. At most CONFIG_MAX_KEY_LEN bytes are read.
 * @param[out] value Pointer to write the result string to. Must be at least CONFIG_MAX_VALUE_LEN in size.
 */
qmc_status_t CONFIG_GetStrValue(const unsigned char* key, unsigned char* value);

/*!
 * @brief Get the value indicated by id.
 *
 * If id doesn't exist, return kStatus_QMC_ErrRange. If the configuration section does not store a changed value, the default value shall be used instead.
 *
 * @param[in]  id ID of of the corresponding value to retrieve
 * @param[out] value Pointer to write the result string to. Must be at least CONFIG_MAX_VALUE_LEN in size.
 */
qmc_status_t CONFIG_GetStrValueById(config_id_t id, unsigned char* value);

/*!
 * @brief Set the new value for the given key.
 *
 * This shall not trigger CONFIG_UpdateFlash() : qmc_status_t automatically. If key doesn't exist, return kStatus_QMC_ErrRange. If value is NULL, return kStatus_QMC_ErrArgInvalid.
 *
 * @param[in] key Pointer to the key string. At most CONFIG_MAX_KEY_LEN bytes are read.
 * @param[in] value Pointer to read the value from. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN bytes will be read.
 */
qmc_status_t CONFIG_SetStrValue(const unsigned char* key, const unsigned char* value);

/*!
 * @brief Set the new value for the given id.
 *
 * This shall not trigger CONFIG_UpdateFlash() : qmc_status_t automatically. If id doesn't exist, return kStatus_QMC_ErrRange. If id equals kCONFIG_KEY_NONE or value is NULL, return kStatus_QMC_ErrArgInvalid.
 *
 * @param[in] id ID of of the corresponding value to set
 * @param[in] value Pointer to read the value from. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN bytes will be read.
 */
qmc_status_t CONFIG_SetStrValueById(config_id_t id, const unsigned char* value);
/*!
 * @brief Get the value indicated by "key".
 *
 * If key doesn't exist, return kStatus_QMC_ErrRange. If the configuration section does not store a changed value, the default value shall be used instead.
 *
 * @param[in]  key Pointer to the key string. At most size of data specified in length parameter are read.
 * @param[out] value Pointer to write the result binary data to. Must be at least length in size.
 * @param[in]  length Max size of binary data being read.
 */
qmc_status_t CONFIG_GetBinValue(const unsigned char* key, unsigned char* value, const uint16_t length);

/*!
 * @brief Get the value indicated by id.
 *
 * If id doesn't exist, return kStatus_QMC_ErrRange. If the configuration section does not store a changed value, the default value shall be used instead.
 *
 * @param[in]  id ID of of the corresponding value to retrieve
 * @param[out] value Pointer to write the result binary data to. Must be at least length in size.
 * @param[in]  length Max size of binary data being read.
 */
qmc_status_t CONFIG_GetBinValueById(config_id_t id, unsigned char* value, const uint16_t length);

/*!
 * @brief Set the new value for the given key.
 *
 * This shall not trigger CONFIG_UpdateFlash() : qmc_status_t automatically. If key doesn't exist, return kStatus_QMC_ErrRange. If value is NULL, return kStatus_QMC_ErrArgInvalid.
 *
 * @param[in] key Pointer to the key string. At most CONFIG_MAX_KEY_LEN bytes are read.
 * @param[in] value Pointer to read the value from. At most CONFIG_MAX_VALUE_LEN bytes or size specified in length will be read.
 * @param[in] length Max size of binary data being read.
 */
qmc_status_t CONFIG_SetBinValue(const unsigned char* key, const unsigned char* value, const uint16_t length);

/*!
 * @brief Set the new value for the given id.
 *
 * This shall not trigger CONFIG_UpdateFlash() : qmc_status_t automatically. If id doesn't exist, return kStatus_QMC_ErrRange. If id equals kCONFIG_KEY_NONE or value is NULL, return kStatus_QMC_ErrArgInvalid.
 *
 * @param[in] id ID of of the corresponding value to set
 * @param[in] value Pointer to read the value from. At most CONFIG_MAX_VALUE_LEN bytes or size specified in length will be read.
 * @param[in] length Max size of binary data being read.
 */
qmc_status_t CONFIG_SetBinValueById(config_id_t id, const unsigned char* value, const uint16_t length);

/*!
 * @brief Encrypt the current configuration and write it to the flash.
 */
qmc_status_t CONFIG_UpdateFlash();

/*!
 * @brief Return the config_id_t that matches the given key. If key does not exist, return kCONFIG_KEY_NONE.
 *
 * @param[in] key Pointer to the key string. At most CONFIG_MAX_KEY_LEN bytes are read.
 * @return Matching config_id_t or kCONFIG_KeyNone
 */
config_id_t CONFIG_GetIdfromKey(const unsigned char* key);

/*!
 * @brief Tries to parse an integer from the configuration value retrieved by CONFIG_GetValue(key : unsigned char*, value : unsigned char*) : qmc_status_t or CONFIG_GetValueById(id : config_id_t, value : unsigned char*) : qmc_status_t.
 *
 * @param[in]  value Pointer to read the value string from. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN bytes will be read.
 * @param[out] integer Pointer to write the conversion result to
 */
qmc_status_t CONFIG_GetIntegerFromValue(const unsigned char* value, int* integer);

/*!
 * @brief Convert an integer to a string in order to store it as a configuration value.
 *
 * This value is then stored in the configuration by calling CONFIG_SetValue(key : unsigned char*, value : unsigned char*) : qmc_status_t or CONFIG_SetValueById(id : config_id_t, value : unsigned char*) : qmc_status_t.
 *
 * @param[in]  integer Integer value to be written to the value string
 * @param[in]  valueLen Available size in the value string
 * @param[out] value Pointer to the value string. At most min(valueLen, CONFIG_MAX_VALUE_LEN) bytes will be written.
 */
qmc_status_t CONFIG_SetIntegerAsValue(int integer, size_t valueLen, unsigned char* value);

/*!
 * @brief Tries to parse a boolean from the configuration value retrieved by CONFIG_GetValue(key : unsigned char*, value : unsigned char*) : qmc_status_t or CONFIG_GetValueById(id : config_id_t, value : unsigned char*) : qmc_status_t.
 *
 * Values such as "true"/"false", "yes"/"no", "on"/"off", "1"/"0" shall be recognized by the function.
 *
 * @param[in]  value Pointer to read the value string from. Should be zero terminated. At most CONFIG_MAX_VALUE_LEN bytes will be read.
 * @param[out] boolean Pointer to write the conversion result to
 */
qmc_status_t CONFIG_GetBooleanFromValue(const unsigned char* value, bool* boolean);

/*!
 * @brief Convert a boolean to a string in order to store it as a configuration value.
 *
 * This value is then stored in the configuration by calling CONFIG_SetValue(key : unsigned char*, value : unsigned char*) : qmc_status_t or CONFIG_SetValueById(id : config_id_t, value : unsigned char*) : qmc_status_t.
 *
 * @param[in]  boolean Boolean value to be written to the value string
 * @param[in]  valueLen Available size in the value string
 * @param[out] value Pointer to the value string. At most min(valueLen, CONFIG_MAX_VALUE_LEN) bytes will be written.
 */
qmc_status_t CONFIG_SetBooleanAsValue(bool boolean, size_t valueLen, unsigned char* value);

/*!
 * @brief Write a chunk of data to the firmware update location.
 *
 * @param[in] offset Position to write the chunk of data to
 * @param[in] data Pointer to the data to write
 * @param[in] dataLen Length of the data to write
 */
qmc_status_t CONFIG_WriteFwUpdateChunk(size_t offset, const uint8_t* data, size_t dataLen);

#endif /* _API_CONFIG_H_ */

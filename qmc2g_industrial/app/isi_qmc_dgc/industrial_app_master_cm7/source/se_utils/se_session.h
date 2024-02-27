/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef SECURE_ELEMENT_H_
#define SECURE_ELEMENT_H_

#include "api_qmc_common.h"
#include "fsl_sss_api.h"

/*!
 * @brief Enumeration of object/key IDs stored in the secure element.
 */
enum qmc_se_key_ids {
    idFirmwareMinRevision     = 0x00000001, /*!< file, 4 bytes */
    idManifestMinRevision     = 0x00000002, /*!< file, 4 bytes */
    idOemCaCert               = 0x00000003, /*!< file, <1 kBytes */
    idOemCaPubKey             = 0x00000004, /*!< BrainpoolP512r1 public key */
    idCustomerCaCert          = 0x00000005, /*!< file, <1 kBytes */
    idCustomerCaPubKey        = 0x00000006, /*!< BrainpoolP512r1 public key */
    idLogReaderIdCert         = 0x00000007, /*!< file, <1 kBytes */
    idLogReaderIdPubKey       = 0x00000008, /*!< RSA3072 public key */
    idFwUpdateIssuerIdCert    = 0x00000009, /*!< file, <1 kBytes */
    idFwUpdateIssuerIdPubKey  = 0x0000000A, /*!< BrainpoolP512r1 public key */
    idAwdtSignerIdCert        = 0x0000000B, /*!< file, <1 kBytes */
    idAwdtSignerIdPubKey      = 0x0000000C, /*!< BrainpoolP512r1 public key */
    idFwUpdateCreatorIdCert   = 0x0000000D, /*!< file, <1 kBytes */
    idFwUpdateCreatorIdPubKey = 0x0000000E, /*!< BrainpoolP512r1 public key */
    idCloud1ServerCaCert      = 0x0000000F, /*!< file, <1 kBytes */
    idCloud1ServerCaPubKey    = 0x00000010, /*!< public key, type depends on service used */
    idCloud2ServerCaCert      = 0x00000011, /*!< file, <1 kBytes */
    idCloud2ServerCaPubKey    = 0x00000012, /*!< public key, type depends on service used */
    idDevIdCert               = 0x00000013, /*!< file, <1 kBytes */
    idDevIdKeyPair            = 0x00000014, /*!< BrainpoolP512r1 key pair */
    idAwdtDevIdCert           = 0x00000015, /*!< file, <1 kBytes */
    idAwdtDevIdKeyPair        = 0x00000016, /*!< NISTP-256 key pair */
    idCloud1DevCert           = 0x00000017, /*!< file, <1 kBytes */
    idCloud1DevKeyPair        = 0x00000018, /*!< NISTP-256 key pair */
    idCloud2DevCert           = 0x00000019, /*!< file, <1 kBytes */
    idCloud2DevKeyPair        = 0x0000001A, /*!< NISTP-256 key pair */
    idCertRevocationList      = 0x0000001B, /*!< file */
    idSblAuthObject           = 0x0000001C, /*!< AES128 key */
    idSblAuthObjectFirstRun   = 0x0000001D, /*!< file, 16 bytes */
    idAppAuthObject           = 0x0000001E, /*!< AES128 key */
    idAppAuthObjectFirstRun   = 0x0000001F, /*!< file, 16 bytes */
    idConfigEnc               = 0x00000020, /*!< AES256 key */
    idMetaDataEnc             = 0x00000021, /*!< AES256 key */
    idAwdtServerIdCert        = 0x00000022, /*!< file, <1 kBytes */
    idAwdtServerIdPubKey      = 0x00000023, /*!< NISTP-256 public key */
    idWebServerIdCert         = 0x00000024, /*!< file, <1 kBytes */
    idWebServerIdKeyPair      = 0x00000025, /*!< NISTP-521 key pair */
    idDeviceIdFull            = 0x00000026, /*!< file, 32 bytes */
    idDeviceIdShort           = 0x00000027, /*!< file, <32 bytes */
	idDefaultUser             = 0x00000028, /*!< file */
};

/*!
 * @brief Init SE Hostlib and open SE session
 *
 * Must be called after the scheduler has started
 */
qmc_status_t SE_Initialize(void);

/*!
 * @brief Deinit SE Hostlib and close SE session
 */
void SE_Deinitialize(void);

/*!
 * @brief Checks the status of the SE session
 *
 * @return True if session open else False
 */
bool SE_IsInitialized(void);

/*!
 * @brief Get pointer to SE session
 */
sss_session_t* SE_GetSession(void);

/*!
 * @brief Get pointer to SE key store
 */
sss_key_store_t* SE_GetKeystore(void);

/*!
 * @brief Get pointer to HOST session
 */
sss_session_t* SE_GetHostSession(void);

/*!
 * @brief Get pointer to HOST key store
 */
sss_key_store_t* SE_GetHostKeystore(void);

/*!
 * @brief Get the unique ID of the SE
 */
const char* SE_GetUid(void);

#endif /* SECURE_ELEMENT_H_ */

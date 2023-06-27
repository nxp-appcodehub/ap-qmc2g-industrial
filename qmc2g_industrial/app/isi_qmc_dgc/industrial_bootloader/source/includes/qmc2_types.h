/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef __QMC2_TYPES_h_
#define __QMC2_TYPES_h_

//-----------------------------------------------------------------------
// Linker macros placing include file
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include <qmc2_boot_cfg.h>
#include "fsl_common.h"
#include "fsl_puf.h"
#include "fsl_sss_api.h"
#include <ex_sss_boot.h>

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define SCP03_PLATFORM_KEY_CODE_SIZE PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(PUF_INTRINSIC_SCP03_KEY_SIZE)
#define SE_AES_POLICY_KEY_CODE_SIZE PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(PUF_AES_POLICY_KEY_SIZE)
#define PUF_INTRINSIC_LOG_KEY_CODE_SIZE PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(PUF_INTRINSIC_LOG_KEY_SIZE)
#define PUF_INTRINSIC_LOG_NONCE_CODE_SIZE PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(PUF_INTRINSIC_LOG_NONCE_SIZE)

#define ENSURE_MESSAGE(strCONDITION)   \
        PRINTF("nxEnsure:'" strCONDITION "' failed. At Line:%d Function:%s", __LINE__, __FUNCTION__)

#define ENSURE_OR_EXIT_WITH_LOG(CONDITION, LOG, LOG_REASON) \
    if (!(CONDITION)) { \
        ENSURE_MESSAGE(#CONDITION); \
        LOG = LOG_REASON; \
        goto exit; \
    }

#define ENSURE_OR_EXIT(CONDITION) \
    if (!(CONDITION)) { \
        ENSURE_MESSAGE(#CONDITION); \
        goto exit; \
    }
//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Typedef
//------------------------------------------------------------------------------
/*!
 * @brief Lists possible reasons that can trigger a reset.
 */
typedef enum _qmc_reset_cause_id
{
	kQMC_ResetNone         = 0U,
    kQMC_ResetRequest      = 1U,
    kQMC_ResetSecureWd     = 2U,
    kQMC_ResetFunctionalWd = 3U,
} qmc_reset_cause_id_t;

typedef enum _qmc_status
{
    kStatus_QMC_Ok            = kStatus_Success,                         /*!< Operation was successful; no errors occurred. */
    kStatus_QMC_Err           = kStatus_Fail,                            /*!< General error; not further specified. */
    kStatus_QMC_ErrRange      = kStatus_OutOfRange,                      /*!< Error in case input argument is out of range e.g. array out of bounds, value not defined in enumeration. */
    kStatus_QMC_ErrArgInvalid = kStatus_InvalidArgument,                 /*!< Argument invalid e.g. wrong type, NULL pointer, etc. */
    kStatus_QMC_Timeout       = kStatus_Timeout,                         /*!< Operation was not carried out because a timeout occurred. */

} qmc_status_t;

/*!
 * @brief A timestamp based on the UNIX epoch (01.01.1970 00:00:00 UT) with added sub-second resolution. Timestamps have a resolution of 10 milliseconds.
 */
typedef struct _qmc_timestamp
{
    uint64_t seconds;
    uint16_t milliseconds;
} qmc_timestamp_t;

/*!
 * @brief Firmware version (format: MAJOR . MINOR . BUGFIX)
 */
typedef struct _qmc_fw_version
{
	uint8_t major;
	uint8_t minor;
	uint8_t bugfix;
} qmc_fw_version_t;
/*
 *  The main Firmware header.
 *  Must be paced at the beginning of the image with size 256 Bytes.
 *  */
typedef struct _header
{
	uint32_t magic;					/* Magic number 0x456D1BE8U */
	uint32_t version;				/* FW version */
	uint32_t fwDataAddr;			/* FW start address - from FW header */
	uint32_t fwDataLength;			/* FW data length - up to signature*/
	uint32_t signDataAddr;			/* FW signature start */
	uint32_t cm4FwDataAddr;			/* CM4 image start address */
	uint32_t cm4FwDataLength; 		/* CM4 image length */
	uint32_t cm4BootAddr;			/* CM4 boot address */
	uint32_t cm7VectorTableAddr;	/* CM7 Vector Table address */
	uint32_t cfgDataAddr;			/* Configuration Data start address in Octal Flash */
	uint32_t cfgDataLength;			/* Configuration Data length */
} header_t;

/* Tha main Firmware header. */
typedef struct _fwu_header
{
	header_t hdr;
	bool backupImgActive;
} fw_header_t;

/* The new Firmware manifest.
 * Must be paced at the begginig of the FWU image with size 256 Bytes.
 * */
typedef struct _manifest
{
	uint32_t vendorID;			/* Vendir ID - 0xB5A5B5A5U*/
	uint32_t version;			/* FWU version */
	uint32_t fwDataAddr;		/* FW Data address */
	uint32_t fwDataLength;		/* FW data length: Header+ CM7 + CM4 iamges + signature */
	uint32_t fwuDataAddr;		/* FWU data address in FWU storage */
	uint32_t fwuDataLength;		/* FWU Data length */
	uint32_t signDataAddr;		/* FWU signature address */
} manifest_t;


/*************************************
 *  FWU Manifest Data
 *************************************/
/* The new Firmware manifest. */
typedef struct _sbl_fwu_manifest
{
	manifest_t man;
	bool isFwuImgOnSdCard;
} fwu_manifest_t;

typedef enum _log_entry
{
    kLOG_Scp03ConnFailed 			= 0x31U, /* SCP03 connection Failed. */
	kLOG_Scp03KeyReconFailed		= 0x32U, /* Key reconstruction Failed. */
	kLOG_NewFWReverted				= 0x33U, /* Program Backup image.*/
	kLOG_NewFWRevertFailed			= 0x34U, /* Program Backup image Failed.*/
	kLOG_NewFWCommited 				= 0x35U, /* Commit new version of FW into SE and create new Backup Image. */
	kLOG_NewFWCommitFailed			= 0x36U, /* Commit new version of FW into SE and create new Backup Image Failed. */
	kLOG_AwdtExpired				= 0x37U, /* Authenticated WDOG expired, reboot keeping this state in SNVS_LP_GPR to go into maitanance mode. */
	kLOG_CfgDataBackuped			= 0x38U, /* Back up configuration data. */
	kLOG_CfgDataBackupFailed		= 0x39U, /* Back up configuration data failed. */
	kLOG_MainFwAuthFailed			= 0x3AU, /* Main FW authentication failed. */
	kLOG_FwuAuthFailed				= 0x3BU, /* FWU authentication failed. */
	kLOG_StackError					= 0x3CU, /* Stack Error/Corruption detected. */
	kLOG_KeyRevocation				= 0x3DU, /* Key Revocation request. */
	kLOG_InvalidFwuVersion			= 0x3EU, /* Invalid FWU version */
	kLOG_ExtMemOprFailed			= 0x3FU, /* Invalid FWU version */
	kLOG_BackUpImgAuthFailed		= 0x41U, /* Invalid FWU version */
	kLOG_SdCardFailed				= 0x42U, /* Invalid FWU version */
	kLOG_HwInitDeinitFailed			= 0x43U, /* HW Init Failed */
	kLOG_SvnsLpGprOpFailed			= 0x45U, /* SNVS LP GPR read or write failed.*/
	kLOG_Scp03KeyRotationFailed		= 0x46U, /* Key rotation Failed. */
	kLOG_DecomissioningFailed		= 0x47U, /* Decommisioning failed. */
	kLOG_VerReadFromSeFailed		= 0x48U, /* Reading version from SE failed. */
	kLOG_FwExecutionFailed			= 0x49U, /* Booting of the main FW failed. */
	kLOG_FwuCommitFailed			= 0x50U, /* Booting of the main FW failed. */
	kLOG_DeviceDecommisioned		= 0x61U, /* Booting of the main FW failed. */
	kLOG_RpcInitFailed				= 0x73U, /* Booting of the main FW failed. */
	kLOG_NoLogEntry					= 0x00U, /* There is not log entry. */
} log_entry_t;

/* SBL interna state */
typedef enum _fwu_int_state
{
	kFWU_SblFirstBoot				= 0x53,
	kFWU_EraseFwUpdtStorage			= 0x54,
	kFWU_EraseMainFw				= 0x35,
	kFWU_EraseBackupImg				= 0x78,
	kFWU_ProgramNewFw				= 0x51,
	kFWU_PgmFwSdCardToStorage 		= 0x58,
	kFWU_ProgramBackupImage			= 0x14,
	kFWU_UpdateNewVersion 			= 0x64,
	kFWU_UpdateCompleted 			= 0x87,
	kFWU_NoIntState 				= 0x00,
} fwu_int_state_t;

#if 0
/* Main FW state */
typedef enum _fw_state
{
	kFWU_Revert			= 0xAA, /* Program Backup image.*/
	kFWU_Commit 		= 0xBA, /* Commit new version of FW into SE and create new Backup Image. */
	kFWU_AwdtExpired	= 0xCB, /* Authenticated WDOG expired, reboot keeping this state in SNVS_LP_GPR to go into maitanance mode. */
	kFWU_BackUpCfgData	= 0xDE, /* Back up configuration data. */
	kFWU_VerifyFw		= 0xEF, /* After FW update is programmed and authenticated. SBL indicates to FW that selfcheck must be performed to provide kFWU_Commit for status to finalize FW update. */
	kFWU_StackError		= 0xAF, /* Stack Error/Corruption detected. */
	kFWU_KeyRevocation  = 0xBB, /* Key Revocation Request */
	kFWU_NoState	    = 0x00, /* Notify SBL */
} fw_state_t;
#endif
/*!
 * @brief Represents firmware update activities that the main application firmware can send to the SBL for execution upon the next boot.
 */
typedef enum _fw_state
{
    kFWU_Revert         = 0x01U, /*!< Program the recovery image */
    kFWU_Commit         = 0x02U, /*!< Commit new version of FW into SE and create new recovery image */
    kFWU_BackupCfgData  = 0x04U, /*!< Back up configuration data */
    kFWU_AwdtExpired    = 0x08U, /*!< Authenticated watchdog expired; reboot keeping this state in SNVS_LP_GPR (main FW must go into maintenance mode) */
    kFWU_VerifyFw       = 0x10U, /*!< After FW update is programmed, SBL indicates to the new FW that a self-check must be performed to provide kFWU_Commit or kFWU_Revert status finalizing the FW update process. */
	kFWU_TimestampIssue = 0x20U, /*!< Firmware timestamp could not be checked, e.g. timestamp is in the future / RTC not set correctly. */
	kFWU_NoState  		= 0x00U,
} fw_state_t;

/* Wdog reset reason */
typedef enum _wdog_reset_reason
{
	kWDOGRR_AWDT		= 0x84, /* Program Backup image.*/
	kWDOGRR_WDOG3 		= 0x85, /* Commit new version of FW into SE and create new Backup Image. */
} wdog_reset_reason_t;

/* SNVS LP GPRD data structure for SBL. Only 4-bytes dedicated to SBL! */
typedef struct _svns_lpgpr
{
    uint16_t wdTimerBackup;   /*!< Backup of the ticks which the AWDT has left till expiry. */
    uint8_t wdStatus;         /*!< Status whether AWDT was running before reset. */
    fw_state_t fwState;
} svns_lpgpr_t;

typedef struct _scp03_keys
{
	uint8_t enc_key[PUF_INTRINSIC_SCP03_KEY_SIZE];
	uint8_t mac_key[PUF_INTRINSIC_SCP03_KEY_SIZE];
	uint8_t dek_key[PUF_INTRINSIC_SCP03_KEY_SIZE];
	uint8_t aes_policy_key[PUF_AES_POLICY_KEY_SIZE];
} scp03_keys_t;

typedef struct _log_keys
{
	uint8_t aesKey1[PUF_INTRINSIC_LOG_KEY_SIZE];
	uint8_t nonce1[PUF_INTRINSIC_LOG_NONCE_SIZE];
	uint8_t aesKey2[PUF_INTRINSIC_LOG_KEY_SIZE];
	uint8_t nonce2[PUF_INTRINSIC_LOG_NONCE_SIZE];
}  log_keys_t;

typedef struct _scp03_puf_key_codes
{
	uint8_t enc_key_code[SCP03_PLATFORM_KEY_CODE_SIZE];
	uint8_t mac_key_code[SCP03_PLATFORM_KEY_CODE_SIZE];
	uint8_t dek_key_code[SCP03_PLATFORM_KEY_CODE_SIZE];
	uint8_t aes_policy_key_code[SE_AES_POLICY_KEY_CODE_SIZE];
} scp03_puf_key_codes_t;

typedef struct _log_puf_key_codes
{
	uint8_t aesKeyCode1[PUF_INTRINSIC_LOG_KEY_CODE_SIZE];
	uint8_t nonceKeyCode1[PUF_INTRINSIC_LOG_NONCE_CODE_SIZE];
	uint8_t aesKeyCode2[PUF_INTRINSIC_LOG_KEY_CODE_SIZE];
	uint8_t nonceKeyCode2[PUF_INTRINSIC_LOG_NONCE_CODE_SIZE];
} log_puf_key_codes_t;

typedef struct _puf_key_store
{
	scp03_puf_key_codes_t 		scp03;
	log_puf_key_codes_t			log;
} puf_key_store_t;

typedef struct _boot_keys
{
	scp03_keys_t 				scp03;
	log_keys_t					log;
} boot_keys_t;

typedef struct _sdcard
{
	bool isInserted;
} sdcard_t;


/* Struct for storing variables for/from SE */
typedef struct _se_data
{
	uint32_t fwVersion;
	uint32_t manVersion;
} se_data_t;

/* SNVS LP GPRD data structure for SBL. Only 4-bytes dedicated to SBL! */
typedef struct _sign_header
{
    union
    {
        struct
        {
        	uint8_t sequenceTag;
        	uint8_t numOfLengthBytes;
        	uint8_t Length1;
        	uint8_t Length2;
        } B;
        uint32_t U;
    } data;
} sign_header_t;

/* Structure for signature verification*/
typedef struct _se_verify_sign
{
	uint32_t fwDataAddr;
	uint32_t fwDataLength;
	uint32_t signDataAddr;
	uint32_t signDataLength;
    uint32_t keyId;
    sss_key_part_t keyPart;
    sss_cipher_type_t cipherType;
    sss_algorithm_t algorithm;
    sss_mode_t mode;
} se_verify_sign_t;


//TODO remove
/* Generate keys */
typedef struct _se_gen_keys
{
    uint32_t keyId;
    size_t keyBitLen;
    sss_key_part_t keyPart;
    sss_cipher_type_t cipherType;
} se_gen_keys_t;
//

/* The new Firmware manifest. */
typedef struct _se_state
{
	SE05x_LockState_t lockState;
	SE05x_RestrictMode_t restrictMode;
	SE05x_PlatformSCPRequest_t platformSCPRequest;
} se_state_t;


/* The main SBL structure */
typedef struct _boot_data
{
	fw_header_t fwHeader;
	fwu_manifest_t fwuManifest;
	svns_lpgpr_t svnsLpGpr;
	boot_keys_t bootKeys;
	se_data_t seData;
	sdcard_t sdCard;
} boot_data_t;

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __qmc2_types_h_ */

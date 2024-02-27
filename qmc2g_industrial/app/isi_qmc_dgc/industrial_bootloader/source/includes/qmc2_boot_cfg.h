/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _QMC2_BOOT_CFG_H
#define _QMC2_BOOT_CFG_H

/*******************************************************************************
 * Include files dependency check
 *******************************************************************************/
#include "fsl_common.h"
#include "fsl_debug_console.h"
/*******************************************************************************
 * Doxygen
 *******************************************************************************/
/**
 * \defgroup bootloader_cfg
 * Main L2 Bootloader configuration.
 *
 * This module describes configuration options for L2 Bootloader example 
 *
 * Correct configuration is necessary to make L2 Boot working properly. You can setup various keys e.g. for BEE HW 
 * emulation and ECB encryption of FW update binary file. FAC regions must corelate with linker file configurations.
 * This file also contains various debug options.
 * \{
 */
/* End of Doxygen */

/*******************************************************************************
 * Macros & Constants definitions
 *******************************************************************************/
#define SD_CARD_FW_UPDATE_FILE				"/qmc2/fw_update.bin"
#define SD_CARD_DECOMMISSIONING_FILE		"/qmc2/decommission.bin"

#define SEC_SIGNATURE_BLOCK_SIZE			256
#define FW_HEADER_BLOCK_SIZE				256
#define RAM_BUFFER_SIZE						0x2000U

#define RPC_SECWD_MAX_RNG_SEED_SIZE  		(48U) /*!< see secure watchdog implementation for details */
#define RPC_SECWD_MAX_PK_SIZE        		(158U) /*!< see secure watchdog implementation for details */
#define RPC_SECWD_MAX_MSG_SIZE       		(150U)
#define RPC_SECWD_INIT_DATA_ADDRESS 		(0x2023FF00U)//CM7 alias for (0x2001ff00) /*!< address to which the secure watchdog init data should be copied */
#define RPC_SHM_INIT_DATA_ADDRESS			(0x20340000U) // Initialization data for RPC
#define SECURE_WDOG_READY		       		(0x01U)

#ifdef NON_SECURE_SBL

#define AES_LOG_NONCE                                                                                       \
    {                                                                                                               \
        0x30, 0x81, 0x9b, 0x30, 0x14, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x09, \
    }

#define AES_LOG_KEY                                                                                        \
    {                                                                                                               \
        0xdf, 0xf3, 0x89, 0x35, 0x07, 0xe3, 0xb8, 0x43, 0x33, 0xaf, 0xad, 0x6a, 0xfe, 0xfd, 0x0b, 0xce,   \
        0xe5, 0xc9, 0x3b, 0x4f, 0x2d, 0xfd, 0x3e, 0x9f, 0x3e, 0xbc, 0xa7, 0x76, 0xb5, 0x6a, 0x26, 0x9e   \
    }

#define QMC_CM4_TEST_AWDG_PK                                                                                        \
    {                                                                                                               \
        0x30, 0x81, 0x9b, 0x30, 0x14, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x09, 0x2b, 0x24, \
            0x03, 0x03, 0x02, 0x08, 0x01, 0x01, 0x0d, 0x03, 0x81, 0x82, 0x00, 0x04, 0x5b, 0x7c, 0x55, 0xb3, 0x7c,   \
            0xd8, 0xa1, 0xfe, 0xf0, 0x1a, 0x8a, 0xf6, 0xa4, 0x84, 0x58, 0x02, 0xc5, 0x90, 0x65, 0xc7, 0x0c, 0x3d,   \
            0xdf, 0xf3, 0x89, 0x35, 0x07, 0xe3, 0xb8, 0x43, 0x33, 0xaf, 0xad, 0x6a, 0xfe, 0xfd, 0x0b, 0xce, 0x5a,   \
            0xe5, 0xc9, 0x3b, 0x4f, 0x2d, 0xfd, 0x3e, 0x9f, 0x3e, 0xbc, 0xa7, 0x76, 0xb5, 0x6a, 0x26, 0x9e, 0xf9,   \
            0x15, 0xf4, 0xa3, 0xe4, 0xbd, 0x0c, 0x06, 0xae, 0x25, 0x92, 0x1f, 0x5d, 0xa5, 0xbc, 0x1f, 0x9f, 0xe8,   \
            0x48, 0xa3, 0x75, 0x15, 0xd3, 0xe8, 0xd9, 0xf0, 0x9d, 0x1c, 0x30, 0xf4, 0x21, 0xa5, 0x35, 0x19, 0x39,   \
            0xf5, 0x8b, 0x7c, 0xbf, 0x26, 0x89, 0x4b, 0x16, 0x2b, 0x12, 0x39, 0xf7, 0x44, 0x31, 0xcf, 0xc3, 0xd9,   \
            0x1d, 0x9d, 0x04, 0x9e, 0xa3, 0x45, 0xc6, 0x5c, 0xfc, 0x57, 0xca, 0x2f, 0x30, 0x52, 0xeb, 0x96, 0x11,   \
            0xec, 0x89, 0xc1, 0x46                                                                                  \
    }


#define QMC_CM4_TEST_AWDG_RNG_SEED                                                                                   \
    {                                                                                                               \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00                            \
    }                                         /*!< AWDG RNG seed for non-SBL testing */

#endif

#define RDC_SHA512_SBL                                                                                        		\
	{                                                                                                               \
		0x5E, 0x33, 0x08, 0x49, 0xDB, 0xD6, 0xD5, 0x1C, 0xAC, 0x31, 0x91, 0x24, 0xBF, 0xBF, 0x21, 0xA8, 0x90, 0xA2, 0x96, 0x7F, 0x46, 0x86, 0x96,\
		0x18, 0x46, 0xE8, 0xDC, 0x35, 0x6E, 0x5F, 0x82, 0x87, 0xC5, 0x12, 0xAA, 0xFE, 0x9C, 0xE5, 0xD3, 0xD6, 0xDD, 0xDB, 0xE3, 0x86, 0x72, 0xE3,\
		0xEE, 0x35, 0xA1, 0xF6, 0xB0, 0xE3, 0x6A, 0x4B, 0xCC, 0xE0, 0x1E, 0x55, 0x7A, 0x14, 0xD3, 0xAF, 0x12, 0x7F,\
	}

#define RDC_SHA512_APP                                                                                        		\
	{                                                                                                               \
		0x7F, 0x96, 0x43, 0xD8, 0x24, 0x33, 0xB5, 0x2E, 0xC7, 0x6D, 0x93, 0xF7, 0x96, 0x39, 0x96, 0x38, 0x01, 0xE6, 0x4A, 0x67, 0xA3, 0x8E, 0xA1,\
		0xBE, 0x27, 0x43, 0x4E, 0xCA, 0x10, 0x1D, 0x02, 0x24, 0xF9, 0x14, 0x7D, 0xFC, 0x4C, 0x6A, 0x08, 0x01, 0xB8, 0x1D, 0xE8, 0xE0, 0x9F, 0x8F,\
		0xA1, 0xF0, 0x18, 0xD2, 0x31, 0x0C, 0xFD, 0x9E, 0x19, 0x50, 0x09, 0x85, 0x05, 0x38, 0xC9, 0xFD, 0xDA, 0x25\
	}

#define SHA512_SIZE_IN_BYTES				64

#define LOG_AES_KEY_NONCE_ADDRESS	 		0x2034FC00U
#define MAIN_FW_SCP03_KEYS_ADDRESS			0x2034FC80U

#define SE_FWU_PUB_KEY_ID 					0x0000000AU
#define SE_FW_PUB_KEY_ID 					0x0000000EU

#define SE_RPC_KEY_ID	 					0x0000000CU
#define SE_FW_VERSION_ID 					0x00000001U
#define SE_MAN_VERSION_ID					0x00000002U
#define SE_OEM_CA_CERT						0x00000003U

#define SE_AES_AUTH_OBJ_SBL_ID 				0x0000001CU
#define SE_AES_POLICY_KEY_SBL_ID			0x0000001DU

#define SE_MANDATE_SCP_ID					0xEF00A5E5U

#define SE_MAX_BYTE_WRITE_LENGTH			RPC_SECWD_MAX_PK_SIZE

#define SNVS_LPGPR_REG_IDX					(0U) /*!< which SVNS GPR index is used for the FwuStatus */
#define SNVS_LPGPR_FWUSTATUS_SNVS_MASK  	(0xFF000000U) /*!< used bits mask for FwuStatus in SNVS GPR */
#define SNVS_LPGPR_FWUSTATUS_SNVS_POS   	(24U)         /*!< shift position for FwuStatus in SNVS GPR */

#define FLEXSPI1_FCB_ADDRESS 	            0x60000800U 	/* Default FCB address for Octal Flash memory */
#define FLEXSPI2_FCB_ADDRESS 	            0x60000400U		/* Default FCB address for QSPI memory */

#define FLEXSPI1_INSTANCE  		            1				/* FlexSPI 1 instance */
#define FLEXSPI2_INSTANCE  		            2				/* FlexSPI 2 instance */
#define QSPI_FLASH_ERASE_SECTOR_SIZE		0x1000U
#define PGM_PAGE_SIZE						0x100U
#define EXAMPLE_FLEXSPI1_AMBA_BASE 			FlexSPI1_AMBA_BASE
#define EXAMPLE_FLEXSPI2_AMBA_BASE 			FlexSPI2_AMBA_BASE
#define ENCRYPTED_XIP_FLEXSPI_BASE			EXAMPLE_FLEXSPI2_AMBA_BASE

#define SBL_MAIN_FW_ADDRESS					0x60080000U
#define SBL_MAIN_FW_SIZE					0xFBF000U

#define SBL_BACKUP_IMAGE_ADDRESS			0x6103F000U
#define SBL_BACKUP_IMAGE_SIZE				0xFBF000U

#define SBL_CFGDATA_BACKUP_ADDRESS			0x61FFE000U
#define SBL_CFGDATA_BACKUP_SIZE				2*QSPI_FLASH_ERASE_SECTOR_SIZE

#define SBL_CFGDATA_ADDRESS					0x30000000U
#define SBL_CFGDATA_SIZE					2*QSPI_FLASH_ERASE_SECTOR_SIZE

#define SBL_LOG_STORAGE_ADDRESS				0x30002000U
#define SBL_LOG_STORAGE_SIZE				0x303E000U

#define SBL_FWU_STORAGE_ADDRESS				0x33040000U
#define SBL_FWU_STORAGE_SIZE				0xFC0000U

#define OCTAL_FLASH_FLEXSPI FLEXSPI1
#define OCTAL_RAM_FLEXSPI FLEXSPI1
#define OCTAL_FLASH_SIZE_A1 0x10000
#define OCTAL_RAM_SIZE_A2 0x8000
#define OCTAL_FLASH_SIZE (OCTAL_FLASH_SIZE_A1*1024U)
#define OCTAL_FLASH_PAGE_SIZE 256U
#define	OCTAL_FLASH_SECTOR_SIZE 4096U

//LUT sequences
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA 		0
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READID 			1
#define OCTALRAM_CMD_LUT_SEQ_IDX_READDATA 			2
#define OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA 			3
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS		4
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2			6
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2		8
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2_SPI	13
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE		7
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE_SPI	10
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA 		11
#define OCTALFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR 		12
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG			9

#define CUSTOM_LUT_LENGTH 64

/* Address of memory, from which the secondary core will boot */
#define CORE1_BOOT_ADDRESS 					0x20200000U
/*The size of SVNS_LPGPR register is 16 bytes */
#define SVNS_LPGPR_SIZE_IN_BYTES			16
/* Magic word for a record validation */
#define SVNS_LP_GPR_MAGIC					0xA5B5A5B5U
/* Main FW header MAGIC */
#define MAIN_FW_HEADER_MAGIC				0x456D1BE8U
/* Main FW update Vendor ID */
#define MAIN_FW_UPDATE_VENDOR_ID			0xB5A5B5A5U

#define PUF_KEY_INDEX_ENC 					kPUF_KeyIndex_02
#define PUF_KEY_INDEX_MAC 					kPUF_KeyIndex_03
#define PUF_KEY_INDEX_DEK 					kPUF_KeyIndex_04
#define PUF_KEY_INDEX_POLICY				kPUF_KeyIndex_05

#define PUF_KEY_INDEX_LOG_1 				kPUF_KeyIndex_06
#define PUF_KEY_INDEX_LOG_NONCE_1 			kPUF_KeyIndex_07
#define PUF_KEY_INDEX_LOG_2 				kPUF_KeyIndex_08
#define PUF_KEY_INDEX_LOG_NONCE_2 			kPUF_KeyIndex_09

#define PUF_INTRINSIC_SCP03_KEY_SIZE 		16
#define PUF_AES_POLICY_KEY_SIZE				16
#define PUF_INTRINSIC_LOG_KEY_SIZE 	 		32
#define PUF_INTRINSIC_LOG_NONCE_SIZE		16

/* Worst-case time in ms to fully discharge PUF SRAM */
#define PUF_DISCHARGE_TIME 					400
#define PUF                					KEY_MANAGER__PUF
#define PUF_INTRINSIC_KEY_SIZE 				16

#if defined(ROM_ENCRYPTED_XIP_ENABLED) && (ROM_ENCRYPTED_XIP_ENABLED == 1)
#define PUF_AC_CODE_ADDRESS 0x60000810
#else
#define PUF_AC_CODE_ADDRESS 0x60078010
#endif

#define PUF_SBL_KEY_STORE_ADDRESS 0x60079000

/*******************************************************************************
 * Datalogger
 ******************************************************************************/

//Time to wait for grant access by mutex
#define DATALOGGER_MUTEX_XDELAYS_MS (800)

//Depth of receiving datalogger task queue
#define DATALOGGER_RCV_QUEUE_DEPTH 3U

//dynamicly allocated queue for 3th party services
#define  FEATURE_DATALOGGER_DQUEUE
#define  FEATURE_DATALOGGER_DQUEUE_EVENT_BITS

//Depth of dynamicly allocated queue
#define DATALOGGER_DYNAMIC_RCV_QUEUE_DEPTH 3U

//Max number of dynamicy allocated dynamic queues
#define DATALOGGER_RCV_QUEUE_CN 2U

//Report LOW_MEMORY if LOW_MEMORY_TRESHOLD is triggered
#define DATALOGGER_REPORT_LOW_MEMORY
//LOW_MEMORY_TRESHOLD in percentage
#define DATALOGGER_LOW_MEMORY_TRESHOLD (80U)
//Octal flash start address of recorder
#define FLASH_RECORDER_ORIGIN   SBL_LOG_STORAGE_ADDRESS

//Size of LogRecorder in sectors
#define FLASH_RECORDER_SECTORS 32U

#define portMAX_DELAY 100
#define RTCLOW_SNVS_INDEX (1U)          /*!< which SNVS GPR index is used for the lowest bits RTC offset */
#define RTCHIGH_SNVS_INDEX (2U)          /*!< which SNVS GPR index is used for the highest bits RTC offset */
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
    idSblAuthObject           = 0x0000001C, /*!< AES256 key */
    idSblAuthObjectFirstRun   = 0x0000001D, /*!< file, 32 bytes */
    idAppAuthObject           = 0x0000001E, /*!< AES256 key */
    idAppAuthObjectFirstRun   = 0x0000001F, /*!< file, 32 bytes */
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

/*******************************************************************************
 * Variables externs definitions
 *******************************************************************************/
/* End of Variables */

#ifdef __cplusplus
}
#endif

/** \} */ // end of bootloader_source group
#endif /* _QMC2_BOOT_CFG_H */

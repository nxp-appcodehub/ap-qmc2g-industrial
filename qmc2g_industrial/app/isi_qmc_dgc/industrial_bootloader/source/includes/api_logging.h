/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_LOGGING_H_
#define _API_LOGGING_H_

#include "qmc2_types.h"
#include "stdbool.h"

#include "app.h"
#include "lcrypto.h"
#include "flash_recorder.h"

#define LOG_ENCRYPTED_RECORD_MAX_SIZE (64U)

#define DATALOGGER_EVENTBIT_DQUEUE_QUEUE      (1 << 0)
#define DATALOGGER_EVENTBIT_FIRST_STATUS_QUEUE (1 << 2)

/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/

/*!
 * @brief Lists all available log data formats for the data : log_recorddata_t field of log_record_t.
 *
 * Implementation hint: When defining new formats, extend log_recorddata_t as well.
 */
typedef enum _log_record_type_id
{
    kLOG_DefaultData        = 0x01U, /*!< Identifier for log_recorddata_default_t. */
    kLOG_FaultDataWithID	= 0x02U, /*!< Identifier for log_recorddata_fault_with_id_t. */
    kLOG_FaultDataWithoutID = 0x03U, /*!< Identifier for log_recorddata_fault_without_id_t. */
    kLOG_SystemData         = 0x04U, /*!< Identifier for log_recorddata_system_t. */
    kLOG_ErrorCount         = 0x05U  /*!< Identifier for log_recorddata_error_count_t. */
} log_record_type_id_t;

/*!
 * @brief List of log record sources
 */
typedef enum _log_source_id
{
    LOG_SRC_Unspecified                        = 0x00U, /*!< Unspecified source */
    LOG_SRC_Webservice                         = 0x01U, /*!< Log written by the Web Service */
    LOG_SRC_FaultHandling                      = 0x02U, /*!< Log written by the Fault Handling */
    LOG_SRC_CloudService                       = 0x03U, /*!< Log written by the Cloud Service */
    LOG_SRC_LocalService                       = 0x04U, /*!< Log written by the Local Service */
    LOG_SRC_BoardService                       = 0x05U, /*!< Log written by the Board Service */
    LOG_SRC_AnomalyDetection                   = 0x06U, /*!< Log written by the Anomaly Detection */
    LOG_SRC_MotorControl                       = 0x07U, /*!< Log written by the Motor Control e.g. via DataHub task */
    LOG_SRC_SecureWatchdog                     = 0x08U, /*!< Log written based on a secure watchdog reset detection. */
    LOG_SRC_TaskStartup                        = 0x09U, /*!< Log written by the Startup Task */
    LOG_SRC_RpcModule                          = 0x0AU, /*!< Log written by the RPC module */
    LOG_SRC_SecureWatchdogServiceRequestNonce  = 0x0BU, /*!< Log written by the Secure Watchdog Service (getting nonce) */
    LOG_SRC_SecureWatchdogServiceRequestTicket = 0x0CU, /*!< Log written by the Secure Watchdog Service (requesting ticket) */
    LOG_SRC_SecureWatchdogServiceKick          = 0x0DU, /*!< Log written by the Secure Watchdog Service (kick) */
    LOG_SRC_FunctionalWatchdog                 = 0x0EU, /*!< Log written based on a functional watchdog reset detection. */
    LOG_SRC_PowerLossInterrupt                 = 0x0FU, /*!< Log written based on a power loss event detection. */
    LOG_SRC_LoggingService			           = 0x10U, /*!< Log written by the Logging Service */
	LOG_SRC_TSN			         			   = 0x11U, /*!< Log written by the TSN */
	LOG_SRC_DataHub	         				   = 0x12U, /*!< Log written by the DataHub */
	LOG_SRC_SecureBootloader 				   = 0x13U, /*!< Log written by the Secure Bootloader */
} log_source_id_t;

/*!
 * @brief List of log record categories
 */
typedef enum _log_category_id
{
    LOG_CAT_General        = 0x00U, /*!< General category; covers log entries that don't fit any of the other categories */
    LOG_CAT_Fault          = 0x01U, /*!< Motor and system fault events */
    LOG_CAT_Authentication = 0x02U, /*!< Authentication events, e.g. login attempts */
    LOG_CAT_Connectivity   = 0x03U, /*!< Connectivity events e.g. connection established/lost, synchronization state change, etc. */
} log_category_id_t;

/*!
 * @brief List of log event codes
 */
typedef enum _log_event_code
{
    /* Fault Handling*/
    LOG_EVENT_AfeDbCommunicationError      = 0x00U,
    LOG_EVENT_AfePsbCommunicationError     = 0x01U,
    LOG_EVENT_DBTempSensCommunicationError = 0x02U,
    LOG_EVENT_DbOverTemperature            = 0x03U,
    LOG_EVENT_EmergencyStop                = 0x04U,
    LOG_EVENT_FaultBufferOverflow          = 0x05U,
    LOG_EVENT_FaultQueueOverflow           = 0x06U,
    LOG_EVENT_GD3000_Desaturation          = 0x07U,
    LOG_EVENT_GD3000_LowVLS                = 0x08U,
    LOG_EVENT_GD3000_OverCurrent           = 0x09U,
    LOG_EVENT_GD3000_OverTemperature       = 0x0AU,
    LOG_EVENT_GD3000_PhaseError            = 0x0BU,
    LOG_EVENT_GD3000_Reset                 = 0x0CU,
    LOG_EVENT_InvalidFaultSource           = 0x0DU,
    LOG_EVENT_McuOverTemperature           = 0x0EU,
    LOG_EVENT_NoFault                      = 0x0FU,
    LOG_EVENT_NoFaultBS                    = 0x10U,
    LOG_EVENT_NoFaultMC                    = 0x11U,
    LOG_EVENT_OverCurrent                  = 0x12U,
    LOG_EVENT_OverDcBusVoltage             = 0x13U,
    LOG_EVENT_OverLoad                     = 0x14U,
    LOG_EVENT_OverSpeed                    = 0x15U,
    LOG_EVENT_PmicOverTemperature          = 0x16U,
    LOG_EVENT_PmicUnderVoltage             = 0x17U,
    LOG_EVENT_SPISwitchFailed              = 0x18U,
    LOG_EVENT_PsbOverTemperature1          = 0x19U,
    LOG_EVENT_PsbOverTemperature2          = 0x1AU,
    LOG_EVENT_RotorBlocked                 = 0x1BU,
    LOG_EVENT_UnderDcBusVoltage            = 0x1CU,

    /* Local Service */
    LOG_EVENT_Button1Pressed         = 0x1DU,
    LOG_EVENT_Button2Pressed         = 0x1EU,
    LOG_EVENT_Button3Pressed         = 0x1FU,
    LOG_EVENT_Button4Pressed         = 0x20U,
    LOG_EVENT_EmergencyButtonPressed = 0x21U,
    LOG_EVENT_LidOpenButton          = 0x22U,
    LOG_EVENT_LidOpenSd              = 0x23U,
    LOG_EVENT_TamperingButton        = 0x24U,
    LOG_EVENT_TamperingSd            = 0x25U,

    /* Secure Watchdog */
    LOG_EVENT_ResetSecureWatchdog = 0x26U,

    /* Webservice / Authentication */
    LOG_EVENT_AccountResumed   = 0x27U,
    LOG_EVENT_AccountSuspended = 0x28U,
    LOG_EVENT_LoginFailure     = 0x29U,
    LOG_EVENT_SessionTimeout   = 0x2AU,
    LOG_EVENT_TerminateSession = 0x2BU,
    LOG_EVENT_UserLogin        = 0x2CU,
    LOG_EVENT_UserLogout       = 0x2DU,

    /* Motor control / DataHub */
    LOG_EVENT_QueueingCommandFailedInternal = 0x2EU,
    LOG_EVENT_QueueingCommandFailedTSN      = 0x2FU,
    LOG_EVENT_QueueingCommandFailedQueue    = 0x30U,

    /* RPC module */
    LOG_EVENT_ResetRequest      = 0x31U,
    LOG_EVENT_InvalidResetCause = 0x32U,

    /* General */
    LOG_EVENT_InvalidArgument  = 0x33U,
    LOG_EVENT_RPCCallFailed    = 0x34U,
    LOG_EVENT_AWDTExpired      = 0x35U,
    LOG_EVENT_SignatureInvalid = 0x36U,
    LOG_EVENT_Timeout          = 0x37U,
    LOG_EVENT_SyncError        = 0x38U,
    LOG_EVENT_InternalError    = 0x39U,
    LOG_EVENT_NoBufsError      = 0x3AU,
    LOG_EVENT_ConnectionError  = 0x3BU,
    LOG_EVENT_RequestError     = 0x3CU,
    LOG_EVENT_JsonParsingError = 0x3DU,
    LOG_EVENT_RangeError       = 0x3EU,
	LOG_EVENT_PowerLoss        = 0x3FU,
	
	/* Functional Watchdog */
    LOG_EVENT_ResetFunctionalWatchdog = 0x40U,
	LOG_EVENT_FunctionalWatchdogKickFailed = 0x41U,
	LOG_EVENT_FunctionalWatchdogInitFailed = 0x42U,

	/* Secure Bootloader */
	LOG_EVENT_Scp03ConnFailed 			= 0x43U, /* SCP03 connection Failed. */
	LOG_EVENT_Scp03KeyReconFailed		= 0x44U, /* Key reconstruction Failed. */
	LOG_EVENT_NewFWReverted				= 0x45U, /* Program Backup image.*/
	LOG_EVENT_NewFWRevertFailed			= 0x46U, /* Program Backup image Failed.*/
	LOG_EVENT_NewFWCommitted 			= 0x47U, /* Commit new version of FW into SE and create new Backup Image. */
	LOG_EVENT_NewFWCommitFailed			= 0x48U, /* Commit new version of FW into SE and create new Backup Image Failed. */
	LOG_EVENT_AwdtExpired				= 0x49U, /* Authenticated WDOG expired, reboot keeping this state in SNVS_LP_GPR to go into maintenance mode. */
	LOG_EVENT_CfgDataBackedUp			= 0x4AU, /* Backed up configuration data. */
	LOG_EVENT_CfgDataBackUpFailed		= 0x4BU, /* Backing up configuration data failed. */
	LOG_EVENT_MainFwAuthFailed			= 0x4CU, /* Main FW authentication failed. */
	LOG_EVENT_FwuAuthFailed				= 0x4DU, /* FWU authentication failed. */
	LOG_EVENT_StackError				= 0x4EU, /* Stack Error/Corruption detected. */
	LOG_EVENT_KeyRevocation				= 0x4FU, /* Key Revocation request. */
	LOG_EVENT_InvalidFwuVersion			= 0x50U, /* Invalid FWU version */
	LOG_EVENT_ExtMemOprFailed			= 0x51U, /* External Memory Operation Failed */
	LOG_EVENT_BackUpImgAuthFailed		= 0x52U, /* Back Up Image Authentication Failed */
	LOG_EVENT_SdCardFailed				= 0x53U, /* SD Card Failed */
	LOG_EVENT_HwInitDeinitFailed		= 0x54U, /* HW Init Failed */
	LOG_EVENT_SvnsLpGprOpFailed			= 0x55U, /* SNVS LP GPR read or write failed.*/
	LOG_EVENT_Scp03KeyRotationFailed	= 0x56U, /* Key rotation Failed. */
	LOG_EVENT_DecommissioningFailed		= 0x57U, /* Decommissioning failed. */
	LOG_EVENT_VerReadFromSeFailed		= 0x58U, /* Reading version from SE failed. */
	LOG_EVENT_FwExecutionFailed			= 0x59U, /* Booting of the main FW failed. */
	LOG_EVENT_FwuCommitFailed			= 0x5AU, /* Commit of the FWU failed. */
	LOG_EVENT_DeviceDecommissioned		= 0x5BU, /* Device decommissioned. */
	LOG_EVENT_RpcInitFailed				= 0x5CU, /* RPC initialization failed. */
	LOG_EVENT_UnknownFWReturnStatus		= 0x5DU, /* Unknown Firmware return status. */
	LOG_EVENT_NoLogEntry				= 0x5EU, /* There is not log entry. */

} log_event_code_t;




/*******************************************************************************
 * Definitions => Structures
 ******************************************************************************/

extern recorder_t g_LogRecorder;

/*!
 * @brief Structure that defines a general (default) log entry.
 */
typedef struct _log_recorddata_default
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
    uint16_t          user;
} log_recorddata_default_t;

/*!
 * @brief Structure that defines a log entry with an associated counter.
 */
typedef struct _log_recorddata_error_count
{
    log_source_id_t   source;
    log_category_id_t category;
    uint16_t          errorCode;
    uint16_t          user;
    uint16_t          count;
}   log_recorddata_error_count_t;

/*!
 * @brief Structure that defines fault log entry further specified by an ID. ID can relate to a motor, PSB or PMIC.
 */
typedef struct _log_recorddata_fault_with_id_t
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
    uint8_t			  id;
} log_recorddata_fault_with_id_t;

/*!
 * @brief Structure that defines a fault handling application error log entry.
 */
typedef struct _log_recorddata_fault_without_id_t
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
} log_recorddata_fault_without_id_t;

/*!
 * @brief Structure that defines a system log entry.
 */
typedef struct _log_recorddata_system_t
{
    log_source_id_t   source;
    log_category_id_t category;
    log_event_code_t  eventCode;
} log_recorddata_system_t;

 /*!
 * @brief Union that groups all available log data formats.
 *
 * Implementation hint: When defining new formats, extend log_record_type_id_t as well.
 */
typedef union _log_recorddata
{
    log_recorddata_default_t defaultData;
    log_recorddata_fault_with_id_t faultDataWithID;
    log_recorddata_fault_without_id_t faultDataWithoutID;
    log_recorddata_system_t systemData;
    log_recorddata_error_count_t errorCount;
} log_recorddata_t;

/*!
 * @brief A plain log record
 */
typedef struct _log_record
{
    record_head_t     rhead;
    uint32_t          type;    /*!< Log data format used for the data field */
    log_recorddata_t  data;    /*!< Additional data. Interpretation according to the type field. */
} log_record_t;

/*!
 * @brief An encrypted log record
 */
typedef struct __attribute__((__packed__)) _log_enc_data {
	uint8_t keyiv_enc[LCRYPTO_EX_RSA_KEY_SIZE];
	uint8_t lr_enc[ MAKE_EVEN(sizeof(log_record_t))];
} log_enc_data;

typedef struct __attribute__((__packed__)) _log_encrypted_record {
	size_t length;
	log_enc_data data;
	uint8_t data_signature[LCRYPTO_EX_SIGN_SIZE];
} log_encrypted_record_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#endif /* _API_LOGGING_H_ */

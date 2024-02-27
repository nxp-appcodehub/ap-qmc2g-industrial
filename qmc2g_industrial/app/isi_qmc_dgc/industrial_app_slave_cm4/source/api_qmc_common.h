/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_QMC_COMMON_H_
#define _API_QMC_COMMON_H_

#include <stdint.h>
#include "fsl_common.h"
#ifdef FSL_RTOS_FREE_RTOS
  #include "FreeRTOS.h"
  #include "event_groups.h"
  #include "queue.h"
#endif



/*******************************************************************************
 * Definitions => Event bits
 ******************************************************************************/

/* Input & Button Status */
#define QMC_IOEVENT_BTN1_PRESSED  (UINT32_C(1) <<  0U)
#define QMC_IOEVENT_BTN1_RELEASED (UINT32_C(1) <<  1U)
#define QMC_IOEVENT_BTN2_PRESSED  (UINT32_C(1) <<  2U)
#define QMC_IOEVENT_BTN2_RELEASED (UINT32_C(1) <<  3U)
#define QMC_IOEVENT_BTN3_PRESSED  (UINT32_C(1) <<  4U)
#define QMC_IOEVENT_BTN3_RELEASED (UINT32_C(1) <<  5U)
#define QMC_IOEVENT_BTN4_PRESSED  (UINT32_C(1) <<  6U)
#define QMC_IOEVENT_BTN4_RELEASED (UINT32_C(1) <<  7U)
#define QMC_IOEVENT_INPUT0_HIGH   (UINT32_C(1) <<  8U)
#define QMC_IOEVENT_INPUT0_LOW    (UINT32_C(1) <<  9U)
#define QMC_IOEVENT_INPUT1_HIGH   (UINT32_C(1) << 10U)
#define QMC_IOEVENT_INPUT1_LOW    (UINT32_C(1) << 11U)
#define QMC_IOEVENT_INPUT2_HIGH   (UINT32_C(1) << 12U)
#define QMC_IOEVENT_INPUT2_LOW    (UINT32_C(1) << 13U)
#define QMC_IOEVENT_INPUT3_HIGH   (UINT32_C(1) << 14U)
#define QMC_IOEVENT_INPUT3_LOW    (UINT32_C(1) << 15U)
#define QMC_IOEVENT_INPUT4_HIGH   (UINT32_C(1) << 16U)
#define QMC_IOEVENT_INPUT4_LOW    (UINT32_C(1) << 17U)
#define QMC_IOEVENT_INPUT5_HIGH   (UINT32_C(1) << 18U)
#define QMC_IOEVENT_INPUT5_LOW    (UINT32_C(1) << 19U)
#define QMC_IOEVENT_INPUT6_HIGH   (UINT32_C(1) << 20U)
#define QMC_IOEVENT_INPUT6_LOW    (UINT32_C(1) << 21U)
#define QMC_IOEVENT_INPUT7_HIGH   (UINT32_C(1) << 22U)
#define QMC_IOEVENT_INPUT7_LOW    (UINT32_C(1) << 23U)

/* System Status */
#define QMC_SYSEVENT_LIFECYCLE_Commissioning     (UINT32_C(1) <<  0U)
#define QMC_SYSEVENT_LIFECYCLE_Operational       (UINT32_C(1) <<  1U)
#define QMC_SYSEVENT_LIFECYCLE_Error             (UINT32_C(1) <<  2U)
#define QMC_SYSEVENT_LIFECYCLE_Maintenance       (UINT32_C(1) <<  3U)
#define QMC_SYSEVENT_LIFECYCLE_Decommissioning   (UINT32_C(1) <<  4U)
#define QMC_SYSEVENT_FAULT_Motor1                (UINT32_C(1) <<  5U)
#define QMC_SYSEVENT_FAULT_Motor2                (UINT32_C(1) <<  6U)
#define QMC_SYSEVENT_FAULT_Motor3                (UINT32_C(1) <<  7U)
#define QMC_SYSEVENT_FAULT_Motor4                (UINT32_C(1) <<  8U)
#define QMC_SYSEVENT_FAULT_System                (UINT32_C(1) <<  9U)
#define QMC_SYSEVENT_FWUPDATE_RestartRequired    (UINT32_C(1) << 10U)
#define QMC_SYSEVENT_FWUPDATE_VerifyMode         (UINT32_C(1) << 11U)
#define QMC_SYSEVENT_CONFIGURATION_ConfigChanged (UINT32_C(1) << 12U)
#define QMC_SYSEVENT_ANOMALYDETECTION_Audio      (UINT32_C(1) << 13U)
#define QMC_SYSEVENT_ANOMALYDETECTION_Current    (UINT32_C(1) << 14U)
#define QMC_SYSEVENT_SHUTDOWN_PowerLoss          (UINT32_C(1) << 15U)
#define QMC_SYSEVENT_SHUTDOWN_WatchdogReset      (UINT32_C(1) << 16U)
#define QMC_SYSEVENT_MEMORY_SdCardAvailable      (UINT32_C(1) << 17U)
#define QMC_SYSEVENT_NETWORK_TsnSyncLost         (UINT32_C(1) << 18U)
#define QMC_SYSEVENT_NETWORK_NoLink              (UINT32_C(1) << 19U)
#define QMC_SYSEVENT_LOG_FlashError              (UINT32_C(1) << 20U)
#define QMC_SYSEVENT_LOG_LowMemory               (UINT32_C(1) << 21U)
#define QMC_SYSEVENT_LOG_MessageLost             (UINT32_C(1) << 22U)

/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/

/*!
 * @brief Common API return values.
 */
typedef enum _qmc_status
{
    kStatus_QMC_Ok            = kStatus_Success,                         /*!< Operation was successful; no errors occurred. */
    kStatus_QMC_Err           = kStatus_Fail,                            /*!< General error; not further specified. */
    kStatus_QMC_ErrRange      = kStatus_OutOfRange,                      /*!< Error in case input argument is out of range e.g. array out of bounds, value not defined in enumeration. */
    kStatus_QMC_ErrArgInvalid = kStatus_InvalidArgument,                 /*!< Argument invalid e.g. wrong type, NULL pointer, etc. */
    kStatus_QMC_Timeout       = kStatus_Timeout,                         /*!< Operation was not carried out because a timeout occurred. */
    kStatus_QMC_ErrBusy        = MAKE_STATUS(kStatusGroup_QMC, 7),       /*!< Error because device or resource is busy. */
    kStatus_QMC_ErrMem         = MAKE_STATUS(kStatusGroup_QMC, 8),       /*!< Out of memory error. */
    kStatus_QMC_ErrSync        = MAKE_STATUS(kStatusGroup_QMC, 9),       /*!< Synchronization error when accessing shared resources. */
    kStatus_QMC_ErrNoMsg       = MAKE_STATUS(kStatusGroup_QMC, 10),      /*!< Error if a message shall be retrieved (e.g. from a message queue) but is not available. */
    kStatus_QMC_ErrInterrupted = MAKE_STATUS(kStatusGroup_QMC, 11),      /*!< Operation could not be executed because it was interrupted. */
    kStatus_QMC_ErrNoBufs      = MAKE_STATUS(kStatusGroup_QMC, 12),      /*!< Given buffer space is not sufficient. */
    kStatus_QMC_ErrInternal    = MAKE_STATUS(kStatusGroup_QMC, 13),      /*!< An internal error occurred. */
    kStatus_QMC_ErrSignatureInvalid = MAKE_STATUS(kStatusGroup_QMC, 14)  /*!< A signature check failed. */
} qmc_status_t;

/*!
 * @brief Enumeration of weekdays.
 */
typedef enum _qmc_weekday
{
    kQMC_Sunday    = 0U,
    kQMC_Monday    = 1U,
    kQMC_Tuesday   = 2U,
    kQMC_Wednesday = 3U,
    kQMC_Thursday  = 4U,
    kQMC_Friday    = 5U,
    kQMC_Saturday  = 6U,
} qmc_weekday_t;

/*!
 * @brief Enumeration of user controllable output pins.
 *
 * Consists of regular digital user output pins, digital user output pins in the SNVS domain and SPI slave select pins for the power stage boards.
 */
typedef enum _qmc_output_id
{
    kQMC_UserOutput1 = 0x0001U,
    kQMC_UserOutput2 = 0x0002U,
    kQMC_UserOutput3 = 0x0004U,
    kQMC_UserOutput4 = 0x0008U,
    kQMC_UserOutput5 = 0x0010U,
    kQMC_UserOutput6 = 0x0020U,
    kQMC_UserOutput7 = 0x0040U,
    kQMC_UserOutput8 = 0x0080U,
    kQMC_PsbSpiSel0  = 0x0100U,
    kQMC_PsbSpiSel1  = 0x0200U,
} qmc_output_id_t;

/*!
 * @brief SPI device selection options for the power stage boards.
 */
typedef enum _qmc_spi_id
{
    kQMC_SpiNone        = 0U, /*!< Deselect active SPI slave device. */
    kQMC_SpiMotorDriver = 1U, /*!< Select the MC34GD3000 motor pre-drivers as active SPI slave device. */
    kQMC_SpiAfe         = 2U, /*!< Select the analogue frontend as active SPI slave device. */
    kQMC_SpiAbsEncoder  = 3U, /*!< Select the absolute encoder as active SPI slave device. */
} qmc_spi_id_t;

/*!
 * @brief Enumeration of user LEDs.
 */
typedef enum _qmc_led_id
{
    kQMC_Led1 = 0x0020U,
    kQMC_Led2 = 0x0040U,
    kQMC_Led3 = 0x0080U,
    kQMC_Led4 = 0x0100U,
} qmc_led_id_t;

/*!
 * @brief Defines the state of one of the eight digital user output pins.
 */
typedef enum _qmc_output_cmd
{
    kQMC_CmdOutputClear  = 0U, /*!< Low level on the selected output pin */
    kQMC_CmdOutputSet    = 1U, /*!< High level on the selected output pin */
    kQMC_CmdOutputToggle = 2U, /*!< Change level on the selected output pin e.g. from high to low or low to high */
} qmc_output_cmd_t;

/*!
 * @brief Defines the state of one of the four digital user LEDs.
 */
typedef enum _qmc_led_cmd
{
    kQMC_CmdLedOff    = 0U, /*!< Turn LED off. */
    kQMC_CmdLedOn     = 1U, /*!< Turn LED on. */
    kQMC_CmdLedToggle = 2U, /*!< Switch LED state from "on" to "off" or from "off" to "on". */
} qmc_led_cmd_t;

/*!
 * @brief Represents the lifecycle states of a QMC device.
 *
 * Values are aligned with the lifecycle state representation in the System Status event group.
 */
typedef enum _qmc_lifecycle
{
    kQMC_LcCommissioning   = 0x01U,
    kQMC_LcOperational     = 0x02U,
    kQMC_LcError           = 0x04U,
    kQMC_LcMaintenance     = 0x08U,
    kQMC_LcDecommissioning = 0x10U,
} qmc_lifecycle_t;

/*!
 * @brief Represents firmware update activities that the main application firmware can send to the SBL for execution upon the next boot.
 */
typedef enum _qmc_fw_update_state
{
    kFWU_Revert         = 0x01U, /*!< Program the recovery image */
    kFWU_Commit         = 0x02U, /*!< Commit new version of FW into SE and create new recovery image */
    kFWU_BackupCfgData  = 0x04U, /*!< Back up configuration data */
    kFWU_AwdtExpired    = 0x08U, /*!< Authenticated watchdog expired; reboot keeping this state in SNVS_LP_GPR (main FW must go into maintenance mode) */
    kFWU_VerifyFw       = 0x10U, /*!< After FW update is programmed, SBL indicates to the new FW that a self-check must be performed to provide kFWU_Commit or kFWU_Revert status finalizing the FW update process. */
	kFWU_TimestampIssue = 0x20U, /*!< Firmware timestamp could not be checked, e.g. timestamp is in the future / RTC not set correctly. */
} qmc_fw_update_state_t;

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

/*!
 * @brief  Enumeration of temperature sensors on the Power Stage Boards.
 */
typedef enum _qmc_psb_temperature_id
{
	kQMC_Psb1Sensor1 = 0x10U, /*!< First temperature sensor on PSB 1 */
	kQMC_Psb2Sensor1 = 0x11U, /*!< First temperature sensor on PSB 2 */
	kQMC_Psb3Sensor1 = 0x12U, /*!< First temperature sensor on PSB 3 */
	kQMC_Psb4Sensor1 = 0x13U, /*!< First temperature sensor on PSB 4 */
	kQMC_Psb1Sensor2 = 0x20U, /*!< Second temperature sensor on PSB 1 */
	kQMC_Psb2Sensor2 = 0x21U, /*!< Second temperature sensor on PSB 2 */
	kQMC_Psb3Sensor2 = 0x22U, /*!< Second temperature sensor on PSB 3 */
	kQMC_Psb4Sensor2 = 0x23U, /*!< Second temperature sensor on PSB 4 */
} qmc_psb_temperature_id_t;

/*!
 * @brief  Enumeration of Datalogger wake up notification types.
 */
typedef enum
{
	kDLG_LOG_Queued		  		= 0x01U, 			/*!< Notification of a new log request being triggered */
	kDLG_SHUTDOWN_PowerLoss   	= 0x02U, 			/*!< Notification of a power loss event */
	kDLG_SHUTDOWN_SecureWatchdogReset = 0x04U, 		/*!< Notification of a secure watchdog reset event */
	kDLG_SHUTDOWN_FunctionalWatchdogReset = 0x08U, 	/*!< Notification of a functional watchdog reset event */
} qmc_dlg_notification_t;

/*******************************************************************************
 * Definitions => Related to Structures
 ******************************************************************************/
#define QMC_MEM_WRITE_MAX_DATA_WORDS (16U)

/*******************************************************************************
 * Definitions => Structures
 ******************************************************************************/

/*!
 * @brief A decomposition of the qmc_timestamp_t type that can be used for direct display or other cases where a human readable timestamp format is required.
 *
 * BoardAPI provides the functions  BOARD_ConverTimestamp2Datetime(timestamp : qmc_timestamp_t*, dt : qmc_datetime_t*) : qmc_status_t and
 * BOARD_ConvertDatetime2Timestamp(dt : qmc_datetime_t*, timestamp : qmc_timestamp_t*) : qmc_status_t that allow to convert between the
 * two formats.
 */
typedef struct q_mc_datetime
{
    uint16_t      year;
    uint8_t       month;
    uint8_t       day;
    qmc_weekday_t dayOfWeek;
    uint8_t       hour;
    uint8_t       minute;
    uint8_t       second;
    uint16_t      millisecond;
} qmc_datetime_t;

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

#ifdef FSL_RTOS_FREE_RTOS
/*!
 * @brief Represents a message queue with its associated events.
 */
typedef struct _qmc_msg_queue_handle
{
    QueueHandle_t      *queueHandle;
    EventGroupHandle_t *eventHandle;
    EventBits_t        eventMask;
} qmc_msg_queue_handle_t;
#endif

/*!
 * @brief Represents a memory write request.
 */
typedef struct _qmc_mem_write
{
    uintptr_t baseAddress;
    uint32_t data[QMC_MEM_WRITE_MAX_DATA_WORDS];
    uint8_t dataWords;
    uint8_t accessSize;
} qmc_mem_write_t;

/*******************************************************************************
 * Definitions => Other
 ******************************************************************************/

#endif /* _API_QMC_COMMON_H_ */

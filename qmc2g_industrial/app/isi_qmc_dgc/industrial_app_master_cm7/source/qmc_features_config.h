/*
 * Copyright 2022-2023 NXP  
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/**
 * QMC features and configuration
 */

#ifndef _QMC_FEATURES_CONFIG_H_
#define _QMC_FEATURES_CONFIG_H_

#include <stdint.h>

/*******************************************************************************
 * Motor Control (MC)
 ******************************************************************************/

/* FEATURES */
#define FEATURE_MC_INVERTER_OUTPUT_DEBUG_ENABLE (0) /* Enable the capability of outputting PWM in ready state directly */
#define FEATURE_MC_LOOP_BANDWIDTH_TEST_ENABLE   (0) /* Enable the test of current loop bandwidth in current FOC mode.
                                                       Enable the test of speed loop bandwidth in speed FOC mode.
                                                       Enable the test of position loop bandwidth in position FOC mode */
#define FEATURE_MC_PSB_TEMPERATURE_FAULTS		(0)
#define FEATURE_TSN_DEBUG_ENABLED				(0)
#define FEATURE_TSN_PRINT_IF					(1)
#define FEATURE_FREEMASTER_ENABLE				(1) // TODO: Disable freemaster in the final release
#define FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB	(1) // TODO: Disable it in the final release because freemaster is disabled
#define FEATURE_TOOGLE_USER_LED_ENABLE			(0)
#define FEATURE_HANDLE_BUTTON_PRESS_EVENTS		(1) /* When no lid is installed over the buttons, pressing a button will be detected as a tampering event */

/* CONFIGURATION */
#define MC_MAX_MOTORS        (4)          /* Number of supported motors */
#define MC_LIMIT_H_SPEED     (4000.0f)    /* Upper speed limit in RPM when using kMC_FOC_SpeedControl */
#define MC_LIMIT_L_SPEED     (-4000.0f)   /* Lower speed limit in RPM when using kMC_FOC_SpeedControl */
#define MC_LIMIT_H_GAIN      (0.2f)       /* Upper limit for the gain in V/HZ when using kMC_ScalarControl */
#define MC_LIMIT_L_GAIN      (0.0f)       /* Lower limit for the gain in V/HZ when using kMC_ScalarControl */
#define MC_LIMIT_H_FREQUENCY (40.0f)      /* Upper limit for the frequency in HZ when using kMC_ScalarControl */
#define MC_LIMIT_L_FREQUENCY (-40.0f)     /* Lower limit for the frequency in HZ when using kMC_ScalarControl */
#define MC_LIMIT_H_POSITION  (100 << 16)  /* Upper limit for the position (revolutions) in Q16.16 format when using kMC_FOC_PositionControl */
#define MC_LIMIT_L_POSITION  (-100 << 16) /* Lower limit for the position (revolutions) in Q16.16 format when using kMC_FOC_PositionControl */



/*******************************************************************************
 * Data Hub
 ******************************************************************************/

/* FEATURES */

/* CONFIGURATION */
#define DATAHUB_MAX_STATUS_QUEUES           (4)
#define DATAHUB_STATUS_QUEUE_LENGTH         (10)
#define DATAHUB_COMMAND_QUEUE_LENGTH        (10)
#define DATAHUB_STATUS_SAMPLING_INTERVAL_MS (100)



/*******************************************************************************
 * Anomaly Detection (AD)
 ******************************************************************************/

/* FEATURES */
#define FEATURE_ANOMALY_DETECTION             (0)
#define FEATURE_ANOMALY_DETECTION_SYNC_CHECK  (0)

/* CONFIGURATION */
#define AD_CURRENT_BLK_SIZE (8)              /* Number of motor status values per block */

#define MC_HAS_AFE_MOTOR1 (1) /* Defines if PSB1 has the AFE soldered on. 1 means the AFE is soldered, 0 means the AFE is missing. */
#define MC_HAS_AFE_MOTOR2 (1) /* Defines if PSB2 has the AFE soldered on. 1 means the AFE is soldered, 0 means the AFE is missing. */
#define MC_HAS_AFE_MOTOR3 (1) /* Defines if PSB3 has the AFE soldered on. 1 means the AFE is soldered, 0 means the AFE is missing. */
#define MC_HAS_AFE_MOTOR4 (1) /* Defines if PSB4 has the AFE soldered on. 1 means the AFE is soldered, 0 means the AFE is missing. */

#define MC_HAS_AFE_ANY_MOTOR ((MC_HAS_AFE_MOTOR1) | (MC_HAS_AFE_MOTOR2 << 1) | (MC_HAS_AFE_MOTOR3 << 2) | (MC_HAS_AFE_MOTOR4 << 3))
#define MC_PSBx_HAS_AFE(x) (MC_HAS_AFE_ANY_MOTOR & (1 << x))

/*******************************************************************************
 * Board API
 ******************************************************************************/

/* FEATURES */
#define FEATURE_BOARD_SANITY_CHECK_DAY_OF_WEEK   (1)

/* CONFIGURATION */
#define BOARD_GETTIME_REFRESH_INTERVAL_S (24*60*60) /* After this time the tick-based time is synchronized to the RTC again */

/*******************************************************************************
 * Fault Detection
 ******************************************************************************/

#define PSB_TEMP1_THRESHOLD (85.0)
#define PSB_TEMP2_THRESHOLD (75.0)
#define DB_TEMP_THRESHOLD (90.0)
#define MCU_TEMP_THRESHOLD (100.0)
#define FAULT_HANDLING_FUNCTIONAL_WATCHDOG_KICK_PERIOD_IN_MS (5000)

/*******************************************************************************
 * Configurations
 ******************************************************************************/
#define FEATURE_CONFIG_DBG_PRINT
#define FEATURE_LCRYPT_DBG_PRINT
#define FEATURE_DATALOGGER_RECORDER_DBG_PRINT
#define FEATURE_DATALOGGER_DISPATCHER_DBG_PRINT
#define FEATURE_DATALOGGER_SDCARD_DBG_PRINT
#define FEATURE_CLOUD_DBG_PRINT

//Time to wait for grant access by mutex
#define CONFIG_MUTEX_XDELAYS_MS (1000)

//Size parameters for configuration items
#define CONFIG_MAX_KEY_LEN     (32)
#define CONFIG_MAX_VALUE_LEN  (128)
#define CONFIG_MAX_RECORDS	  (128)

//Configuration size for 32/128/128
//sizeof( cnf_record_t) = 161
//sizeof( cnf_struct_t) = sizeof( cnf_record_t) * CONFIG_MAX_RECORDS + CONFIGURATION_HASH_SIZE = 161 * 128 + 32 = 20640
//FLASH_CONFIG_SECTORS = sizeof( cnf_struct_t) / OCTAL_FLASH_SECTOR_SIZE = 20640 / 4096 = 5.039 = 6
//in 6 sectors fits ( 6 * 4096 - 32 ) / 161 = 152
//in 8 sectors fits ( 8 * 4096 - 32 ) / 161 = 203

//Size of Configuration in sectors
#define FLASH_CONFIG_SECTORS 6U


/*******************************************************************************
 * Datalogger
 ******************************************************************************/

//Time to wait for grant access by mutex
#define DATALOGGER_MUTEX_XDELAYS_MS (800)

//Depth of receiving datalogger task queue
#define DATALOGGER_RCV_QUEUE_DEPTH 10U

//dynamicly allocated queue for 3th party services
#define  FEATURE_DATALOGGER_DQUEUE
#define  FEATURE_DATALOGGER_DQUEUE_EVENT_BITS

//Depth of dynamicly allocated queue
#define DATALOGGER_DYNAMIC_RCV_QUEUE_DEPTH 10U

//Max number of dynamicy allocated dynamic queues
#define DATALOGGER_RCV_QUEUE_CN 2U

//Report LOW_MEMORY if LOW_MEMORY_TRESHOLD is triggered
#define DATALOGGER_REPORT_LOW_MEMORY
//LOW_MEMORY_TRESHOLD in percentage (20 means signals low memory if free space is less than 20%)
#define DATALOGGER_LOW_MEMORY_TRESHOLD (20U)

//Sync records stored into the NOR flash by SBL with the SDCARD
#define FEATURE_DATALOGGER_SYNC_WITH_SBL

//SDCARD implementation
#define FEATURE_DATALOGGER_SDCARD
#define DATALOGGER_SDCARD_DIRPATH "/dat"
#define DATALOGGER_SDCARD_FILEPATH "/dat/datfile.bin"
#define DATALOGGER_SDCARD_MAX_FILESIZE (10000000U)
#define DATALOGGER_SDCARD_FATFS_DELAYED_MOUNT

//Octal flash start address of recorder
#define FLASH_RECORDER_ORIGIN   0x30002000

//LogRecorder size
//Number of max stored log_records = FLASH_RECORDER_SECTORS * ( OCTAL_FLASH_SECTOR_SIZE / sizeof(log_record_t))
//log_records = 32 * (4096 / 68) = 32 * 60 = 1920

//Size of LogRecorder in sectors
#define FLASH_RECORDER_SECTORS 32U

/******************************************************************************
 * Input signal interrupts configuration
 ******************************************************************************/
#define FEATURE_ENABLE_GPIO_SW_DEBOUNCING (1)
#define FEATURE_DETECT_POWER_LOSS (1)
#define GPIO_SW_DEBOUNCE_MS (5U)

/*******************************************************************************
 * Cyber Resilience 
 ******************************************************************************/
/* Enables the secure watchdog */
#define FEATURE_SECURE_WATCHDOG (0)

/* URL of the secure watchdog ticket server
 * (client attempts to connect to port 443 as TLS must be used) */
#define SECURE_WATCHDOG_HOST "api.awdt.server"

/* Delay in seconds between secure watchdog kick sequences
 * A kick sequence consists of fetching the secure watchdog's current nonce,
 * requesting a deferral ticket from the server and trying to kick the watchdog
 * with the received ticket.
 * Note that this delay must be lower than the secure watchdog's initial timeout
 * (or any new timeout included in a ticket) minus the maximum time required for
 * the kicking sequence (around 20s depending on network operations). */
#define SECURE_WATCHDOG_KICK_INTERVAL_S (60U)

/* Retry delay in seconds after a failed secure watchdog kick sequence
 * Same value restrictions as for SECURE_WATCHDOG_KICK_INTERVAL_S apply. */
#define SECURE_WATCHDOG_KICK_RETRY_S (30U)

/* Timeout in milliseconds for network operations with the SW server */
#define SECURE_WATCHDOG_SOCKET_TIMEOUT_MS (1000U)

/*******************************************************************************
 * Local Service
 ******************************************************************************/

#define COLOR_MAIN_DARK		0x00, 0x1d, 0x4a
#define COLOR_MAIN_LIGHT	0xff, 0xfa, 0xff
#define COLOR_OPERATIONAL	0x1b, 0x99, 0x8b
#define COLOR_MAINTENANCE	0xff, 0xa6, 0x2b
#define COLOR_FAULT			0xcc, 0x29, 0x36
#define COLOR_ERROR			COLOR_FAULT
#define COLOR_OFF			COLOR_MAIN_LIGHT

#define TASK_DELAY_MS 100
#define GUI_HANDLER_DELAY_AT_LEAST_MS 1000
#define MOTOR_STATUS_AND_LOGS_DELAY_AT_LEAST_MS 100
#define DEFAULT_MOTOR_SPEED 200.0f

#define GUI_MAX_MESSAGE_LENGTH 33
#define GUI_MAX_TIMESTAMP_LENGTH 20

#define QMC_IOEVENT_EMERGENCY_PRESSED QMC_IOEVENT_INPUT3_HIGH
#define QMC_IOEVENT_EMERGENCY_RELEASED QMC_IOEVENT_INPUT3_LOW
#define QMC_IOEVENT_LID_OPEN_SD QMC_IOEVENT_INPUT1_HIGH
#define QMC_IOEVENT_LID_CLOSE_SD QMC_IOEVENT_INPUT1_LOW
#define QMC_IOEVENT_LID_OPEN_BUTTON QMC_IOEVENT_INPUT2_HIGH
#define QMC_IOEVENT_LID_CLOSE_BUTTON QMC_IOEVENT_INPUT2_LOW

#define FEATURE_LCD_POWER_PIN_USED (0)
#define FEATURE_LCD_RESET_PIN_USED (0)
#define FEATURE_LCD_TOUCH_PINS_USED (0)

/*******************************************************************************
 * Secure Element: SE051
 ******************************************************************************/

#define SE051_APPLET_VERSION_06_00 (0)
#define SE051_APPLET_VERSION_07_02 (1)
										  
/* For finer control over logging from the SE
 * middleware, check nxLog_DefaultConfig.h */
#define FEATURE_SE_DEBUG_ENABLED   (1)

/*******************************************************************************
 * User Management
 ******************************************************************************/

/*
 * number of concurrent sessions
 */
#define USRMGMT_MAX_SESSIONS 8

/*
 * number of sessions reserved for users with role "maintenance head"
 */
#define USRMGMT_RESERVED_SESSIONS 2

/*
 * minimal passphrase length
 */
#define USRMGMT_MIN_PASSPHRASE_LENGTH 8

/* Character classification classes:
 * UPPERCASE LOWERCASE NUMBERS SPECIAL CONTROL NON_ASCII
 *
 * set required and rejected character classes as requirement for the
 * user's passphrase
 */
#define USRMGMT_PASSPHRASE_REQUIRED_CLASSESS() UPPERCASE LOWERCASE NUMBERS
#define USRMGMT_PASSPHRASE_REJECTED_CLASSESS() CONTROL

/* User Lockout duration after unsuccessful authentication attempts
 * in seconds
 */
#define USRMGMT_LOCKOUT_DURATION 300

/* Session duration limit
 * in seconds
 */
#define USRMGMT_SESSION_DURATION (60*60*2)

/* Passphrase validity duration limit
 * in seconds
 */
#define USRMGMT_PASSPHRASE_DURATION (60*60*24*60)

/* Number of attempts a locked account has to unlock until the lock is enforced.
 * Accounts get locked immediately after the first authentication failure.
 * If the system stays on, they have this amount of attempts to unlock.
 * during the lockout period without waiting.
 */
#define USRMGMT_AUTHENTICATION_ATTEMPTS (5U)


/*
 * size of the session authentication secret (in bytes)
 */
#define USRMGMT_SESSION_SECRET_LENGTH  (32U)

/*
 * size of the user authentication secret (in bytes)
 */
#define USRMGMT_USER_SECRET_LENGTH     (32U)

/*
 * size of the user authentication salt (in bytes)
 */
#define USRMGMT_SALT_LENGTH            (16U)


/*
 * user name length limit
 */
#define USRMGMT_USER_NAME_MAX_LENGTH   (32U)

/*
 * password buffer length, limit on password length
 * size limit is used to perform password 
 * analytics in constant time
 */
#define USRMGMT_PASSWORD_BUFFER_LENGTH (255U)

/*
 * size limit of the JWT json payload
 */
#define USRMGMT_PAYLOAD_BUFFER_LENGTH  (255U)

/*
 * PBKDF2 hash function to use
 */
#define USRMGMT_PASSPHRASE_HASH MBEDTLS_MD_SHA1

/*
 * PBKDF2 iterations for the password hash
 */
#define USRMGMT_MIN_PASSPHRASE_ITERATIONS (2000U)

/*******************************************************************************
 * Webservice
 ******************************************************************************/

/*
 * HTTPD error code logging interval, in seconds
 */
#define WEBSERVICE_HTTPD_ERROR_LOG_INTERVAL (120U)


/*
 * Firmware Upload sector write retry count
 */
#define WEBSERVICE_FIRMWARE_UPLOAD_WRITE_RETRIES (4U)

/*******************************************************************************
 * Cloud Service
 ******************************************************************************/

/* Only one type of cloud service can be selected at a time,
 * the check below should be updated to reflect the flags */
#define FEATURE_CLOUD_AZURE_IOTHUB (1)
#define FEATURE_CLOUD_GENERIC_MQTT (0)

#define INTERFACE_SETUP_DELAY_MS (2500)

/*******************************************************************************
 * Remote Procedure Call Interface
 ******************************************************************************/

/* Wait time in ms when calling RPC_Reset() before the request is forwarded
 * to the M4. During this wait period the M7 has time to write pending logs. */
#define RPC_WAIT_MS_BEFORE_RESET (5000U) 

/*******************************************************************************
 * Compile time checks
 ******************************************************************************/

#if( (RECORDER_REC_INF_DATALOGGER_AREABEGIN + RECORDER_REC_INF_DATALOGGER_AREALENGTH) > UINT32_MAX )
#error "Datalogger Inf area space exceeds UINT32_MAX"
#endif

#if( (RECORDER_REC_DATALOGGER_AREABEGIN + RECORDER_REC_DATALOGGER_AREALENGTH) > UINT32_MAX )
#error "Datalogger data area space exceeds UINT32_MAX"
#endif

#if BOARD_GETTIME_REFRESH_INTERVAL_S > UINT32_MAX
    #error "BOARD_GETTIME_REFRESH_INTERVAL_S must not exceed UINT32_MAX"
#endif

#if( (MC_MAX_MOTORS < 1) || (MC_MAX_MOTORS > 4) )
    #error "Number of motors must be in the range of 1 - 4."
#endif

#if MC_LIMIT_L_POSITION >= MC_LIMIT_H_POSITION
#error "Motor control position limit: Lower limit is bigger than or equal to upper limit."
#endif

#if( (FEATURE_ANOMALY_DETECTION_SYNC_CHECK != 0) && (FEATURE_ANOMALY_DETECTION == 0) )
    #error "AD sync. check requires AD feature to be enabled."
#endif

#if BOARD_GETTIME_REFRESH_INTERVAL_S > UINT32_MAX
    #error "BOARD_GETTIME_REFRESH_INTERVAL_S must not exceed UINT32_MAX"
#endif

#if(DATAHUB_STATUS_QUEUE_LENGTH < MC_MAX_MOTORS)
    #error "Status queue length cannot be smaller than the number of motors."
#endif

#if ((MC_HAS_AFE_MOTOR1 > 1 || MC_HAS_AFE_MOTOR1 < 0) || (MC_HAS_AFE_MOTOR2 > 1 || MC_HAS_AFE_MOTOR2 < 0) ||\
	 (MC_HAS_AFE_MOTOR3 > 1 || MC_HAS_AFE_MOTOR3 < 0) || (MC_HAS_AFE_MOTOR4 > 1 || MC_HAS_AFE_MOTOR4 < 0))
#error "MC_HAS_AFE_MOTORx can only be configured as 0 or 1!"
#endif

#if	((MC_MAX_MOTORS < 4 && MC_HAS_AFE_MOTOR4) || (MC_MAX_MOTORS < 3 && MC_HAS_AFE_MOTOR3) ||\
	 (MC_MAX_MOTORS < 2 && MC_HAS_AFE_MOTOR2) || (MC_MAX_MOTORS < 1 && MC_HAS_AFE_MOTOR1))
#error "AFE defined as soldered on a PSB that is defined as not connected!"
#endif

#if ((FEATURE_ENABLE_GPIO_SW_DEBOUNCING) && (GPIO_SW_DEBOUNCE_MS < 0))
#error "Debouncing time cannot be shorter than 0 ms!"
#endif

#if (defined(DATALOGGER_REPORT_LOW_MEMORY) && ((DATALOGGER_LOW_MEMORY_TRESHOLD > 100) || (DATALOGGER_LOW_MEMORY_TRESHOLD < 0)))
#error "Macro DATALOGGER_LOW_MEMORY_TRESHOLD out of range! <0,100> % allowed."
#endif

#if ((FLASH_RECORDER_ORIGIN > 0x30400000) || (FLASH_RECORDER_ORIGIN < 0x30002000))
#error "Macro FLASH_RECORDER_ORIGIN out of range! <0x30002000,0x30400000> allowed."
#endif

#if ((DATALOGGER_SDCARD_MAX_FILESIZE > 100000000U) || (DATALOGGER_SDCARD_MAX_FILESIZE < 1000000U))
#error "Macro DATALOGGER_SDCARD_MAX_FILESIZE out of range! <1000000U,100000000U> allowed."
#endif

#if ((DATALOGGER_MUTEX_XDELAYS_MS > 1000U ) || (DATALOGGER_MUTEX_XDELAYS_MS < 300U ))
#error "Macro DATALOGGER_MUTEX_XDELAYS_MS out of range! <300,1000> allowed."
#endif

#if ((CONFIG_MUTEX_XDELAYS_MS > 1000U ) || (CONFIG_MUTEX_XDELAYS_MS < 300U ))
#error "Macro CONFIG_MUTEX_XDELAYS_MS out of range! <300,1000> allowed."
#endif

#if (FEATURE_CLOUD_AZURE_IOTHUB && FEATURE_CLOUD_GENERIC_MQTT)
#        error "A maximum of one type of cloud service can be enabled"
#endif

#if ((WEBSERVICE_FIRMWARE_UPLOAD_WRITE_RETRIES < 0 ) || (WEBSERVICE_FIRMWARE_UPLOAD_WRITE_RETRIES >5 ))
#error "Macro WEBSERVICE_FIRMWARE_UPLOAD_WRITE_RETRIES out of range! <0,5> allowed."
#endif

#if (USRMGMT_RESERVED_SESSIONS > USRMGMT_MAX_SESSIONS )
#error "Macro USRMGMT_RESERVED_SESSIONS out of range! <0,USRMGMT_MAX_SESSIONS > allowed."
#endif

#if (USRMGMT_RESERVED_SESSIONS == USRMGMT_MAX_SESSIONS )
#warning "Macro USRMGMT_RESERVED_SESSIONS: operator user logins not allowed."
#endif

#if ((USRMGMT_MIN_PASSPHRASE_LENGTH < 1 ) || (USRMGMT_MIN_PASSPHRASE_LENGTH > USRMGMT_PASSWORD_BUFFER_LENGTH ))
#error "Macro USRMGMT_MIN_PASSPHRASE_LENGTH out of range! <1,USRMGMT_PASSWORD_BUFFER_LENGTH > allowed."
#endif

#endif /* _QMC_FEATURES_CONFIG_H_ */

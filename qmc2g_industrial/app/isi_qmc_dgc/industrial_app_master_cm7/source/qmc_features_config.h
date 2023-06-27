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
#define FEATURE_MC_PSB_TEMPERATURE_FAULTS (0)
#define FEATURE_TSN_DEBUG_ENABLED         (0)
#define FEATURE_FREEMASTER_ENABLE         (1)   // TODO: Disable freemaster in the final release
#define FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB    (1) // TODO: Disable it in the final release because freemaster is disabled
#define FEATURE_TOOGLE_USER_LED_ENABLE    (1) // TODO: Disable it in the final release because user led is reserved for users
#define FEATURE_HANDLE_BUTTON_PRESS_EVENTS (0) /* When no lid is installed over the buttons, pressing a button will be detected as a tampering event */

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
//TODO: put MC features here

/* CONFIGURATION */
#define DATAHUB_MAX_STATUS_QUEUES           (3)
#define DATAHUB_STATUS_QUEUE_LENGTH         (10)
#define DATAHUB_COMMAND_QUEUE_LENGTH        (10)
#define DATAHUB_STATUS_SAMPLING_INTERVAL_MS (100)



/*******************************************************************************
 * Anomaly Detection (AD)
 ******************************************************************************/

/* FEATURES */
#define FEATURE_ANOMALY_DETECTION             (1)
#define FEATURE_ANOMALY_DETECTION_SYNC_CHECK  (1)

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
//TODO: put Board API features here

/* CONFIGURATION */
#define BOARD_GETTIME_REFRESH_INTERVAL_S (24*60*60) /* After this time the tick-based time is synchronized to the RTC again */

/*******************************************************************************
 * Fault Detection
 ******************************************************************************/

#define PSB_TEMP1_THRESHOLD (85.0)
#define PSB_TEMP2_THRESHOLD (75.0)
#define DB_TEMP_THRESHOLD (90.0)
#define MCU_TEMP_THRESHOLD (100.0)

/*******************************************************************************
 * Configurations
 ******************************************************************************/
#define FEATURE_CONFIG_DBG_PRINT
#define FEATURE_LCRYPT_DBG_PRINT
#define FEATURE_DATALOGGER_RECORDER_DBG_PRINT
#define FEATURE_DATALOGGER_DISPATCHER_DBG_PRINT
#define FEATURE_DATALOGGER_SDCARD_DBG_PRINT

//Time to wait for grant access by mutex
#define CONFIG_MUTEX_XDELAYS_MS (800)

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
#define DATALOGGER_RCV_QUEUE_DEPTH 3U

//dynamicly allocated queue for 3th party services
#define  FEATURE_DATALOGGER_DQUEUE
#define  FEATURE_DATALOGGER_DQUEUE_EVENT_BITS

//Depth of dynamicly allocated queue
#define DATALOGGER_DYNAMIC_RCV_QUEUE_DEPTH 3U

//Max number of dynamicy allocated dynamic queues
#define DATALOGGER_RCV_QUEUE_CN 2U

//SDCARD implementation
#define FEATURE_DATALOGGER_SDCARD
#define DATALOGGER_SDCARD_DIRPATH "/dat"
#define DATALOGGER_SDCARD_FILEPATH "/dat/datfile.bin"
#define DATALOGGER_SDCARD_MAX_FILESIZE (10000000U)

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
#define FEATURE_SECURE_WATCHDOG (0) /* Enables the secure watchdog timer */
#define SECURE_WATCHDOG_HOST "api.awdt.local" /* URL of the secure watchdog ticket server */
#define SECURE_WATCHDOG_KICK_INTERVAL_S (60)  /* Kick interval of the secure watchdog in seconds */
#define SECURE_WATCHDOG_KICK_RETRY_S (30) /* Retry interval in case kicking the secure watchdog failed in seconds */
#define SECURE_WATCHDOG_SOCKET_TIMEOUT_MS (1000U) /* timeout for network operations with the SW server in ms */

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
 * Compile time checks
 ******************************************************************************/

#if BOARD_GETTIME_REFRESH_INTERVAL_S > UINT32_MAX
    #error "BOARD_GETTIME_REFRESH_INTERVAL_S must not exceed UINT32_MAX"
#endif

#if( (MC_MAX_MOTORS < 1) || (MC_MAX_MOTORS > 4) )
    #error "Number of motors must be in the range of 1 - 4."
#endif
#define BOARD_GETTIME_REFRESH_INTERVAL_S (24*60*60)

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

#endif /* _QMC_FEATURES_CONFIG_H_ */

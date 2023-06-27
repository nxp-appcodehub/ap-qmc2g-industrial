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
#define FEATURE_MC_PSB_TEMPERATURE_FAULTS (0)
#define FEATURE_TSN_DEBUG_ENABLED (0)

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



/*******************************************************************************
 * Board API
 ******************************************************************************/

/* FEATURES */
//TODO: put Board API features here

/* CONFIGURATION */
#define BOARD_GETTIME_REFRESH_INTERVAL_S (24*60*60) /* After this time the tick-based time is synchronized to the RTC again */



/*******************************************************************************
 * Cyber Resilience 
 ******************************************************************************/
#define FEATURE_SECURE_WATCHDOG (0) /* Enables the secure watchdog timer */



/*******************************************************************************
 * Compile time checks
 ******************************************************************************/

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

#if(DATAHUB_STATUS_QUEUE_LENGTH < MC_MAX_MOTORS)
    #error "Status queue length cannot be smaller than the number of motors."
#endif

#endif /* _QMC_FEATURES_CONFIG_H_ */

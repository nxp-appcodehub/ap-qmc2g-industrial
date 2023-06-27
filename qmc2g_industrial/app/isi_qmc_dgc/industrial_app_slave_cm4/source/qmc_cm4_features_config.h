/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file qmc_cm4_features_config.h
 * @brief Allows to customize the QMC 2G behavior on the CM4.
 */
#ifndef _QMC_CM4_FEATURES_CONFIG_H_
#define _QMC_CM4_FEATURES_CONFIG_H_

#include "qmc_features_config.h"
#include "hal/hal.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*
 * QMC CM4 user settings
 * NOTE: Customize CM4 behavior here
 */
#define QMC_CM4_SNVS_USER_OUTPUTS_INIT_STATE \
    ((1U << kHAL_SnvsSpiCs0) | (1U << kHAL_SnvsSpiCs1)) /*!< initial output values */

/* Functional watchdogs
 * NOTE: If you add more watchdogs you also have to adapt the HandleFunctionalWatchdogCommand
 * function in the RPC module! */
#define QMC_CM4_FWDGS_COUNT           (1U)    /*!< number of functional watchdogs */
#define QMC_CM4_FWDGS_GRACE_PERIOD_MS (5000U) /*!< TODO grace period of the functional watchdogs in ms */
#define QMC_CM4_FWDGS_TIMEOUT_MS \
    {                            \
        24 * 3600 * 1000U        \
    } /*!< TODO timeout of the individual functional watchdogs in ms */

/* Authenticated / Secure watchdog */
#define QMC_CM4_AWDG_GRACE_PERIOD_MS (5000U) /*!< TODO grace period of the authenticated watchdog in ms */
#define QMC_CM4_AWDG_INITIAL_TIMEOUT_MS \
    (24 * 3600 * 1000U) /*!< TODO initial timeout of the authenticated watchdog in ms */

/* Static RNG seed and PK
 * !!!ONLY for non-SBL TESTING, not for PRODUCTION!!!
 * These dummy values are used if the NO_SBL define is set!
 */
#define QMC_CM4_TEST_AWDG_RNG_SEED                                                                                  \
    {                                                                                                               \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   \
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00                            \
    }                                         /*!< AWDG RNG seed for non-SBL testing */
#define QMC_CM4_TEST_AWDG_RNG_SEED_SIZE (48U) /*!< AWDG RNG seed for non SBL testing length */

#define QMC_CM4_TEST_AWDG_PK                                                                                        \
    {                                                                                                               \
        0x30, 0x81, 0x9b, 0x30, 0x14, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x09, 0x2b, 0x24, \
            0x03, 0x03, 0x02, 0x08, 0x01, 0x01, 0x0d, 0x03, 0x81, 0x82, 0x00, 0x04, 0xa0, 0xd5, 0x64, 0xe8, 0x13,   \
            0x0a, 0x17, 0xd7, 0xff, 0xec, 0xd0, 0x3e, 0xe4, 0xd5, 0xa1, 0x5c, 0x4d, 0x42, 0x30, 0xd7, 0x2b, 0x5c,   \
            0xe2, 0x0b, 0x83, 0x0d, 0xab, 0x25, 0xc1, 0xe7, 0xd8, 0x3b, 0x2d, 0x57, 0x96, 0xfc, 0xa3, 0x14, 0x2d,   \
            0xf9, 0x85, 0xa6, 0x51, 0x20, 0xdb, 0x19, 0x37, 0xc8, 0x0b, 0x27, 0xde, 0x88, 0x4a, 0xc3, 0x06, 0x17,   \
            0xe5, 0xbb, 0x67, 0x3c, 0xa7, 0x6b, 0xac, 0xc8, 0x7d, 0x47, 0x21, 0x8f, 0xfe, 0xbc, 0xb7, 0x67, 0xa5,   \
            0x72, 0x33, 0x1d, 0x0d, 0x8c, 0x4d, 0x0e, 0x6e, 0x84, 0x18, 0x59, 0xe3, 0x97, 0xb2, 0xfb, 0x44, 0xe8,   \
            0x26, 0xfa, 0xb8, 0x42, 0xf0, 0x83, 0xfb, 0x88, 0x0f, 0xb2, 0x60, 0x5f, 0x98, 0xf8, 0xfa, 0x9d, 0x90,   \
            0x88, 0x0b, 0x57, 0xb6, 0x3b, 0xb7, 0x18, 0xb5, 0xee, 0xc9, 0xcc, 0x7a, 0x10, 0x1f, 0xdc, 0xe9, 0x5f,   \
            0x0f, 0xea, 0xac, 0x89                                                                                  \
} /*!< AWDG key for non-SBL testing */
#define QMC_CM4_TEST_AWDG_PK_SIZE (158U) /*!< AWDG PK for non SBL testing length */

#endif

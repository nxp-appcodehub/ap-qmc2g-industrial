/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
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
            0x03, 0x03, 0x02, 0x08, 0x01, 0x01, 0x0d, 0x03, 0x81, 0x82, 0x00, 0x04, 0x5b, 0x7c, 0x55, 0xb3, 0x7c,   \
            0xd8, 0xa1, 0xfe, 0xf0, 0x1a, 0x8a, 0xf6, 0xa4, 0x84, 0x58, 0x02, 0xc5, 0x90, 0x65, 0xc7, 0x0c, 0x3d,   \
            0xdf, 0xf3, 0x89, 0x35, 0x07, 0xe3, 0xb8, 0x43, 0x33, 0xaf, 0xad, 0x6a, 0xfe, 0xfd, 0x0b, 0xce, 0x5a,   \
            0xe5, 0xc9, 0x3b, 0x4f, 0x2d, 0xfd, 0x3e, 0x9f, 0x3e, 0xbc, 0xa7, 0x76, 0xb5, 0x6a, 0x26, 0x9e, 0xf9,   \
            0x15, 0xf4, 0xa3, 0xe4, 0xbd, 0x0c, 0x06, 0xae, 0x25, 0x92, 0x1f, 0x5d, 0xa5, 0xbc, 0x1f, 0x9f, 0xe8,   \
            0x48, 0xa3, 0x75, 0x15, 0xd3, 0xe8, 0xd9, 0xf0, 0x9d, 0x1c, 0x30, 0xf4, 0x21, 0xa5, 0x35, 0x19, 0x39,   \
            0xf5, 0x8b, 0x7c, 0xbf, 0x26, 0x89, 0x4b, 0x16, 0x2b, 0x12, 0x39, 0xf7, 0x44, 0x31, 0xcf, 0xc3, 0xd9,   \
            0x1d, 0x9d, 0x04, 0x9e, 0xa3, 0x45, 0xc6, 0x5c, 0xfc, 0x57, 0xca, 0x2f, 0x30, 0x52, 0xeb, 0x96, 0x11,   \
            0xec, 0x89, 0xc1, 0x46                                                                                  \
    }                                    /*!< AWDG key for non-SBL testing */
#define QMC_CM4_TEST_AWDG_PK_SIZE (158U) /*!< AWDG PK for non SBL testing length */

#endif

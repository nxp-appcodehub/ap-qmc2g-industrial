/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef MC_COMMON_H
#define MC_COMMON_H

#include <mcinit_qmc2g_imxrt1170.h>
#include "m1_pmsm_appconfig.h"
#include "m2_pmsm_appconfig.h"
#include "m3_pmsm_appconfig.h"
#include "m4_pmsm_appconfig.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define RAM_FUNC_CRITICAL __attribute__((section(".ramfunc.$SRAM_ITC_cm7")))
#define ALWAYS_INLINE __attribute__((always_inline))

#endif /* MC_COMMON_H */

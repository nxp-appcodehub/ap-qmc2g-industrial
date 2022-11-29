/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef MC_COMMON_H_
#define MC_COMMON_H_

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

#endif /* MC_COMMON_H_ */

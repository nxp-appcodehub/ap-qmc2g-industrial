/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#ifndef _M4_SM_REF_SOL_H_
#define _M4_SM_REF_SOL_H_

#include "sm_ref_sol_comm.h"
#include "m4_pmsm_appconfig.h"
#include "state_machine.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MCAT_SENSORLESS_CTRL 0 /* Sensorless control flag */
#define MCAT_ENC_CTRL 1 /* Position quadrature encoder control flag */
   
/*******************************************************************************
 * Variables
 ******************************************************************************/
extern bool_t g_bM4SwitchAppOnOff;
extern mcdef_pmsm_t g_sM4Drive;
extern sm_app_ctrl_t g_sM4Ctrl;
extern run_substate_t g_eM4StateRun;

extern volatile float g_fltM4voltageScale;
extern volatile float g_fltM4DCBvoltageScale;
extern volatile float g_fltM4currentScale;
extern volatile float g_fltM4speedScale;
extern volatile float g_fltM4speedAngularScale;

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _M4_SM_REF_SOL_H_ */


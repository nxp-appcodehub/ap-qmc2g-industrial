/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#ifndef _M2_SM_REF_SOL_H_
#define _M2_SM_REF_SOL_H_

#include "sm_ref_sol_comm.h"
#include "m2_pmsm_appconfig.h"
#include "state_machine.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MCAT_SENSORLESS_CTRL 0 /* Sensorless control flag */
#define MCAT_ENC_CTRL 1 /* Position quadrature encoder control flag */
   
/*******************************************************************************
 * Variables
 ******************************************************************************/
extern bool_t g_bM2SwitchAppOnOff;
extern mcdef_pmsm_t g_sM2Drive;
extern sm_app_ctrl_t g_sM2Ctrl;
extern run_substate_t g_eM2StateRun;

extern volatile float g_fltM2voltageScale;
extern volatile float g_fltM2DCBvoltageScale;
extern volatile float g_fltM2currentScale;
extern volatile float g_fltM2speedScale;
extern volatile float g_fltM2speedAngularScale;

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/


#ifdef __cplusplus
}
#endif

#endif /* _M2_SM_REF_SOL_H_ */


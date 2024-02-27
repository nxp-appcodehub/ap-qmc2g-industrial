/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "stdint.h"
#include "mlib_types.h"
#include "mc_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/



/* State machine control command flags */
#define SM_CTRL_NONE 0x0
#define SM_CTRL_FAULT 0x1
#define SM_CTRL_FAULT_CLEAR 0x2
#define SM_CTRL_INIT_DONE 0x4
#define SM_CTRL_STOP 0x8
#define SM_CTRL_START 0x10
#define SM_CTRL_STOP_ACK 0x20
#define SM_CTRL_RUN_ACK 0x40

typedef uint16_t sm_app_ctrl;
typedef uint32_t sm_app_fault;

/*! @brief Pointer to function */
typedef void (*pfcn_void_void)(void);

/*! @brief Application state identification enum */
typedef enum sm_app_state
{
    kSM_AppFault = 0,
    kSM_AppInit = 1,
    kSM_AppStop = 2,
    kSM_AppRun = 3,
} sm_app_state_t;

/*! @brief User state machine functions structure */
typedef struct sm_app_state_fcn
{
    pfcn_void_void Fault;
    pfcn_void_void Init;
    pfcn_void_void Stop;
    pfcn_void_void Run;
} sm_app_state_fcn_t;

/*! @brief User state-transition functions structure */
typedef struct sm_app_trans_fcn
{
    pfcn_void_void FaultStop;
    pfcn_void_void InitFault;
    pfcn_void_void InitStop;
    pfcn_void_void StopFault;
    pfcn_void_void StopRun;
    pfcn_void_void RunFault;
    pfcn_void_void RunStop;
} sm_app_trans_fcn_t;

/*! @brief State machine control structure */
typedef struct sm_app_ctrl
{
    sm_app_state_fcn_t const *psStateFast; /* State functions */
    sm_app_state_fcn_t const *psStateSlow; /* State functions slow*/
    sm_app_trans_fcn_t const *psTrans;     /* Transition functions */
    sm_app_ctrl uiCtrl;                    /* Control flags */
    sm_app_state_t eState;                 /* State */
} sm_app_ctrl_t;

/*! @brief Pointer to function with a pointer to state machine control structure */
typedef void (*pfcn_void_psm)(sm_app_ctrl_t *sAppCtrl);

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

/*! @brief State machine fast function prototype */
static inline void SM_StateMachineFast(sm_app_ctrl_t *sAppCtrl);
/*! @brief State machine slow function prototype */
static inline void SM_StateMachineSlow(sm_app_ctrl_t *sAppCtrl);

/*! @brief State machine fast function table */
extern  pfcn_void_psm g_SM_STATE_TABLE_FAST[4];

/*! @brief State machine slow function table */
extern  pfcn_void_psm g_SM_STATE_TABLE_SLOW[4];

/*!
 * @brief State Machine fast application state machine function
 *
 * @param sAppCtrl     Pointer to application state machine
 *
 * @return None
 */
ALWAYS_INLINE static inline void SM_StateMachineFast(sm_app_ctrl_t *sAppCtrl)
{
     g_SM_STATE_TABLE_FAST[sAppCtrl->eState](sAppCtrl);
}

/*!
 * @brief State Machine slow application state machine function
 *
 * @param sAppCtrl     Pointer to application state machine
 *
 * @return None
 */
ALWAYS_INLINE static inline void SM_StateMachineSlow(sm_app_ctrl_t *sAppCtrl)
{
    g_SM_STATE_TABLE_SLOW[sAppCtrl->eState](sAppCtrl);
}

#ifdef __cplusplus
}
#endif

#endif /* STATE_MACHINE_H */


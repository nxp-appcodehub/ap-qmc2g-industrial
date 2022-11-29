/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#ifndef _SM_REF_SOL_COMM_H_
#define _SM_REF_SOL_COMM_H_

#include "pmsm_control.h"
#include "api_motorcontrol_internal.h"
#include "board_service_tasks.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief device fault typedef */
typedef uint16_t mcdef_fault_t;

/*! @brief States of machine enumeration */
typedef enum _run_substate_t
{
    kRunState_Calib = 0,
    kRunState_Ready = 1,
    kRunState_Align = 2,
    kRunState_Startup = 3,
    kRunState_Spin = 4,
    kRunState_Freewheel = 5,
    kRunState_Measure = 6,
} run_substate_t; /* Run sub-states */

/*! @brief Control modes of the motor */
typedef enum _mcs_ctrl_mode_t
{
    kControlMode_Scalar = 0,
    kControlMode_VoltageFOC = 1,
    kControlMode_CurrentFOC = 2,
    kControlMode_SpeedFOC = 3,
    kControlMode_PositionFOC = 4,
	kControlMode_VoltageOpenloop = 5,
	kControlMode_CurrentOpenloop = 6
} mcs_ctrl_mode_t;

/*! @brief Device fault thresholds */
typedef struct _mcdef_fault_thresholds_t
{
    float_t fltUDcBusOver;     /* DC bus over voltage level */
    float_t fltUDcBusUnder;    /* DC bus under voltage level */
    float_t fltUDcBusTrip;     /* DC bus voltage level to start braking */
    float_t fltSpeedOver;      /* Over speed level */
    float_t fltSpeedMin;       /* Minimum speed level */
    float_t fltSpeedNom;       /* Nominal speed */
    float_t fltUqBemf;         /* Blocked rotor U level */
    uint16_t ui16BlockedPerNum; /* Number of period to set blocked rotor fault */
} mcdef_fault_thresholds_t;

/*! @brief PMSM FOC with BEMF observer in DQ */
typedef struct _mcdef_pmsm_t
{
    mcs_pmsm_foc_t sFocPMSM;                    /* Field Oriented Control structure */
    mcs_speed_t sSpeed;                         /* Speed control loop structure */
    mcs_position_t sPosition;                   /* Position control loop structure */
    mcs_pmsm_startup_t sStartUp;                /* Open loop start-up */
    mcs_alignment_t sAlignment;                 /* PMSM simple two-step Ud voltage alignment */
    mcs_mcat_ctrl_t sMCATctrl;                  /* Structure containing control variables directly updated from MCAT */
    mcs_pmsm_scalar_ctrl_t sScalarCtrl;         /* Scalar control structure */
    mcdef_fault_t sFaultIdCaptured;                /* Captured faults (must be cleared manually) */
    mcdef_fault_t sFaultIdPending;                 /* Fault pending structure */
    mcdef_fault_thresholds_t sFaultThresholds;     /* Fault thresholds */
    mcs_ctrl_mode_t eControl;                      /* MCAT control modes */
    mcs_ctrl_mode_t eControl_1;                      /* MCAT control modes of last step in slow loop */
    GDFLIB_FILTER_MA_T_FLT msM1BlockedRotorUqFilt; /* Blocked rotor detection filter */
    GDFLIB_FILTER_MA_T_FLT msM2BlockedRotorUqFilt; /* Blocked rotor detection filter */
    GDFLIB_FILTER_MA_T_FLT msM3BlockedRotorUqFilt; /* Blocked rotor detection filter */
    GDFLIB_FILTER_MA_T_FLT msM4BlockedRotorUqFilt; /* Blocked rotor detection filter */
    frac16_t f16AdcAuxSample;                      /* Auxiliary ADC sample  */
    uint16_t ui16CounterState;                     /* Main state counter */
    uint16_t ui16TimeFullSpeedFreeWheel;           /* Free-wheel time count number */
    uint16_t ui16TimeCalibration;                  /* Calibration time count number */
    uint16_t ui16TimeFaultRelease;                 /* Fault time count number */
    float_t fltBemfUqAvg;                          /* Blocked rotor filter output */
    uint16_t ui16BlockRotorCnt;                    /* Blocked rotor fault counter */
    uint16_t ui16FastCtrlLoopFreq;                 /* Pass fast loop frequency to FreeMASTER */
    uint16_t ui16SlowCtrlLoopFreq;                 /* Pass slow loop frequency to FreeMASTER */
    float_t fltSpeedEnc;                           /* Encoder speed */
    frac16_t f16PosElEnc;                          /* Encoder electrical position */
    bool_t bFaultClearMan;                         /* Manual fault clear detection */
    mc_motor_command_t *psMotorCmd;                /* Pointer to Motor commands from external sources */
    uint32_t ui32FaultIdCapturedExt;               /* Captured faults for external usage */
} mcdef_pmsm_t;

#define FAULT_I_DCBUS_OVER 	 0  /* OverCurrent fault flag */
#define FAULT_U_DCBUS_UNDER  1  /* Undervoltage fault flag */
#define FAULT_U_DCBUS_OVER   2  /* Overvoltage fault flag */
#define FAULT_LOAD_OVER      3  /* Overload fault flag */
#define FAULT_SPEED_OVER     4  /* Over speed fault flag */
#define FAULT_ROTOR_BLOCKED  5  /* Blocked rotor fault flag */
#define FAULT_GD3000_OC      6  /* GD3000 Over current */
#define FAULT_GD3000_OT		 7  /* GD3000 Over temperature */
#define FAULT_GD3000_VLS_UV  8  /* GD3000 VLS under voltage */
#define FAULT_GD3000_PHASE_ERR 9 /* GD3000 Phase error */
#define FAULT_GD3000_DESAT   10  /* GD3000 Desaturation detected */
#define FAULT_GD3000_IN_RESET   11  /* GD3000 is in reset state */

/* Sets the fault bit defined by faultid in the faults variable */
#define FAULT_SET(faults, faultid) (faults |= ((mcdef_fault_t)1 << faultid))

/* Clears the fault bit defined by faultid in the faults variable */
#define FAULT_CLEAR(faults, faultid) (faults &= ~((mcdef_fault_t)1 << faultid))

/* Check the fault bit defined by faultid in the faults variable, returns 1 or 0 */
#define FAULT_CHECK(faults, faultid) ((faults & ((mcdef_fault_t)1 << faultid)) >> faultid)

/* Clears all fault bits in the faults variable */
#define FAULT_CLEAR_ALL(faults) (faults = 0)

/* Check if a fault bit is set in the faults variable, 0 = no fault */
#define FAULT_ANY(faults) (faults > 0)

/*******************************************************************************
 * Variables
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _SM_REF_SOL_COMM_H_ */


/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef PMSM_CONTROL_H
#define PMSM_CONTROL_H

#include "char_pwrstg.h"
#include "gflib_FP.h"
#include "gmclib_FP.h"
#include "gdflib_FP.h"
#include "amclib_FP.h"
#include "mlib.h"
#include "mlib_FP.h"
#include "gflib.h"
#include "amclib.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef struct pi_controller_desat_coeff
{
	float_t fltPGain;
	float_t fltIGain;
	float_t fltDesatGain;
	float_t fltUpperLim;
	float_t fltLowerLim;
}pi_controller_desat_coeff_t;

typedef struct pi_controller_desat
{
	pi_controller_desat_coeff_t sCoeff;

	float_t fltIntegral; /* Integral part */
	float_t fltProp;     /* Proportional part */
	float_t fltDesat;    /* De-saturation part */
	float_t fltPresatOut; /* Pre-saturation output */
	float_t fltPresatOut_1;
	float_t fltIntegral_1;

	float_t fltErr; /* Input */
	float_t fltOut; /* Output */
	float_t fltOut_1;

}pi_controller_desat_t;

typedef struct trajectory_filter
{
	int32_t    i32In;
	int64_t    i64Out;

	int64_t    i64Out_1;
	int64_t    i64M;    /* Internal memory */
	int64_t    i64M_1;  /* Memory of last step */
    frac32_t   f32W;    /* Oscillation frequency in a 2nd order system */
}trajectory_filter_t;

/*====================== Type definitions ======================*/
/*
 *                                    b0 + b1*z^-1 + b2*z^-2
 *  For a second order series, Out = ------------------------ In
 *                                     1 + a1*z^-1 + a2*z^-2
 *
 *  Scale is a gain for this SOS
 *  w0, w1 and w2 are internal states of an SOS
 *
 * */

typedef struct
{
	float_t fltA1;
	float_t fltA2;
	float_t fltB0;
	float_t fltB1;
	float_t fltB2;
	float_t fltScale;
}IIR2_COEFF_T;

typedef struct
{
	IIR2_COEFF_T sCoeff;
	float_t fltW0;
	float_t fltW1;
	float_t fltW2;
	float_t fltIn;
	float_t fltOut;
}IIR2_T;

typedef struct
{
	IIR2_T sSOS_array[5];
	float_t fltOutScale; /* Final gain of the filter */
}FILTER_T;

/*! @brief mcs alignment structure */
typedef struct mcs_alignment_a1
{
    float_t fltUdReq;       /* Required D voltage at alignment */
    uint16_t ui16Time;      /* Alignment time duration */
    uint16_t ui16TimeHalf;  /* Alignment half time duration */
    frac16_t f16PosAlign;   /* Position for alignment */
} mcs_alignment_t;       /* PMSM simple two-step Ud voltage alignment */

/*! @brief mcs foc structure */
typedef struct mcs_pmsm_foc_a1
{
    GFLIB_CTRL_PI_P_AW_T_FLT sIdPiParams;       /* Id PI controller parameters */
    GFLIB_CTRL_PI_P_AW_T_FLT sIqPiParams;       /* Iq PI controller parameters */
    GDFLIB_FILTER_IIR1_T_FLT sUDcBusFilter;     /* Dc bus voltage filter */
    GMCLIB_3COOR_T_F16 sIABCFrac;               /* Measured 3-phase current (FRAC)*/
    GMCLIB_3COOR_T_FLT sIABC;                   /* Measured 3-phase current [A] */
    GMCLIB_2COOR_ALBE_T_FLT sIAlBe;             /* Alpha/Beta current */
    GMCLIB_2COOR_DQ_T_FLT sIDQ;                 /* DQ current */
    GMCLIB_2COOR_DQ_T_FLT sIDQReq;              /* DQ required current */
    float_t fltIqFiltReq;
    GMCLIB_2COOR_DQ_T_FLT sIDQError;            /* DQ current error */
    GMCLIB_3COOR_T_F16 sDutyABC;                /* Applied duty cycles ABC */
    GMCLIB_2COOR_ALBE_T_FLT sUAlBeReq;          /* Required Alpha/Beta voltage */
    GMCLIB_2COOR_ALBE_T_F16 sUAlBeCompFrac;     /* Compensated to DC bus Alpha/Beta voltage (FRAC) */
    GMCLIB_2COOR_ALBE_T_FLT sUAlBeDTComp;       /* Alpha/Beta stator voltage  */
    GMCLIB_2COOR_DQ_T_FLT sUDQReq;              /* Required DQ voltage */
    GMCLIB_2COOR_DQ_T_FLT sUDQEst;              /* BEMF observer input DQ voltages */
    GMCLIB_2COOR_SINCOS_T_FLT sAnglePosEl;      /* Electrical position sin/cos (at the moment of PWM current reading) */
    AMCLIB_BEMF_OBSRV_DQ_T_FLT sBemfObsrv;      /* BEMF observer in DQ */
    AMCLIB_TRACK_OBSRV_T_FLT sTo;               /* Tracking observer */
    GDFLIB_FILTER_IIR1_T_FLT sSpeedElEstFilt;   /* Estimated speed filter */
    acc32_t acc32BemfErr;                       /* BEMF observer output */
    float_t fltSpeedElEst;                      /* Rotor electrical speed estimated */
    uint16_t ui16SectorSVM;                     /* SVM sector */
    frac16_t f16PosEl;                          /* Electrical position */
    frac16_t f16PosElExt;                       /* Electrical position set from external function - sensor, open loop */
    frac16_t f16PosElEst;                       /* Rotor electrical position estimated*/
    float_t fltDutyCycleLimit;                  /* Maximum allowable duty cycle in frac */
    float_t fltUDcBus;                          /* DC bus voltage */
    frac16_t f16UDcBus;                         /* DC bus voltage */
    float_t fltUDcBusFilt;                      /* Filtered DC bus voltage */
    float_t fltPwrStgCharIRange;                /* Power Stage characteristic current range */
    float_t fltPwrStgCharLinCoeff;              /* Power Stage characteristic linear coefficient */
    bool_t bCurrentLoopOn;                      /* Flag enabling calculation of current control loop */
    bool_t bPosExtOn;                           /* Flag enabling use of electrical position passed from other functions */
    bool_t bOpenLoop;                           /* Position estimation loop is open */
    bool_t bIdPiStopInteg;                      /* Id PI controller manual stop integration */
    bool_t bIqPiStopInteg;                      /* Iq PI controller manual stop integration */
    bool_t bFlagDTComp;                         /* Enable/disable dead-time compensation flag */
} mcs_pmsm_foc_t;

/*! @brief mcs scalar structure */
typedef struct mcs_pmsm_scalar_ctrl_a1
{
    GFLIB_RAMP_T_FLT sFreqRampParams;           /* Parameters of frequency ramp */
    GMCLIB_2COOR_DQ_T_FLT sUDQReq;              /* Required voltage vector in d,q coordinates */
    GFLIB_INTEGRATOR_T_A32 sFreqIntegrator;     /* structure contains the integrator parameters (integrates the omega in
                                                   order to get the position */
    float_t fltFreqCmd;                         /* Required electrical frequency from master system */
    float_t fltFreqMax;                         /* Frequency maximum scale calculated from speed max */
    float_t fltFreqRamp;                        /* Required frequency limited by ramp - the ramp output */
    frac16_t f16PosElScalar;                    /* Electrical angle of the rotor */
    float_t fltVHzGain;                         /* VHz_factor constant gain for scalar control */
} mcs_pmsm_scalar_ctrl_t;

/*! @brief mcs scalar structure */
typedef struct mcs_speed_a1
{
    GDFLIB_FILTER_IIR1_T_FLT sSpeedFilter;      /* Speed filter */
    GFLIB_CTRL_PI_P_AW_T_FLT sSpeedPiParams;    /* Speed PI controller parameters */
    pi_controller_desat_t    sSpeedPiParamsDesat;
    GFLIB_RAMP_T_FLT sSpeedRampParams;          /* Speed ramp parameters */
    float_t fltSpeed;                           /* Speed */
    float_t fltSpeedFilt;                       /* Speed filtered */
    float_t fltSpeedError;                      /* Speed error */
    float_t fltSpeedRamp;                       /* Required speed (ramp output) */
    float_t fltSpeedRamp_1;
    float_t fltIqFwdGain;
    float_t fltSpeedCmd;                        /* Speed command (entered by user or master layer) */
    float_t fltIqReq;				/* Output of ASR */
    float_t fltIqFwd;               /* Feed forward current */
    float_t fltIqFwdFilt;
    float_t fltIqController;
    bool_t bSpeedPiStopInteg;                   /* Speed PI controller saturation flag */
    bool_t bIqPiLimFlag;			/* Saturation flag of Iq controller */

    GDFLIB_FILTER_IIR1_T_FLT sIqFwdFilter;
} mcs_speed_t;

/*! @brief mcs position structure */
typedef struct mcs_trajectory_a1
{
	int32_t i32Q16PosCmd;      /* Desired position */
	int32_t i32Q16PosRamp;     /* Ramping output of position command */
	int32_t i32Q16PosFilt;	   /* Filter output to achieve s-curve */
	GFLIB_RAMP_T_F32 sPosRamp; /* Position ramp, Q16.16 */

	trajectory_filter_t sTrajFilter; /* A second order filter to get s-curve position reference */
}mcs_trajectory_t;

/*! @brief mcs position structure */
typedef struct mcs_position_a1
{

	/* Position controller related */
    int32_t 	i32Q16PosRef;    	/* Position reference */
    int32_t 	i32Q16PosRef_1;    	/* Position reference of last step */
	int32_t 	i32Q16PosFdbk;   	/* Feedback position */
	int32_t 	i32Q16PosErr;    	/* Position error */
	acc32_t 	a32PosGain;      	/* Proportional control gain, Q17.15 */
	frac16_t	f16SpeedController; /* Position controller output, Q1.15 with base of speed scale */
	float_t		fltSpeedController; /* [rad/s], position controller output, electrical angular speed */
	frac16_t    f16SpeedRefLim;     /* Speed reference limit */
	float_t     fltSpeedRefLim;

	/* Speed feed forward related */
	int32_t		i32Q16FreqFwdNoGain;  /* [Hz], feed forward mechanical frequency without gain, Q16.16 */
	float_t		fltSpeedFwdNoGain;    /* Feed-forward speed reference to speed loop without gain, electrical angular speed */
	float_t		fltGainSpeedFwd;	  /* Gain to feed forward speed */
	float_t		fltSpeedFwd;		  /* Feed-forward speed reference to speed loop, electrical angular speed */
	int32_t     i32PosLoopFreq; 	  /* Position loop frequency in Hz, Q32.0 */
	float_t		fltFreqToAngularSpeedCoeff; /* a coefficient to convert mechanical frequency to electrical angular speed */
	float_t     fltFracToAngularSpeedCoeff; /* a coefficient to convert fractional frequency to electrical angular speed */
    float_t     fltRpmToAngularSpeedCoeff;  /* a coefficient to convert mechanical speed in RPM to electrical angular speed */

	/* Position control block output */
	float_t		fltSpeedRef;		  /* Speed command, electrical angular speed */

	/* Position command filter */
	mcs_trajectory_t sCurveRef; /* Position curve command */

	FILTER_T    sNotchFilter;

	int32_t     i32Q16PosCmd;      /* Desired position, used when trajectory is not needed in position loop */
	bool_t      bIsRandomPosition; /* trajectory is not needed when it's true */
} mcs_position_t;




/*! @brief mcs mcat control structure */
typedef struct mcs_mcat_ctrl_a1
{
    GMCLIB_2COOR_DQ_T_FLT sIDQReqMCAT;          /* required dq current entered from MCAT tool */
    GMCLIB_2COOR_DQ_T_FLT sUDQReqMCAT;          /* required dq voltage entered from MCAT tool */
    uint16_t ui16PospeSensor;                   /* position sensor type information */
} mcs_mcat_ctrl_t;

/*! @brief mcs pmsm startup structure */
typedef struct mcs_pmsm_startup_a1
{
    GFLIB_INTEGRATOR_T_A32 sSpeedIntegrator;    /* Speed integrator structure */
    GFLIB_RAMP_T_FLT sSpeedRampOpenLoopParams;  /* Parameters of startup speed ramp */
    float_t fltSpeedReq;                        /* Required speed */
    float_t fltSpeedMax;                        /* Maximum speed scale */
    frac16_t f16PosEst;                         /* Fractional electrical position */
    float_t fltSpeedRampOpenLoop;               /* Open loop startup speed ramp */
    frac16_t f16CoeffMerging;                   /* increment of merging weight for position merging */
    frac16_t f16RatioMerging;                   /* merging weight coefficient */
    frac16_t f16PosGen;                         /* generated open loop position from the speed ramp integration */
    frac16_t f16PosMerged;                      /* merged position */
    float_t fltSpeedCatchUp;                    /* merging speed threshold */
    float_t fltCurrentStartup;                  /* required Iq current during open loop start up */
    uint16_t ui16TimeStartUpFreeWheel; 		    /* Free-wheel duration if start-up aborted by user input (required zero speed) */
    bool_t bOpenLoop;                           /* Position estimation loop is open */
    bool_t bMergeFlag;                          /* Indicate whether enable merge when openloop speed is larger than catchup speed */
} mcs_pmsm_startup_t; 



/*!
 * @name Motor control PMSM  functions
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/*!
 * @brief PMSM field oriented current control.

  This function is used to compute PMSM field oriented current control.

 * @param psFocPMSM     The pointer of the PMSM FOC structure
 *
 * @return None
 */

extern void MCS_PMSMFocCtrl(mcs_pmsm_foc_t *psFocPMSM);

/*!
 * @brief PMSM field oriented speed control.
 *
 * This function is used to compute PMSM field oriented speed control.
 *
 * @param psSpeed       The pointer of the PMSM speed structure
 *
 * @return None
 */

extern void MCS_PMSMFocCtrlSpeed(mcs_speed_t *psSpeed);
   
/*!
 * @brief PMSM field oriented position control.
 *
   This function is used to compute PMSM field oriented position control.

 * @param psPosition       The pointer of the PMSM position structure
 *
 * @return None
 */
extern void MCS_PMSMFocCtrlPosition(mcs_position_t *psPosition);

/*!
 * @brief PMSM 2-step rotor alignment - 120deg in first step and 0deg in second.
 *
 * This function is used for alignment rotor in two steps - 120deg in first step and 0deg in second
 *
 * @param psAlignment   The pointer of the motor control alignment structure
 *
 * @return None
 */

extern void MCS_PMSMAlignment(mcs_alignment_t *psAlignment);

/*!
 * @brief PMSM Open Loop Start-up
 *
 * This function is used to PMSM Open Loop Start-up
 *
 * @param psStartUp     The pointer of the PMSM open loop start up parameters structure
 *
 * @return None
 */

extern void MCS_PMSMOpenLoopStartUp(mcs_pmsm_startup_t *psStartUp);

/*!
 * @brief PMSM scalar control, voltage is set based on required speed
 *
 * This function is used for alignment rotor in two steps - 120deg in first step and 0deg in second
 *
 * @param psScalarPMSM   The pointer of the PMSM scalar control structure
 *
 * @return None
 */

extern void MCS_PMSMScalarCtrl(mcs_pmsm_scalar_ctrl_t *psScalarPMSM);

/*!
 * @brief An 2nd order filter is used to realize trajectory. This function initializes the filter.

 * @param this       The pointer of the filter
 *
 * @return None
 */
extern void trajectoryFilterInit(trajectory_filter_t *this);

/*!
 * @brief PMSM field oriented position control.
 *
   This function is used to compute PMSM field oriented position control.

 * @param psPosition       The pointer of the PMSM position structure
 *
 * @return None
 */
extern void MCS_PMSMFocCtrlPosition(mcs_position_t *psPosition);

/*!
 * @brief    An 8th order Band Stop Filter, which is realized by 4 Second Order Series(SOS).
            This function updates the filter.

 * @param  ptr   The pointer of the filter structure
 * @param  fltIn  Input data
 * @return Filter result
 */
extern float_t BSF_update(FILTER_T *ptr, float_t f16In);

/*!
 * @brief    An 8th order Band Stop Filter, which is realized by 4 Second Order Series(SOS).
            This function initializes the internal states of the filter.

 * @param  fltIn  Input data
 * @return non
 */
extern void BSF_init(FILTER_T *ptr);

/*!
 * @brief    A PI controller with desaturation gain control. This function performs one step calculation of the controller.

 * @param  fltErr Input to the controller
 * @param  ptr    Pointer to the controller structure
 * @return Controller output
 */
extern float_t PIControllerDesatUpdate(float_t fltErr, pi_controller_desat_t *ptr);

/*!
 * @brief    A PI controller with desaturation gain control. This function initializes the internal states of the controller.

 * @param  ptr    Pointer to the controller structure
 * @return none
 */
extern void PIControllerDesatInit(pi_controller_desat_t *ptr);

#ifdef __cplusplus
}
#endif

#endif /* PMSM_CONTROL_H */


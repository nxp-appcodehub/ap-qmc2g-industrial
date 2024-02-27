/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "m4_sm_ref_sol.h"
#include "mid_sm_states.h"
#include "api_fault.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define M4_SVM_SECTOR_DEFAULT (2) /* default SVM sector */

#define FREQ_CUR_SCALE_M4 2000.0 /* [Hz], frequency scale for current command in bandwidth test */
#define FREQ_SCALE_M4     200.0  /* [Hz], frequency scale for speed command in bandwidth test */
#define FREQ_POS_SCALE_M4 200.0  /* [Hz], frequency scale for position command in bandwidth test */
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* State and transition functions for the main state machine */
RAM_FUNC_CRITICAL 
static void M4_StateFaultFast(void);
RAM_FUNC_CRITICAL
static void M4_StateInitFast(void);
RAM_FUNC_CRITICAL
static void M4_StateStopFast(void);
RAM_FUNC_CRITICAL 
static void M4_StateRunFast(void);

RAM_FUNC_CRITICAL
static void M4_StateFaultSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateInitSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateStopSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunSlow(void);

RAM_FUNC_CRITICAL  
static void M4_TransFaultStop(void);
RAM_FUNC_CRITICAL  
static void M4_TransInitFault(void);
RAM_FUNC_CRITICAL  
static void M4_TransInitStop(void);
RAM_FUNC_CRITICAL  
static void M4_TransStopFault(void);
RAM_FUNC_CRITICAL  
static void M4_TransStopRun(void);
RAM_FUNC_CRITICAL  
static void M4_TransRunFault(void);
RAM_FUNC_CRITICAL  
static void M4_TransRunStop(void);

/* State and transition functions for the sub-state machine in Run state of main state machine */
RAM_FUNC_CRITICAL   
static void M4_StateRunCalibFast(void);
RAM_FUNC_CRITICAL   
static void M4_StateRunMeasureFast(void);
RAM_FUNC_CRITICAL   
static void M4_StateRunReadyFast(void);
RAM_FUNC_CRITICAL   
static void M4_StateRunAlignFast(void);
RAM_FUNC_CRITICAL   
static void M4_StateRunStartupFast(void);
RAM_FUNC_CRITICAL   
static void M4_StateRunSpinFast(void);
RAM_FUNC_CRITICAL   
static void M4_StateRunFreewheelFast(void);

RAM_FUNC_CRITICAL
static void M4_StateRunCalibSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunMeasureSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunReadySlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunAlignSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunStartupSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunSpinSlow(void);
RAM_FUNC_CRITICAL
static void M4_StateRunFreewheelSlow(void);

RAM_FUNC_CRITICAL
static void M4_TransRunCalibReady(void);
RAM_FUNC_CRITICAL
static void M4_TransRunCalibMeasure(void);
RAM_FUNC_CRITICAL 
static void M4_TransRunMeasureReady(void);
RAM_FUNC_CRITICAL 
static void M4_TransRunReadyAlign(void);
RAM_FUNC_CRITICAL
static void M4_TransRunAlignStartup(void);
RAM_FUNC_CRITICAL 
static void M4_TransRunAlignReady(void);
RAM_FUNC_CRITICAL
static void M4_TransRunAlignSpin(void);
RAM_FUNC_CRITICAL
static void M4_TransRunStartupSpin(void);
RAM_FUNC_CRITICAL 
static void M4_TransRunStartupFreewheel(void);
RAM_FUNC_CRITICAL 
static void M4_TransRunSpinFreewheel(void);
RAM_FUNC_CRITICAL
static void M4_TransRunFreewheelReady(void);
RAM_FUNC_CRITICAL
static void M4_TransRunReadySpin(void);

RAM_FUNC_CRITICAL 
static void M4_ClearFOCVariables(void);

RAM_FUNC_CRITICAL
static void M4_FaultDetection(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Main control structure */
mcdef_pmsm_t g_sM4Drive;

/*! @brief Main application switch */
bool_t g_bM4SwitchAppOnOff;

/*! @brief M4 structure */
run_substate_t g_eM4StateRun;

/*! @brief FreeMASTER scales */
/*! DO NOT USE THEM in the code to avoid float library include */
volatile float g_fltM4voltageScale;
volatile float g_fltM4DCBvoltageScale;
volatile float g_fltM4currentScale;
volatile float g_fltM4speedScale;
volatile float g_fltM4speedAngularScale;

volatile static uint16_t ui16PWMOutputDebugFlagM4;

/*! @brief Application state machine table - fast */
static sm_app_state_fcn_t s_M4_STATE_FAST = {M4_StateFaultFast, M4_StateInitFast, M4_StateStopFast, M4_StateRunFast};

/*! @brief Application state machine table - slow */
static sm_app_state_fcn_t s_M4_STATE_SLOW = {M4_StateFaultSlow, M4_StateInitSlow, M4_StateStopSlow, M4_StateRunSlow};

/*! @brief Application sub-state function field - fast */
volatile static pfcn_void_void s_M4_STATE_RUN_TABLE_FAST[7] = {M4_StateRunCalibFast, M4_StateRunReadyFast,
                                                            M4_StateRunAlignFast, M4_StateRunStartupFast,
                                                            M4_StateRunSpinFast,  M4_StateRunFreewheelFast,
                                                            M4_StateRunMeasureFast};

/*! @brief Application sub-state function field - slow */
volatile static pfcn_void_void s_M4_STATE_RUN_TABLE_SLOW[7] = {M4_StateRunCalibSlow, M4_StateRunReadySlow,
                                                            M4_StateRunAlignSlow, M4_StateRunStartupSlow,
                                                            M4_StateRunSpinSlow,  M4_StateRunFreewheelSlow,
                                                            M4_StateRunMeasureSlow};

/*! @brief Application state-transition functions field  */
static  sm_app_trans_fcn_t s_TRANS = {M4_TransFaultStop, M4_TransInitFault, M4_TransInitStop, M4_TransStopFault,
                                           M4_TransStopRun,   M4_TransRunFault,  M4_TransRunStop};

/*! @brief  State machine structure declaration and initialization */
sm_app_ctrl_t g_sM4Ctrl = {
    /* g_sM4Ctrl.psState, User state functions  */
    &s_M4_STATE_FAST,

    /* g_sM4Ctrl.psState, User state functions  */
    &s_M4_STATE_SLOW,

    /* g_sM4Ctrl..psTrans, User state-transition functions */
    &s_TRANS,

    /* g_sM4Ctrl.uiCtrl, Default no control command */
    SM_CTRL_NONE,

    /* g_sM4Ctrl.eState, Default state after reset */
    kSM_AppInit};

/******** For speed loop bandwidth test ********/
volatile static uint16_t ui16SinSpeedCmdSwitchM4 = 0; /* A switch to turn on/off speed loop bandwidth test in speed FOC mode */
volatile static frac32_t f32AngleM4;
volatile static frac32_t f32FreqInM4;  /* Frequency of sinusoidal speed command */
volatile static float_t  fltSinM4;     /* Sin() of speed command frequency */
volatile static float_t  fltSpeedCmdAmplitudeM4 = 2*PI*25.0F; /* Amplitude of sinusoidal speed command */
volatile static float_t  fltSpeedCmdTestM4; /* Sinusoidal speed command */

/***********************************************/

/******** For current loop bandwidth test ********/
volatile static uint16_t ui16SinCurrentCmdSwitchM4 = 0;  /* A switch to turn on/off current loop bandwidth test in current FOC mode */
volatile static frac32_t f32AngleCurM4;
volatile static frac32_t f32FreqInCurM4;  /* Frequency of sinusoidal current command */
volatile static float_t  fltSinCurM4;     /* Sin() of current command frequency */
volatile static float_t  fltCurrentCmdAmplitudeM4 = 0.5F; /* Amplitude of sinusoidal current command */
volatile static float_t  fltCurrentCmdTestM4; /* Sinusoidal current command */

/***********************************************/

/******** For position loop bandwidth test ********/
volatile static uint16_t ui16SinPosCmdSwitchM4 = 0; /* A switch to turn on/off position loop bandwidth test in current FOC mode */
volatile static frac32_t f32AnglePosM4;
volatile static frac32_t f32FreqInPosM4;  /* Frequency of sinusoidal position command */
volatile static frac16_t f16SinPosM4;     /* Sin() of position command frequency */
volatile static int32_t  i32Q16PosCmdAmplitudeM4 = (int32_t)(0.5*65536); /* Amplitude of sinusoidal position command */
volatile static int32_t  i32Q16PosCmdTestM4; /* Sinusoidal position command */

/***********************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Fault state called in fast state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateFaultFast(void)
{
    /* read ADC results (ADC triggered by HW trigger from PDB) */
    /* get all adc samples - DC-bus voltage, current, bemf and aux sample */
	MCDRV_Curr3Ph2ShGet(&g_sM4AdcSensor);
	MCDRV_VoltDcBusGet(&g_sM4AdcSensor);
	MCDRV_AuxValGet(&g_sM4AdcSensor);

    /* get rotor position and speed from quadrature encoder sensor */
	if(g_sM4QdcSensor.bPosAbsoluteFlag == TRUE)
	{
		MCDRV_GetRotorCurrentPos(&g_sM4QdcSensor);
		MCDRV_GetRotorCurrentRev(&g_sM4QdcSensor);
		g_sM4Drive.f16PosElEnc = g_sM4QdcSensor.f16PosElec;
	}
	MCDRV_QdcToSpeedCalUpdate(&g_sM4QdcSensor); // Update speed from tracking observer in fast loop

    /* convert voltages from fractional measured values to float */
    g_sM4Drive.sFocPMSM.fltUDcBus =
        MLIB_ConvSc_FLTsf(g_sM4Drive.sFocPMSM.f16UDcBus, g_fltM4DCBvoltageScale);

    /* Sampled DC-Bus voltage filter */
    g_sM4Drive.sFocPMSM.fltUDcBusFilt =
        GDFLIB_FilterIIR1_FLT(g_sM4Drive.sFocPMSM.fltUDcBus, &g_sM4Drive.sFocPMSM.sUDcBusFilter);

//    /* Braking resistor control */
//    if(g_sM4Drive.sFocPMSM.fltUDcBusFilt > g_sM4Drive.sFaultThresholds.fltUDcBusTrip)
//        M4_BRAKE_SET();
//    else
//        M4_BRAKE_CLEAR();

    /* Disable user application switch */
    g_bM4SwitchAppOnOff = FALSE;

    /* PWM peripheral update */
    MCDRV_eFlexPwm3PhDutyUpdate(&g_sM4Pwm3ph);

    /* Detects faults */
    M4_FaultDetection();

    /* clear recorded fault state manually from FreeMASTER */
    if(g_sM4Drive.bFaultClearMan)
    {
        /* Clear fault state */
        g_sM4Drive.bFaultClearMan = FALSE;
        g_sM4Drive.sFaultIdCaptured = 0;
    }
}

/*!
 * @brief State initialization routine called in fast state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateInitFast(void)
{
	(void)ui16PWMOutputDebugFlagM4;
    (void)ui16SinSpeedCmdSwitchM4;
    (void)f32AngleM4;
    (void)f32FreqInM4;
    (void)fltSinM4;
    (void)fltSpeedCmdAmplitudeM4;
    (void)fltSpeedCmdTestM4;
    (void)ui16SinCurrentCmdSwitchM4;
    (void)f32AngleCurM4;
    (void)f32FreqInCurM4;
    (void)fltSinCurM4;
    (void)fltCurrentCmdAmplitudeM4;
    (void)fltCurrentCmdTestM4;
    (void)ui16SinPosCmdSwitchM4;
    (void)f32AnglePosM4;
    (void)f32FreqInPosM4;
    (void)f16SinPosM4;
    (void)i32Q16PosCmdAmplitudeM4;
    (void)i32Q16PosCmdTestM4;

	/* Type the code to do when in the INIT state */
    g_sM4Drive.sFocPMSM.sIdPiParams.fltPGain = MID_KP_GAIN;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltIGain = MID_KI_GAIN;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltInErrK_1 = 0.0F;
    g_sM4Drive.sFocPMSM.sIdPiParams.bLimFlag = FALSE;

    g_sM4Drive.sFocPMSM.sIqPiParams.fltPGain = MID_KP_GAIN;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltIGain = MID_KI_GAIN;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltInErrK_1 = 0.0F;
    g_sM4Drive.sFocPMSM.sIqPiParams.bLimFlag = FALSE;
  
    /* PMSM FOC params */
    g_sM4Drive.sFocPMSM.sIdPiParams.fltPGain = M4_D_KP_GAIN;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltIGain = M4_D_KI_GAIN;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltUpperLim = M4_U_MAX;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltLowerLim = -M4_U_MAX;

    g_sM4Drive.sFocPMSM.sIqPiParams.fltPGain = M4_Q_KP_GAIN;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltIGain = M4_Q_KI_GAIN;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltUpperLim = M4_U_MAX;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltLowerLim = -M4_U_MAX;

    g_sM4Drive.sFocPMSM.ui16SectorSVM = M4_SVM_SECTOR_DEFAULT;
    g_sM4Drive.sFocPMSM.fltDutyCycleLimit = M4_CLOOP_LIMIT;
    
    /* disable dead-time compensation */
    g_sM4Drive.sFocPMSM.bFlagDTComp = FALSE;

    g_sM4Drive.sFocPMSM.fltUDcBus = 0.0F;
    g_sM4Drive.sFocPMSM.fltUDcBusFilt = 0.0F;
    g_sM4Drive.sFocPMSM.sUDcBusFilter.sFltCoeff.fltB0 = M4_UDCB_IIR_B0;
    g_sM4Drive.sFocPMSM.sUDcBusFilter.sFltCoeff.fltB1 = M4_UDCB_IIR_B1;
    g_sM4Drive.sFocPMSM.sUDcBusFilter.sFltCoeff.fltA1 = M4_UDCB_IIR_A1;
    /* Filter init not to enter to fault */
    g_sM4Drive.sFocPMSM.sUDcBusFilter.fltFltBfrX[0] =
        (M4_U_DCB_UNDERVOLTAGE / 2.0F) + (M4_U_DCB_OVERVOLTAGE / 2.0F);
    g_sM4Drive.sFocPMSM.sUDcBusFilter.fltFltBfrY[0] =
        (M4_U_DCB_UNDERVOLTAGE / 2.0F) + (M4_U_DCB_OVERVOLTAGE / 2.0F);


    g_sM4Drive.sAlignment.fltUdReq = M4_ALIGN_VOLTAGE;
    g_sM4Drive.sAlignment.ui16Time = M4_ALIGN_DURATION;

    /* Position and speed observer */
    g_sM4Drive.sFocPMSM.sTo.fltPGain = M4_BEMF_DQ_TO_KP_GAIN;
    g_sM4Drive.sFocPMSM.sTo.fltIGain = M4_BEMF_DQ_TO_KI_GAIN;
    g_sM4Drive.sFocPMSM.sTo.fltThGain = M4_BEMF_DQ_TO_THETA_GAIN;

    g_sM4Drive.sFocPMSM.sBemfObsrv.fltIGain = M4_I_SCALE;
    g_sM4Drive.sFocPMSM.sBemfObsrv.fltUGain = M4_U_SCALE;
    g_sM4Drive.sFocPMSM.sBemfObsrv.fltEGain = M4_E_SCALE;
    g_sM4Drive.sFocPMSM.sBemfObsrv.fltWIGain = M4_WI_SCALE;
    g_sM4Drive.sFocPMSM.sBemfObsrv.sCtrl.fltPGain = M4_BEMF_DQ_KP_GAIN;
    g_sM4Drive.sFocPMSM.sBemfObsrv.sCtrl.fltIGain = M4_BEMF_DQ_KI_GAIN;

    g_sM4Drive.sFocPMSM.sSpeedElEstFilt.sFltCoeff.fltB0 = M4_TO_SPEED_IIR_B0;
    g_sM4Drive.sFocPMSM.sSpeedElEstFilt.sFltCoeff.fltB1 = M4_TO_SPEED_IIR_B1;
    g_sM4Drive.sFocPMSM.sSpeedElEstFilt.sFltCoeff.fltA1 = M4_TO_SPEED_IIR_A1;
    GDFLIB_FilterIIR1Init_FLT(&g_sM4Drive.sFocPMSM.sSpeedElEstFilt);
    
    /* Speed params */
    g_sM4Drive.sSpeed.sSpeedPiParams.fltPGain = M4_SPEED_PI_PROP_GAIN;
    g_sM4Drive.sSpeed.sSpeedPiParams.fltIGain = M4_SPEED_PI_INTEG_GAIN;
    g_sM4Drive.sSpeed.sSpeedPiParams.fltUpperLim = M4_SPEED_LOOP_HIGH_LIMIT;
    g_sM4Drive.sSpeed.sSpeedPiParams.fltLowerLim = M4_SPEED_LOOP_LOW_LIMIT;

    g_sM4Drive.sSpeed.sSpeedPiParamsDesat.sCoeff.fltPGain = M4_SPEED_PI_PROP_GAIN;
    g_sM4Drive.sSpeed.sSpeedPiParamsDesat.sCoeff.fltIGain = M4_SPEED_PI_INTEG_GAIN;
    g_sM4Drive.sSpeed.sSpeedPiParamsDesat.sCoeff.fltDesatGain = M4_SPEED_PI_DESAT_GAIN;
    g_sM4Drive.sSpeed.sSpeedPiParamsDesat.sCoeff.fltUpperLim = M4_SPEED_LOOP_HIGH_LIMIT;
    g_sM4Drive.sSpeed.sSpeedPiParamsDesat.sCoeff.fltLowerLim = M4_SPEED_LOOP_LOW_LIMIT;

    g_sM4Drive.sSpeed.sSpeedRampParams.fltRampUp = M4_SPEED_RAMP_UP;
    g_sM4Drive.sSpeed.sSpeedRampParams.fltRampDown = M4_SPEED_RAMP_DOWN;

    g_sM4Drive.sSpeed.sSpeedFilter.sFltCoeff.fltB0 = M4_SPEED_IIR_B0;
    g_sM4Drive.sSpeed.sSpeedFilter.sFltCoeff.fltB1 = M4_SPEED_IIR_B1;
    g_sM4Drive.sSpeed.sSpeedFilter.sFltCoeff.fltA1 = M4_SPEED_IIR_A1;
    GDFLIB_FilterIIR1Init_FLT(&g_sM4Drive.sSpeed.sSpeedFilter);

    g_sM4Drive.sSpeed.sIqFwdFilter.sFltCoeff.fltB0 = M4_CUR_FWD_IIR_B0;
    g_sM4Drive.sSpeed.sIqFwdFilter.sFltCoeff.fltB1 = M4_CUR_FWD_IIR_B1;
    g_sM4Drive.sSpeed.sIqFwdFilter.sFltCoeff.fltA1 = M4_CUR_FWD_IIR_A1;
    GDFLIB_FilterIIR1Init_FLT(&g_sM4Drive.sSpeed.sIqFwdFilter);

    g_sM4Drive.sSpeed.fltSpeedCmd = 0.0F;
    
    g_sM4Drive.sSpeed.fltIqFwdGain = M4_SPEED_LOOP_IQ_FWD_GAIN;

    /* Position params */
    g_sM4Drive.sPosition.sCurveRef.sPosRamp.f32State = 0;
    g_sM4Drive.sPosition.f16SpeedController = 0;
    g_sM4Drive.sPosition.fltSpeedFwd = 0;
    g_sM4Drive.sPosition.fltSpeedFwdNoGain = 0;
    g_sM4Drive.sPosition.fltSpeedRef = 0;
    g_sM4Drive.sPosition.sCurveRef.i32Q16PosCmd = 0;
    g_sM4Drive.sPosition.sCurveRef.i32Q16PosRamp = 0;
    g_sM4Drive.sPosition.sCurveRef.i32Q16PosFilt = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.f32W = M4_QDC_TRAJECTORY_FILTER_FREQ_FRAC;
    g_sM4Drive.sPosition.f16SpeedRefLim = M4_QDC_POSITION_CTRL_LIMIT_FRAC;
    g_sM4Drive.sPosition.fltSpeedRefLim = M4_QDC_POSITION_CTRL_LIMIT;
    g_sM4Drive.sPosition.sCurveRef.sPosRamp.f32RampUp = M4_QDC_POSITION_RAMP_UP_FRAC;
    g_sM4Drive.sPosition.sCurveRef.sPosRamp.f32RampDown = M4_QDC_POSITION_RAMP_DOWN_FRAC;
    g_sM4Drive.sPosition.a32PosGain = M4_QDC_POSITION_CTRL_P_GAIN_FRAC;
    g_sM4Drive.sPosition.i32PosLoopFreq = M4_SLOW_LOOP_FREQ;
    g_sM4Drive.sPosition.fltFreqToAngularSpeedCoeff = (float_t)(2.0*PI*M4_MOTOR_PP);
    g_sM4Drive.sPosition.fltFracToAngularSpeedCoeff = M4_SPEED_FRAC_TO_ANGULAR_COEFF;
    g_sM4Drive.sPosition.fltRpmToAngularSpeedCoeff = M4_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF;
    g_sM4Drive.sPosition.fltGainSpeedFwd = M4_QDC_POSITION_CTRL_SPEED_FWD_GAIN;
	trajectoryFilterInit(&g_sM4Drive.sPosition.sCurveRef.sTrajFilter);

	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[0].sCoeff.fltA1 = SOS0_A1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[0].sCoeff.fltA2 = SOS0_A2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[0].sCoeff.fltB0 = SOS0_B0;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[0].sCoeff.fltB1 = SOS0_B1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[0].sCoeff.fltB2 = SOS0_B2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[0].sCoeff.fltScale = SOS0_SCALE;

	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[1].sCoeff.fltA1 = SOS1_A1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[1].sCoeff.fltA2 = SOS1_A2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[1].sCoeff.fltB0 = SOS1_B0;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[1].sCoeff.fltB1 = SOS1_B1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[1].sCoeff.fltB2 = SOS1_B2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[1].sCoeff.fltScale = SOS1_SCALE;

	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[2].sCoeff.fltA1 = SOS2_A1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[2].sCoeff.fltA2 = SOS2_A2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[2].sCoeff.fltB0 = SOS2_B0;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[2].sCoeff.fltB1 = SOS2_B1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[2].sCoeff.fltB2 = SOS2_B2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[2].sCoeff.fltScale = SOS2_SCALE;

	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[3].sCoeff.fltA1 = SOS3_A1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[3].sCoeff.fltA2 = SOS3_A2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[3].sCoeff.fltB0 = SOS3_B0;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[3].sCoeff.fltB1 = SOS3_B1;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[3].sCoeff.fltB2 = SOS3_B2;
	g_sM4Drive.sPosition.sNotchFilter.sSOS_array[3].sCoeff.fltScale = SOS3_SCALE;

	g_sM4Drive.sPosition.sNotchFilter.fltOutScale = OUTPUT_SCALE;
	BSF_init(&g_sM4Drive.sPosition.sNotchFilter);

    /* Scalar control params */
    g_sM4Drive.sScalarCtrl.fltVHzGain = M4_SCALAR_VHZ_FACTOR_GAIN;
    g_sM4Drive.sScalarCtrl.sFreqRampParams.fltRampUp = M4_SCALAR_RAMP_UP;
    g_sM4Drive.sScalarCtrl.sFreqRampParams.fltRampDown = M4_SCALAR_RAMP_DOWN;
    g_sM4Drive.sScalarCtrl.sFreqIntegrator.a32Gain = M4_SCALAR_INTEG_GAIN;
    g_sM4Drive.sScalarCtrl.fltFreqMax = M4_FREQ_MAX;
    
    /* Open loop start up */
    g_sM4Drive.sStartUp.sSpeedIntegrator.a32Gain = M4_SCALAR_INTEG_GAIN;
    g_sM4Drive.sStartUp.f16CoeffMerging = M4_MERG_COEFF;
    g_sM4Drive.sStartUp.fltSpeedCatchUp = M4_MERG_SPEED_TRH;
    g_sM4Drive.sStartUp.fltCurrentStartup = M4_OL_START_I;
    g_sM4Drive.sStartUp.sSpeedRampOpenLoopParams.fltRampUp = M4_OL_START_RAMP_INC;
    g_sM4Drive.sStartUp.sSpeedRampOpenLoopParams.fltRampDown = M4_OL_START_RAMP_INC;
    g_sM4Drive.sStartUp.fltSpeedMax = M4_RAD_MAX;
    g_sM4Drive.sStartUp.bOpenLoop = TRUE;

    /* MCAT cascade control variables */
    g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltD = 0.0F;
    g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltD = 0.0F;
    g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.ui16PospeSensor = MCAT_ENC_CTRL;

    /* Timing control and general variables */
    g_sM4Drive.ui16CounterState = 0U;
    g_sM4Drive.ui16TimeFullSpeedFreeWheel = M4_FREEWHEEL_DURATION;
    g_sM4Drive.ui16TimeCalibration = M4_CALIB_DURATION;
    g_sM4Drive.ui16TimeFaultRelease = M4_FAULT_DURATION;
    g_bM4SwitchAppOnOff = FALSE;
    /* Default MCAT control mode after reset */
    g_sM4Drive.eControl = kControlMode_SpeedFOC;

    /* fault set to init states */
    FAULT_CLEAR_ALL(g_sM4Drive.sFaultIdCaptured);
    FAULT_CLEAR_ALL(g_sM4Drive.sFaultIdPending);

    /* fault thresholds */
    g_sM4Drive.sFaultThresholds.fltUDcBusOver = M4_U_DCB_OVERVOLTAGE;
    g_sM4Drive.sFaultThresholds.fltUDcBusUnder = M4_U_DCB_UNDERVOLTAGE;
    g_sM4Drive.sFaultThresholds.fltUDcBusTrip = M4_U_DCB_TRIP;
    g_sM4Drive.sFaultThresholds.fltSpeedOver = M4_N_OVERSPEED_RAD;
    g_sM4Drive.sFaultThresholds.fltSpeedMin = M4_N_MIN_RAD;
    g_sM4Drive.sFaultThresholds.fltSpeedNom = M4_N_NOM_RAD;
    g_sM4Drive.sFaultThresholds.fltUqBemf = M4_E_BLOCK_TRH;
    g_sM4Drive.sFaultThresholds.ui16BlockedPerNum = M4_E_BLOCK_PER;

    /* fault blocked rotor filter */
    g_sM4Drive.msM4BlockedRotorUqFilt.fltLambda = M4_BLOCK_ROT_FAULT_SH;

    /* Defined scaling for FreeMASTER */
    g_fltM4voltageScale = M4_U_MAX;
    g_fltM4currentScale = M4_I_MAX;
    g_fltM4DCBvoltageScale = M4_U_DCB_MAX;
    g_fltM4speedScale = M4_N_MAX;
    g_fltM4speedAngularScale = M4_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF;

    /* Application timing */
    g_sM4Drive.ui16FastCtrlLoopFreq = (M4_PWM_FREQ/M4_FOC_FREQ_VS_PWM_FREQ);
    g_sM4Drive.ui16SlowCtrlLoopFreq = M4_SLOW_LOOP_FREQ;
    
    /* Power Stage characteristic data */
    g_sM4Drive.sFocPMSM.fltPwrStgCharIRange = DTCOMP_I_RANGE;
    g_sM4Drive.sFocPMSM.fltPwrStgCharLinCoeff = DTCOMP_LINCOEFF;

    /* Clear rest of variables  */
    M4_ClearFOCVariables();

    /* Init sensors/actuators pointers */
    /* For PWM driver */
    g_sM4Pwm3ph.psDutyABC         = &(g_sM4Drive.sFocPMSM.sDutyABC);
    /* For ADC driver */
    g_sM4AdcSensor.pf16UDcBus     = &(g_sM4Drive.sFocPMSM.f16UDcBus);
    g_sM4AdcSensor.psIABC         = &(g_sM4Drive.sFocPMSM.sIABCFrac);
    g_sM4AdcSensor.pui16SVMSector = &(g_sM4Drive.sFocPMSM.ui16SectorSVM);
    g_sM4AdcSensor.pui16AuxChan   = &(g_sM4Drive.f16AdcAuxSample);
    /* For QDC driver */
    g_sM4QdcSensor.sSpeed.sQDCSpeedFilter.sFltCoeff.f32B0 = M4_QDC_SPEED_FILTER_IIR_B0_FRAC;
    g_sM4QdcSensor.sSpeed.sQDCSpeedFilter.sFltCoeff.f32B1 = M4_QDC_SPEED_FILTER_IIR_B1_FRAC;
    g_sM4QdcSensor.sSpeed.sQDCSpeedFilter.sFltCoeff.f32A1 = M4_QDC_SPEED_FILTER_IIR_A1_FRAC;
    MCDRV_QdcSpeedCalInit(&g_sM4QdcSensor);
    MCDRV_QdcToSpeedCalInit(&g_sM4QdcSensor);

    /* INIT_DONE command */
    g_sM4Ctrl.uiCtrl |= SM_CTRL_INIT_DONE;
}

/*!
 * @brief Stop state routine called in fast state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateStopFast(void)
{
    uint8_t ui8MotorsFaultStatus = 0;

	/* get all adc samples - DC-bus voltage, current, bemf and aux sample */
	MCDRV_VoltDcBusGet(&g_sM4AdcSensor);
	MCDRV_AuxValGet(&g_sM4AdcSensor);
	MCDRV_Curr3Ph2ShGet(&g_sM4AdcSensor);

    /* get rotor position and speed from quadrature encoder sensor */
	if(g_sM4QdcSensor.bPosAbsoluteFlag == TRUE)
	{
		MCDRV_GetRotorCurrentPos(&g_sM4QdcSensor);
		MCDRV_GetRotorCurrentRev(&g_sM4QdcSensor);
		g_sM4Drive.f16PosElEnc = g_sM4QdcSensor.f16PosElec;
	}
	MCDRV_QdcToSpeedCalUpdate(&g_sM4QdcSensor); // Update speed from tracking observer in fast loop

    /* convert voltages from fractional measured values to float */
    g_sM4Drive.sFocPMSM.fltUDcBus =
        MLIB_ConvSc_FLTsf(g_sM4Drive.sFocPMSM.f16UDcBus, g_fltM4DCBvoltageScale);

    /* Sampled DC-Bus voltage filter */
    g_sM4Drive.sFocPMSM.fltUDcBusFilt =
        GDFLIB_FilterIIR1_FLT(g_sM4Drive.sFocPMSM.fltUDcBus, &g_sM4Drive.sFocPMSM.sUDcBusFilter);

    /* If the user switches on */
    if (g_bM4SwitchAppOnOff != 0)
    {
    	if(FAULT_GetImmediateStopConfiguration_fromISR(kMC_Motor1,kMC_Motor4) == true)
    	{
    		if(FAULT_GetMotorFault_fromISR(kMC_Motor1) != (kMC_NoFaultMC|kMC_NoFaultBS))
    		{
    			ui8MotorsFaultStatus |= 1<<kMC_Motor1;
    		}
    	}
    	if(FAULT_GetImmediateStopConfiguration_fromISR(kMC_Motor2,kMC_Motor4) == true)
    	{
    		if(FAULT_GetMotorFault_fromISR(kMC_Motor2) != (kMC_NoFaultMC|kMC_NoFaultBS))
    		{
    			ui8MotorsFaultStatus |= 1<<kMC_Motor2;
    		}
    	}
    	if(FAULT_GetImmediateStopConfiguration_fromISR(kMC_Motor3,kMC_Motor4) == true)
    	{
    		if(FAULT_GetMotorFault_fromISR(kMC_Motor3) != (kMC_NoFaultMC|kMC_NoFaultBS))
    		{
    			ui8MotorsFaultStatus |= 1<<kMC_Motor3;
    		}
    	}

    	if(ui8MotorsFaultStatus == 0)
    	{
            /* Set the switch on */
            g_bM4SwitchAppOnOff = TRUE;

            /* Start command */
            g_sM4Ctrl.uiCtrl |= SM_CTRL_START;
    	}
    	else
    	{
    		g_bM4SwitchAppOnOff = FALSE;
    	}
    }

    /* Braking resistor control */
//    if(g_sM4Drive.sFocPMSM.fltUDcBusFilt > g_sM4Drive.sFaultThresholds.fltUDcBusTrip)
//        M4_BRAKE_SET();
//    else
//        M4_BRAKE_CLEAR();

    M4_FaultDetection();

    /* If a fault occurred */
    if (g_sM4Drive.sFaultIdPending)
    {
        /* Switches to the FAULT state */
        g_sM4Ctrl.uiCtrl |= SM_CTRL_FAULT;
    }

    /* clear recorded fault state manually from FreeMASTER */
    if(g_sM4Drive.bFaultClearMan)
    {
        /* Clear fault state */
        g_sM4Drive.bFaultClearMan = FALSE;
        g_sM4Drive.sFaultIdCaptured = 0;
    }

}

/*!
 * @brief Run state routine called in fast state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunFast(void)
{
    /* get all adc samples - DC-bus voltage, current, bemf and aux sample */
	MCDRV_Curr3Ph2ShGet(&g_sM4AdcSensor);
	MCDRV_VoltDcBusGet(&g_sM4AdcSensor);
	MCDRV_AuxValGet(&g_sM4AdcSensor);
    
    /* get position and speed from quadrature encoder sensor */
	if(g_sM4QdcSensor.bPosAbsoluteFlag == TRUE)
	{
		MCDRV_GetRotorCurrentPos(&g_sM4QdcSensor);
		MCDRV_GetRotorCurrentRev(&g_sM4QdcSensor);
		g_sM4Drive.f16PosElEnc = g_sM4QdcSensor.f16PosElec;
	}
	MCDRV_QdcToSpeedCalUpdate(&g_sM4QdcSensor); // Update speed from tracking observer in fast loop
     

    /* If the user switches off */
    if (!g_bM4SwitchAppOnOff)
    {
        /* Stop command */
        g_sM4Ctrl.uiCtrl |= SM_CTRL_STOP;
        
        g_sM4Drive.sPosition.sCurveRef.i32Q16PosCmd = 0;
        g_sM4Drive.sPosition.i32Q16PosRef = 0;
        g_sM4Drive.sPosition.i32Q16PosRef_1 = 0;
        g_sM4Drive.sPosition.fltSpeedFwd = 0;
        g_sM4Drive.sPosition.sCurveRef.i32Q16PosFilt = 0;
        g_sM4Drive.sPosition.i32Q16PosFdbk = 0;
            
    }

    /* detect fault */
    M4_FaultDetection();

    /* If a fault occurred */
    if (g_sM4Drive.sFaultIdPending != 0)
    {
        /* Switches to the FAULT state */
        g_sM4Ctrl.uiCtrl |= SM_CTRL_FAULT;
    }

    /* convert phase currents from fractional measured values to float */
    g_sM4Drive.sFocPMSM.sIABC.fltA = MLIB_ConvSc_FLTsf(g_sM4Drive.sFocPMSM.sIABCFrac.f16A, g_fltM4currentScale);
    g_sM4Drive.sFocPMSM.sIABC.fltB = MLIB_ConvSc_FLTsf(g_sM4Drive.sFocPMSM.sIABCFrac.f16B, g_fltM4currentScale);
    g_sM4Drive.sFocPMSM.sIABC.fltC = MLIB_ConvSc_FLTsf(g_sM4Drive.sFocPMSM.sIABCFrac.f16C, g_fltM4currentScale);

    /* convert voltages from fractional measured values to float */
    g_sM4Drive.sFocPMSM.fltUDcBus =
        MLIB_ConvSc_FLTsf(g_sM4Drive.sFocPMSM.f16UDcBus, g_fltM4DCBvoltageScale);

    /* Sampled DC-Bus voltage filter */
    g_sM4Drive.sFocPMSM.fltUDcBusFilt =
        GDFLIB_FilterIIR1_FLT(g_sM4Drive.sFocPMSM.fltUDcBus, &g_sM4Drive.sFocPMSM.sUDcBusFilter);

//    /* Braking resistor control */
//    if(g_sM4Drive.sFocPMSM.fltUDcBusFilt > g_sM4Drive.sFaultThresholds.fltUDcBusTrip)
//        M4_BRAKE_SET();
//    else
//        M4_BRAKE_CLEAR();

    /* Run sub-state function */
    s_M4_STATE_RUN_TABLE_FAST[g_eM4StateRun]();

    /* PWM peripheral update */
    MCDRV_eFlexPwm3PhDutyUpdate(&g_sM4Pwm3ph);

    /* set current sensor for  sampling */
    MCDRV_Curr3Ph2ShChanAssign(&g_sM4AdcSensor);

    /* clear recorded fault state manually from FreeMASTER */
    if(g_sM4Drive.bFaultClearMan)
    {
        /* Clear fault state */
        g_sM4Drive.bFaultClearMan = FALSE;
        g_sM4Drive.sFaultIdCaptured = 0;
    }

#if FEATURE_MC_LOOP_BANDWIDTH_TEST_ENABLE
    /* Speed loop bandwidth test - For debug only */
    if(ui16SinSpeedCmdSwitchM4 == 1)
    {
    	f32AngleM4 += MLIB_Mul_F32(FRAC32(2.0*FREQ_SCALE_M4/M4_FAST_LOOP_FREQ), f32FreqInM4);
    	fltSinM4 = GFLIB_Sin_FLTa((acc32_t)MLIB_Conv_F16l(f32AngleM4));
    	fltSpeedCmdTestM4 = fltSinM4 * fltSpeedCmdAmplitudeM4;
    }

    /* Current loop bandwidth test - For debug only */
    if(ui16SinCurrentCmdSwitchM4 == 1)
    {
    	f32AngleCurM4 += MLIB_Mul_F32(FRAC32(2.0*FREQ_CUR_SCALE_M4/M4_FAST_LOOP_FREQ), f32FreqInCurM4);
    	fltSinCurM4 = GFLIB_Sin_FLTa((acc32_t)MLIB_Conv_F16l(f32AngleCurM4));
    	fltCurrentCmdTestM4 = fltSinCurM4 * fltCurrentCmdAmplitudeM4;
    }

    /* Position loop bandwidth test - For debug only */
    if(ui16SinPosCmdSwitchM4 == 1)
    {
      	f32AnglePosM4 += MLIB_Mul_F32(FRAC32(2.0*FREQ_POS_SCALE_M4/M3_FAST_LOOP_FREQ), f32FreqInPosM4);
       	f16SinPosM4 = GFLIB_Sin_F16(MLIB_Conv_F16l(f32AnglePosM4));
       	i32Q16PosCmdTestM4 = ((int64_t)f16SinPosM4 * i32Q16PosCmdAmplitudeM4)>>15; // Q1.15 * Q16.16 = Q17.31 => Q16.16
    }
#endif
}

/*!
 * @brief Fault state routine called in slow state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateFaultSlow(void)
{
	// Get speed from QDC M/T method in slow loop
	MCDRV_QdcSpeedCalUpdate(&g_sM4QdcSensor);

	/* after fault condition ends wait defined time to clear fault state */
    if (!FAULT_ANY(g_sM4Drive.sFaultIdPending))
    {
        if (--g_sM4Drive.ui16CounterState == 0)
        {
            /* Clear fault state */
            g_sM4Ctrl.uiCtrl |= SM_CTRL_FAULT_CLEAR;
        }
    }
    else
    {
        g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeFaultRelease;
    }
}

/*!
 * @brief Fault state routine called in slow state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateInitSlow(void)
{
}

/*!
 * @brief Stop state routine called in slow state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateStopSlow(void)
{
	// Get speed from QDC M/T method in slow loop
	MCDRV_QdcSpeedCalUpdate(&g_sM4QdcSensor);
	MCDRV_GetRotorDeltaRev(&g_sM4QdcSensor);
}

/*!
 * @brief Run state routine called in slow state machine
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunSlow(void)
{
	// Get speed from QDC M/T method in slow loop
	MCDRV_QdcSpeedCalUpdate(&g_sM4QdcSensor);

	/* Run sub-state function */
    s_M4_STATE_RUN_TABLE_SLOW[g_eM4StateRun]();

    g_sM4Drive.eControl_1 = g_sM4Drive.eControl;
}

/*!
 * @brief Transition from Fault to Stop state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransFaultStop(void)
{
    fault_source_t motorFaults = 0;

	/* Type the code to do when going from the FAULT to the INIT state */
//    FAULT_CLEAR_ALL(g_sM4Drive.sFaultIdCaptured);

    /* Clear all FOC variables, init filters, etc. */
    M4_ClearFOCVariables();

    motorFaults = kMC_NoFaultMC|kMC_Motor4;
    FAULT_RaiseFaultEvent_fromISR(motorFaults);
}

/*!
 * @brief Transition from Init to Fault state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransInitFault(void)
{
    /* type the code to do when going from the INIT to the FAULT state */
    /* disable PWM outputs */
	MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);
    g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeFaultRelease;

    g_sM4Drive.sSpeed.fltSpeedCmd = 0.0F;
}

/*!
 * @brief Transition from Init to Stop state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransInitStop(void)
{
    /* type the code to do when going from the INIT to the STOP state */
    /* disable PWM outputs */
	MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);

    /* Enable Open loop start up */
    g_sM4Drive.sStartUp.bOpenLoop = TRUE;
}

/*!
 * @brief Transition from Stop to Fault state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransStopFault(void)
{
	fault_source_t motorFaults = 0;

	/* type the code to do when going from the STOP to the FAULT state */
    /* load the fault release time to counter */
    g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeFaultRelease;

    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_I_DCBUS_OVER))
    {
    	motorFaults |=  kMC_OverCurrent;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_U_DCBUS_UNDER))
    {
    	motorFaults |=  kMC_UnderDcBusVoltage;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_U_DCBUS_OVER))
    {
    	motorFaults |=  kMC_OverDcBusVoltage;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_LOAD_OVER))
    {
    	motorFaults |=  kMC_OverLoad;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_SPEED_OVER))
    {
    	motorFaults |=  kMC_OverSpeed;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_ROTOR_BLOCKED))
    {
    	motorFaults |=  kMC_RotorBlocked;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_OC))
    {
    	motorFaults |=  kMC_GD3000_OverCurrent;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_OT))
    {
    	motorFaults |=  kMC_GD3000_OverTemperature;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_VLS_UV))
    {
    	motorFaults |=  kMC_GD3000_LowVLS;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_PHASE_ERR))
    {
    	motorFaults |=  kMC_GD3000_PhaseError;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_DESAT))
    {
    	motorFaults |=  kMC_GD3000_Desaturation;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_IN_RESET))
    {
    	motorFaults |=  kMC_GD3000_Reset;
    }
    motorFaults = (motorFaults)|kMC_Motor4;

    FAULT_RaiseFaultEvent_fromISR(motorFaults);
}

/*!
 * @brief Transition from Stop to Run state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransStopRun(void)
{
    /* type the code to do when going from the STOP to the RUN state */
    /* 50% duty cycle */
    g_sM4Drive.sFocPMSM.sDutyABC.f16A = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16B = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16C = FRAC16(0.5);

    /* PWM duty cycles calculation and update */
    MCDRV_eFlexPwm3PhDutyUpdate(&g_sM4Pwm3ph);

    /* Clear offset filters */
    MCDRV_Curr3Ph2ShCalibInit(&g_sM4AdcSensor);

    /* Enable PWM output */
//    MCDRV_eFlexPwm3PhOutEnable(&g_sM4Pwm3ph);

    /* pass calibration routine duration to state counter*/
    g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeCalibration;

    /* Calibration sub-state when transition to RUN */
    g_eM4StateRun = kRunState_Calib;

    /* Acknowledge that the system can proceed into the RUN state */
    g_sM4Ctrl.uiCtrl |= SM_CTRL_RUN_ACK;
}

/*!
 * @brief Transition from Run to Fault state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunFault(void)
{
	fault_source_t motorFaults = 0;

	/* type the code to do when going from the RUN to the FAULT state */
    /* disable PWM output */
	MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);
    g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeFaultRelease;

    /* clear over load flag */
    g_sM4Drive.sSpeed.bSpeedPiStopInteg = FALSE;

    g_sM4Drive.sSpeed.fltSpeedCmd = 0.0F;
    g_sM4Drive.sScalarCtrl.fltFreqCmd = 0.0F;
    g_sM4Drive.sScalarCtrl.sUDQReq.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltD = 0.0F;
    g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltD = 0.0F;

    /* Clear actual speed values */
    g_sM4Drive.sScalarCtrl.fltFreqRamp = 0.0F;
    g_sM4Drive.sSpeed.fltSpeed = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedFilt = 0.0F;

    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_I_DCBUS_OVER))
    {
    	motorFaults |=  kMC_OverCurrent;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_U_DCBUS_UNDER))
    {
    	motorFaults |=  kMC_UnderDcBusVoltage;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_U_DCBUS_OVER))
    {
    	motorFaults |=  kMC_OverDcBusVoltage;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_LOAD_OVER))
    {
    	motorFaults |=  kMC_OverLoad;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_SPEED_OVER))
    {
    	motorFaults |=  kMC_OverSpeed;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_ROTOR_BLOCKED))
    {
    	motorFaults |=  kMC_RotorBlocked;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_OC))
    {
    	motorFaults |=  kMC_GD3000_OverCurrent;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_OT))
    {
    	motorFaults |=  kMC_GD3000_OverTemperature;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_VLS_UV))
    {
    	motorFaults |=  kMC_GD3000_LowVLS;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_PHASE_ERR))
    {
    	motorFaults |=  kMC_GD3000_PhaseError;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_DESAT))
    {
    	motorFaults |=  kMC_GD3000_Desaturation;
    }
    if(g_sM4Drive.sFaultIdPending &  (1ul<<FAULT_GD3000_IN_RESET))
    {
    	motorFaults |=  kMC_GD3000_Reset;
    }
    motorFaults = (motorFaults)|kMC_Motor4;

    FAULT_RaiseFaultEvent_fromISR(motorFaults);
}

/*!
 * @brief Transition from Run to Stop state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunStop(void)
{
    /* type the code to do when going from the RUN to the STOP state */
    /* disable PWM outputs */
	MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);

    g_sM4Drive.sSpeed.fltSpeedCmd = 0.0F;
    g_sM4Drive.sScalarCtrl.fltFreqCmd = 0.0F;
    g_sM4Drive.sScalarCtrl.sUDQReq.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltD = 0.0F;
    g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ = 0.0F;
    g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltD = 0.0F;

    M4_ClearFOCVariables();

    /* Acknowledge that the system can proceed into the STOP state */
    g_sM4Ctrl.uiCtrl |= SM_CTRL_STOP_ACK;
}

/*!
 * @brief Calibration process called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunCalibFast(void)
{
    /* Type the code to do when in the RUN CALIB sub-state
       performing ADC offset calibration */

    /* call offset measurement */
	MCDRV_Curr3Ph2ShCalib(&g_sM4AdcSensor);

    /* change SVM sector in range <1;6> to measure all AD channel mapping combinations */
    if (++g_sM4Drive.sFocPMSM.ui16SectorSVM > 6)
        g_sM4Drive.sFocPMSM.ui16SectorSVM = 1;
}

/*!
 * @brief Motor identification process called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */

RAM_FUNC_CRITICAL static void M4_StateRunMeasureFast(void)
{
    /* Set zero position at all measurements */
    if((g_sMIDCtrl.eState == kMID_Ld) || (g_sMIDCtrl.eState == kMID_Lq) || (g_sMIDCtrl.eState == kMID_Start) || (g_sMIDCtrl.eState == kMID_Rs) || (g_sMIDCtrl.eState == kMID_PwrStgCharact))
    {
        /* Zero position is needed for RL measurement */
        g_sM4Drive.sFocPMSM.f16PosEl = FRAC16(0.0);

        g_sM4Drive.sFocPMSM.sAnglePosEl.fltSin = 0.0F;
        g_sM4Drive.sFocPMSM.sAnglePosEl.fltCos = 1.0F;
    }
    
    /* turn on dead-time compensation in case of Rs measurement */
    g_sM4Drive.sFocPMSM.bFlagDTComp = (g_sMIDCtrl.eState == kMID_Rs);
    
    /* Perform current transformations if voltage control will be done.
     * At other measurements it is done in a current loop calculation */
    if((g_sMIDCtrl.eState == kMID_Ld) || (g_sMIDCtrl.eState == kMID_Lq))
    {
        /* Current transformations */
        GMCLIB_Clark_FLT(&g_sM4Drive.sFocPMSM.sIABC, &g_sM4Drive.sFocPMSM.sIAlBe);
        GMCLIB_Park_FLT(&g_sM4Drive.sFocPMSM.sIAlBe, &g_sM4Drive.sFocPMSM.sAnglePosEl, &g_sM4Drive.sFocPMSM.sIDQ);
    }
    
    /* If electrical parameters are being measured, put external position to FOC */
    if(g_sMIDCtrl.eState == kMID_Mech)
    {
        g_sM4Drive.sFocPMSM.bPosExtOn = (g_sMID.sMIDMech.eState == kMID_MechStartUp);
        g_sM4Drive.sFocPMSM.bOpenLoop = g_sMID.sMIDMech.sStartup.bOpenLoop;
    }
    else
        g_sM4Drive.sFocPMSM.bPosExtOn = TRUE;

    /* Motor parameters measurement state machine */
    MID_SM_StateMachine(&g_sMIDCtrl);

    /* Perform Current control if MID_START or MID_PWR_STG_CHARACT or MID_RS or MID_PP or MID_KE state */
    if((g_sMIDCtrl.eState == kMID_Start) || (g_sMIDCtrl.eState == kMID_PwrStgCharact) || (g_sMIDCtrl.eState == kMID_Rs) || (g_sMIDCtrl.eState == kMID_Pp) || (g_sMIDCtrl.eState == kMID_Ke) || (g_sMIDCtrl.eState == kMID_Mech))
    {    
        /* enable current control loop */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = TRUE;
    }
    
    /* Perform Voltage control if MID_LD or MID_LQ or START state*/
    else
    {
        /* disable current control loop */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = FALSE;
    }
    
    /* FOC */
    MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);

    /* Force sector to 4 to ensure that currents Ia, Ib will be sensed and Ic calculated */
    g_sM4Drive.sFocPMSM.ui16SectorSVM = 4U;

    /* When Measurement done go to RUN READY sub-state and then to STOP state and reset uw16Enable measurement */
    if(g_sMIDCtrl.uiCtrl & MID_SM_CTRL_STOP_ACK)
    {
        M4_TransRunMeasureReady();
        g_bM4SwitchAppOnOff = FALSE;
        g_sMID.ui16EnableMeasurement = 0U;
    }
}

/*!
 * @brief Ready state called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunReadyFast(void)
{
    /* Type the code to do when in the RUN READY sub-state */
    /* Clear actual speed values */
    g_sM4Drive.sScalarCtrl.fltFreqRamp = 0.0F;
    g_sM4Drive.sSpeed.fltSpeed = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedFilt = 0.0F;
    g_sM4Drive.sFocPMSM.f16PosElEst = 0;
    g_sM4Drive.sFocPMSM.fltSpeedElEst = 0.0F;

#if FEATURE_MC_INVERTER_OUTPUT_DEBUG_ENABLE
    /**************** Inverter output debug *************/
    if(ui16PWMOutputDebugFlagM4 == 1)
    {
        /* PWM peripheral update */
        MCDRV_eFlexPwm3PhDutyUpdate(&g_sM4Pwm3ph);
        MCDRV_eFlexPwm3PhOutEnable(&g_sM4Pwm3ph);
    }
    else
    {
    	MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);
    }
    /****************************************************/
#endif
    /* MCAT control structure switch */
    switch (g_sM4Drive.eControl)
    {
    case kControlMode_Scalar:
        if (!(g_sM4Drive.sScalarCtrl.fltFreqCmd == 0.0F))
        {
            g_sM4Drive.sScalarCtrl.fltFreqRamp = 0.0F;
            g_sM4Drive.sScalarCtrl.sUDQReq.fltQ = 0.0F;

            /* Transition to the RUN ALIGN sub-state */
            M4_TransRunReadyAlign();


        }
        break;

    case kControlMode_VoltageFOC:
        if (!(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ == 0.0F ))
        {
            if(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ > 0.0F)
                g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.sStartUp.fltSpeedCatchUp * 2.0F;
            else
                g_sM4Drive.sSpeed.fltSpeedCmd = MLIB_Neg_FLT(g_sM4Drive.sStartUp.fltSpeedCatchUp * 2.0F);

            /* Transition to the RUN ALIGN sub-state */
            if((g_sM4QdcSensor.bPosAbsoluteFlag == TRUE)&&(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL))
            {
            	M4_TransRunReadySpin();
            }
            else
            {
            	M4_TransRunReadyAlign();
            }
        }
        break;

    case kControlMode_VoltageOpenloop:
        if ((!(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ == 0.0F ))&&(g_sM4Drive.sSpeed.fltSpeedCmd != 0))
        {
            if(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ > 0.0F)
                g_sM4Drive.sStartUp.fltSpeedReq = g_sM4Drive.sSpeed.fltSpeedCmd;
            else
                g_sM4Drive.sStartUp.fltSpeedReq = MLIB_Neg_FLT(g_sM4Drive.sSpeed.fltSpeedCmd);

            M4_TransRunReadyAlign();
        }
        break;

    case kControlMode_CurrentFOC:
        if (!(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ == 0.0F ))
        {
            if(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ > 0.0F)
                g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.sStartUp.fltSpeedCatchUp * 2.0F;
            else
                g_sM4Drive.sSpeed.fltSpeedCmd = MLIB_Neg_FLT(g_sM4Drive.sStartUp.fltSpeedCatchUp * 2.0F);

            /* Transition to the RUN ALIGN sub-state */
            if((g_sM4QdcSensor.bPosAbsoluteFlag == TRUE)&&(g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_ENC_CTRL))
            {
            	M4_TransRunReadySpin();
            }
            else
            {
            	M4_TransRunReadyAlign();
            }
        }
        break;

    case kControlMode_CurrentOpenloop:
        if ((!(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ == 0.0F ))&&(g_sM4Drive.sSpeed.fltSpeedCmd != 0))
        {
            if(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ > 0.0F)
                g_sM4Drive.sStartUp.fltSpeedReq = g_sM4Drive.sSpeed.fltSpeedCmd;
            else
                g_sM4Drive.sStartUp.fltSpeedReq = MLIB_Neg_FLT(g_sM4Drive.sSpeed.fltSpeedCmd);

            M4_TransRunReadyAlign();
        }
        break;

    case kControlMode_SpeedFOC:
               
    case kControlMode_PositionFOC:
      
    default: 
    	if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
    	{
            if(g_sM4QdcSensor.bPosAbsoluteFlag == TRUE)
            {
            	M4_TransRunReadySpin();
            }
            else
            {
            	M4_TransRunReadyAlign();
            }
    	}
    	else if((g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_SENSORLESS_CTRL) &&
                ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedCmd) > g_sM4Drive.sFaultThresholds.fltSpeedMin) &&
                (MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedCmd) <= g_sM4Drive.sFaultThresholds.fltSpeedNom)))
    	{
    		M4_TransRunReadyAlign();
    	}
        else
        {
            g_sM4Drive.sSpeed.fltSpeedCmd = 0.0F;
        }
    }
}

/*!
 * @brief Alignment process called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunAlignFast(void)
{
    /* type the code to do when in the RUN ALIGN sub-state */
    /* When alignment elapsed go to Startup */
    if (--g_sM4Drive.ui16CounterState == 0U)
    {
        if((g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_ENC_CTRL) &&
           (g_sM4Drive.eControl != kControlMode_Scalar)&&(g_sM4Drive.eControl != kControlMode_VoltageOpenloop)&&(g_sM4Drive.eControl != kControlMode_CurrentOpenloop))
        {
            /* Transition to the RUN kRunState_Spin sub-state */
            M4_TransRunAlignSpin();
        }
        else
        {    
            /* Transition to the RUN kRunState_Startup sub-state */
            M4_TransRunAlignStartup();
        }
    }

    /* If zero speed command go back to Ready */
    if((g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_SENSORLESS_CTRL) && (g_sM4Drive.sSpeed.fltSpeedCmd == 0.0F) && (g_sM4Drive.sScalarCtrl.fltFreqCmd == 0.0F))
        M4_TransRunAlignReady();

    /* clear actual speed values */
    g_sM4Drive.sScalarCtrl.fltFreqRamp = 0.0F;
    g_sM4Drive.sSpeed.fltSpeed = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedFilt = 0.0F;
    g_sM4Drive.sFocPMSM.f16PosElEst = 0;
    g_sM4Drive.sFocPMSM.fltSpeedElEst = 0.0F;

    if(g_sM4Drive.ui16CounterState < (M4_ALIGN_DURATION - M4_BOOTSTRAP_CHARGE_DURATION))
    {
		MCS_PMSMAlignment(&g_sM4Drive.sAlignment);
		g_sM4Drive.sFocPMSM.f16PosElExt = g_sM4Drive.sAlignment.f16PosAlign;
		MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
    }
}

/*!
 * @brief Start-up process called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunStartupFast(void)
{
	frac16_t f16PosDiff = 0; // Position difference between "Merged Position" and "Open-loop Position"
	float_t  fltCosPosDiff;

	/* If f16SpeedCmd = 0, go to Free-wheel state */
    if((g_sM4Drive.sSpeed.fltSpeedCmd==0) && (g_sM4Drive.eControl==kControlMode_SpeedFOC))
        M4_TransRunStartupFreewheel();

    /* Type the code to do when in the RUN STARTUP sub-state */
    /* pass actual estimation position to OL startup structure */
    g_sM4Drive.sStartUp.f16PosEst = g_sM4Drive.sFocPMSM.f16PosElEst;

    /*open loop startup */
    MCS_PMSMOpenLoopStartUp(&g_sM4Drive.sStartUp);

    /* Pass f16SpeedRampOpenloop to f16SpeedRamp*/
    g_sM4Drive.sSpeed.fltSpeedRamp = g_sM4Drive.sStartUp.fltSpeedRampOpenLoop;

    /* Position and speed for FOC */
    g_sM4Drive.sFocPMSM.f16PosElExt = g_sM4Drive.sStartUp.f16PosMerged;

    /* MCAT control structure switch */
    switch (g_sM4Drive.eControl)
    {
    case kControlMode_Scalar: 
        /* switch directly to SPIN state */
        M4_TransRunStartupSpin();
        break;

    case kControlMode_VoltageFOC:
        /* pass MCAT required values in run-time */
        g_sM4Drive.sFocPMSM.sUDQReq.fltD = g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltD;
        g_sM4Drive.sFocPMSM.sUDQReq.fltQ = g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ;
        /* FOC */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = FALSE;
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
        break;  

    case kControlMode_CurrentFOC:
        /* FOC */
        g_sM4Drive.sFocPMSM.sIDQReq.fltD = g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltD;
        g_sM4Drive.sFocPMSM.sIDQReq.fltQ = g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ;
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = TRUE;
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
        break;

    case kControlMode_VoltageOpenloop:
        /* pass MCAT required values in run-time */
        g_sM4Drive.sFocPMSM.sUDQReq.fltD = g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltD;
        g_sM4Drive.sFocPMSM.sUDQReq.fltQ = g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ;

        if ((!(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ == 0.0F ))&&(g_sM4Drive.sSpeed.fltSpeedCmd != 0))
        {
            if(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ > 0.0F)
                g_sM4Drive.sStartUp.fltSpeedReq = g_sM4Drive.sSpeed.fltSpeedCmd;
            else
                g_sM4Drive.sStartUp.fltSpeedReq = MLIB_Neg_FLT(g_sM4Drive.sSpeed.fltSpeedCmd);
        }
        else
        {
        	 M4_TransRunStartupFreewheel();
        }

        /* FOC */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = FALSE;
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
        break;

    case kControlMode_CurrentOpenloop:

        g_sM4Drive.sFocPMSM.sIDQReq.fltD = g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltD;
        g_sM4Drive.sFocPMSM.sIDQReq.fltQ = g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ;

        if ((!(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ == 0.0F ))&&(g_sM4Drive.sSpeed.fltSpeedCmd != 0))
        {
            if(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ > 0.0F)
                g_sM4Drive.sStartUp.fltSpeedReq = g_sM4Drive.sSpeed.fltSpeedCmd;
            else
                g_sM4Drive.sStartUp.fltSpeedReq = MLIB_Neg_FLT(g_sM4Drive.sSpeed.fltSpeedCmd);
        }
        else
        {
        	 M4_TransRunStartupFreewheel();
        }
        /* FOC */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = TRUE;
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
        break;


    case kControlMode_SpeedFOC:
      
    case kControlMode_PositionFOC:
        
    default:
        /* Current control loop */
        g_sM4Drive.sFocPMSM.sIDQReq.fltD = 0.0F;

        /* during the open loop start up the values of required Iq current are kept in pre-defined level*/
        if (g_sM4Drive.sSpeed.fltSpeedCmd > 0.0F)
            g_sM4Drive.sFocPMSM.sIDQReq.fltQ = g_sM4Drive.sStartUp.fltCurrentStartup;
        else
            g_sM4Drive.sFocPMSM.sIDQReq.fltQ = MLIB_Neg_FLT(g_sM4Drive.sStartUp.fltCurrentStartup);

        /* Update Iq reference during open-loop and closed-loop merging when sensorless method is used */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_SENSORLESS_CTRL)
        {
        	if(g_sM4Drive.sStartUp.f16RatioMerging > 0) // In merging process
        	{

        		fltCosPosDiff = GFLIB_Cos_FLTa((acc32_t)MLIB_Mul_F16(g_sM4Drive.sStartUp.f16RatioMerging, f16PosDiff));
        		g_sM4Drive.sFocPMSM.sIDQReq.fltQ = fltCosPosDiff * g_sM4Drive.sFocPMSM.sIDQReq.fltQ;
        	}
        	else
        	{
        		f16PosDiff = MLIB_Sub_F16(g_sM4Drive.sStartUp.f16PosEst, g_sM4Drive.sStartUp.f16PosGen);
        	}
        }
        

        /* Init Bemf observer if open-loop speed is under SpeedCatchUp/2 */
        if (MLIB_Abs_FLT(g_sM4Drive.sStartUp.fltSpeedRampOpenLoop) <
            (g_sM4Drive.sStartUp.fltSpeedCatchUp / 2.0F))
        {
            AMCLIB_PMSMBemfObsrvDQInit_A32fff(&g_sM4Drive.sFocPMSM.sBemfObsrv);
            AMCLIB_TrackObsrvInit_A32af(ACC32(0.0), &g_sM4Drive.sFocPMSM.sTo);
        }

        /* FOC */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = TRUE;
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
               
        /* select source of actual speed value */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
            /* pass encoder speed to actual speed value */
#ifdef USE_QDC_TO_SPEED
            g_sM4Drive.sSpeed.fltSpeed = g_sM4QdcSensor.sSpeedEstim.f16SpeedEstim;
#else
        g_sM4Drive.sSpeed.fltSpeed = MLIB_ConvSc_FLTsf(g_sM4QdcSensor.sSpeed.f16SpeedFilt, (float_t)(2*FLOAT_PI*M4_FREQ_MAX)); // Get angular electrical speed
#endif
        else
            /* pass estimated speed to actual speed value */
            g_sM4Drive.sSpeed.fltSpeed = g_sM4Drive.sFocPMSM.fltSpeedElEst;
        
        
        
    }

    /* switch to close loop  */
    if (!g_sM4Drive.sStartUp.bOpenLoop)
    {
        M4_TransRunStartupSpin();
    }
}

/*!
 * @brief Spin state called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunSpinFast(void)
{
    /* Type the code to do when in the RUN SPIN sub-state */
    /* MCAT control structure switch */
    switch (g_sM4Drive.eControl)
    {
    case kControlMode_Scalar:
        /* scalar control */
        MCS_PMSMScalarCtrl(&g_sM4Drive.sScalarCtrl);

        /* pass required voltages to Bemf Observer to work */
        g_sM4Drive.sFocPMSM.sUDQReq.fltQ = g_sM4Drive.sScalarCtrl.sUDQReq.fltQ;
        g_sM4Drive.sFocPMSM.sUDQReq.fltD = g_sM4Drive.sScalarCtrl.sUDQReq.fltD;
        g_sM4Drive.sFocPMSM.f16PosElExt = g_sM4Drive.sScalarCtrl.f16PosElScalar;
        
        /* call voltage FOC to calculate PWM duty cycles */
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);

        /* select source of actual speed value */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
#ifdef USE_QDC_TO_SPEED
        g_sM4Drive.sSpeed.fltSpeed = MLIB_ConvSc_FLTsf(g_sM4QdcSensor.sSpeedEstim.f16SpeedEstim, (float_t)(2*FLOAT_PI*M4_N_MAX*M4_MOTOR_PP/60.0)); // Get electrical angular speed
#else
        g_sM4Drive.sSpeed.fltSpeed = g_sM4QdcSensor.sSpeed.fltSpeed;
#endif
        else
        	 /* pass estimated speed to actual speed value */
        	 g_sM4Drive.sSpeed.fltSpeed = g_sM4Drive.sFocPMSM.fltSpeedElEst;

        /* Sub-state RUN FREEWHEEL */
        if(g_sM4Drive.sScalarCtrl.fltFreqCmd==0.0F)
               M4_TransRunSpinFreewheel();
        break;

    case kControlMode_VoltageFOC:
        /* FOC */
        g_sM4Drive.sFocPMSM.sUDQReq.fltQ = g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ;
        g_sM4Drive.sFocPMSM.sUDQReq.fltD = g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltD;
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = FALSE;
        
        /* Pass encoder position to FOC is enabled */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_ENC_CTRL)
        {
            g_sM4Drive.sFocPMSM.f16PosElExt = g_sM4Drive.f16PosElEnc;
            g_sM4Drive.sFocPMSM.bPosExtOn   = TRUE;
        }
        else
        {
            g_sM4Drive.sFocPMSM.bPosExtOn = FALSE;
        }
        
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);

        /* select source of actual speed value */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
        {
#ifdef USE_QDC_TO_SPEED
            g_sM4Drive.sSpeed.fltSpeed = MLIB_ConvSc_FLTsf(g_sM4QdcSensor.sSpeedEstim.f16SpeedEstim, (float_t)(2*FLOAT_PI*M4_N_MAX*M4_MOTOR_PP/60.0)); // Get electrical angular speed
#else
            g_sM4Drive.sSpeed.fltSpeed = g_sM4QdcSensor.sSpeed.fltSpeed;

#endif
        }
        else
        	 /* pass estimated speed to actual speed value */
        	 g_sM4Drive.sSpeed.fltSpeed = g_sM4Drive.sFocPMSM.fltSpeedElEst;

        /* Sub-state RUN FREEWHEEL */
        if(g_sM4Drive.sMCATctrl.sUDQReqMCAT.fltQ==0.0F)
            M4_TransRunSpinFreewheel();
        break;

    case kControlMode_CurrentFOC: 
        /* current FOC */
        g_sM4Drive.sFocPMSM.sIDQReq.fltQ = g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ;
        g_sM4Drive.sFocPMSM.sIDQReq.fltD = g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltD;

#if FEATURE_MC_LOOP_BANDWIDTH_TEST_ENABLE
        /* Current loop bandwidth test - For debug only */
        if(ui16SinCurrentCmdSwitchM4 == 1)
        {
        	g_sM4Drive.sFocPMSM.sIDQReq.fltD = fltCurrentCmdTestM4;
        }
#endif
        /* Pass encoder position to FOC is enabled */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_ENC_CTRL)
        {
            g_sM4Drive.sFocPMSM.f16PosElExt = g_sM4Drive.f16PosElEnc;
            g_sM4Drive.sFocPMSM.bPosExtOn   = TRUE;
        }
        else
        {
            g_sM4Drive.sFocPMSM.bPosExtOn = FALSE;
        }
        
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = TRUE;
        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);

        /* select source of actual speed value */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
        {
#ifdef USE_QDC_TO_SPEED
        g_sM4Drive.sSpeed.fltSpeed = MLIB_ConvSc_FLTsf(g_sM4QdcSensor.sSpeedEstim.f16SpeedEstim, (float_t)(2*FLOAT_PI*M4_N_MAX*M4_MOTOR_PP/60.0)); // Get electrical angular speed
#else
        g_sM4Drive.sSpeed.fltSpeed = g_sM4QdcSensor.sSpeed.fltSpeed;
#endif
        }
        else
			 /* pass estimated speed to actual speed value */
			 g_sM4Drive.sSpeed.fltSpeed = g_sM4Drive.sFocPMSM.fltSpeedElEst;

        /* Sub-state RUN FREEWHEEL */
        if(g_sM4Drive.sMCATctrl.sIDQReqMCAT.fltQ==0.0F)
            M4_TransRunSpinFreewheel();
        break;

    case kControlMode_SpeedFOC:
    case kControlMode_PositionFOC:
    default: 
        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedRamp) <
            g_sM4Drive.sFaultThresholds.fltSpeedMin) &&
            (g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_SENSORLESS_CTRL))
        {
            /* Sub-state RUN FREEWHEEL */
            M4_TransRunSpinFreewheel();
        }
  
        /* Pass encoder position to FOC is enabled */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_ENC_CTRL)
        {
            g_sM4Drive.sFocPMSM.f16PosElExt = g_sM4Drive.f16PosElEnc;
            g_sM4Drive.sFocPMSM.bPosExtOn   = TRUE;
        }
        else
        {
            g_sM4Drive.sFocPMSM.bPosExtOn = FALSE;
        }
        
        /* FOC */
        g_sM4Drive.sFocPMSM.bCurrentLoopOn = TRUE;

        MCS_PMSMFocCtrl(&g_sM4Drive.sFocPMSM);
        
        /* select source of actual speed value */
        if(g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_ENC_CTRL)
            /* pass encoder speed to actual speed value */
#ifdef USE_QDC_TO_SPEED
            g_sM4Drive.sSpeed.fltSpeed = MLIB_ConvSc_FLTsf(g_sM4QdcSensor.sSpeedEstim.f16SpeedEstim, (float_t)(2*FLOAT_PI*M4_N_MAX*M4_MOTOR_PP/60.0)); // Get electrical angular speed
#else
        g_sM4Drive.sSpeed.fltSpeed = g_sM4QdcSensor.sSpeed.fltSpeed;
#endif
        else
            /* pass estimated speed to actual speed value */
            g_sM4Drive.sSpeed.fltSpeed = g_sM4Drive.sFocPMSM.fltSpeedElEst;
        
        break;
    }
}

/*!
 * @brief Free-wheel process called in fast state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunFreewheelFast(void)
{
    /* Type the code to do when in the RUN FREEWHEEL sub-state */

    /* clear actual speed values */
    g_sM4Drive.sScalarCtrl.fltFreqRamp = 0.0F;
    g_sM4Drive.sSpeed.fltSpeed = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedFilt = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedRamp = 0.0F;
}

/*!
 * @brief Calibration process called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunCalibSlow(void)
{
    if (--g_sM4Drive.ui16CounterState == 0U)
    {
        /* write calibrated offset values */
    	MCDRV_Curr3Ph2ShCalibSet(&g_sM4AdcSensor);

        if(g_sMID.ui16EnableMeasurement != 0U)
            /* To switch to the RUN MEASURE sub-state */
            M4_TransRunCalibMeasure();
        else
            /* To switch to the RUN READY sub-state */
            M4_TransRunCalibReady();
    }
}

/*!
 * @brief Measure state called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunMeasureSlow(void)
{
}

/*!
 * @brief Ready state called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunReadySlow(void)
{
}

/*!
 * @brief Alignment process called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunAlignSlow(void)
{
}

/*!
 * @brief Start-up process called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunStartupSlow(void)
{
    if(g_sM4Drive.eControl == kControlMode_SpeedFOC)
    {
        /* actual speed filter */
        g_sM4Drive.sSpeed.fltSpeedFilt = GDFLIB_FilterIIR1_FLT(g_sM4Drive.sSpeed.fltSpeed, &g_sM4Drive.sSpeed.sSpeedFilter);

        /* pass required speed values lower than nominal speed */
        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedCmd) > g_sM4Drive.sFaultThresholds.fltSpeedNom))
        {
            /* set required speed to nominal speed if over speed command > speed nominal */
            if (g_sM4Drive.sSpeed.fltSpeedCmd > 0.0F)
                g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.sFaultThresholds.fltSpeedNom;
            else
                g_sM4Drive.sSpeed.fltSpeedCmd = MLIB_Neg_FLT(g_sM4Drive.sFaultThresholds.fltSpeedNom);
        }
    }
}

/*!
 * @brief Spin state called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunSpinSlow(void)
{
    if(g_sM4Drive.eControl == kControlMode_SpeedFOC)
    {
        /* Actual position */
        MCDRV_GetRotorDeltaRev(&g_sM4QdcSensor);
        g_sM4Drive.sPosition.i32Q16PosFdbk = g_sM4QdcSensor.i32Q16DeltaRev;

    	if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
    	{
        /* actual speed filter */
#ifdef USE_QDC_TO_SPEED
        g_sM4Drive.sSpeed.fltSpeedFilt = GDFLIB_FilterIIR1_FLT(g_sM4Drive.sSpeed.fltSpeed, &g_sM4Drive.sSpeed.sSpeedFilter);
#else
        g_sM4Drive.sSpeed.fltSpeedFilt = g_sM4Drive.sSpeed.fltSpeed;
#endif
    	}
    	else
    	{
    		g_sM4Drive.sSpeed.fltSpeedFilt = GDFLIB_FilterIIR1_FLT(g_sM4Drive.sSpeed.fltSpeed, &g_sM4Drive.sSpeed.sSpeedFilter);
    	}

        /* pass required speed values lower than nominal speed */
        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedCmd) > g_sM4Drive.sFaultThresholds.fltSpeedNom))
        {
            /* set required speed to nominal speed if over speed command > speed nominal */
            if (g_sM4Drive.sSpeed.fltSpeedCmd > 0.0F)
                g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.sFaultThresholds.fltSpeedNom;
            else
                g_sM4Drive.sSpeed.fltSpeedCmd = MLIB_Neg_FLT(g_sM4Drive.sFaultThresholds.fltSpeedNom);
        }

        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedRamp) < g_sM4Drive.sFaultThresholds.fltSpeedMin)&&
           (g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_SENSORLESS_CTRL))
            M4_TransRunSpinFreewheel();

#if FEATURE_MC_LOOP_BANDWIDTH_TEST_ENABLE
        /* Speed loop bandwidth test - For debug only */
        if(ui16SinSpeedCmdSwitchM4 == 1)
        {
        	g_sM4Drive.sSpeed.fltSpeedCmd = fltSpeedCmdTestM4;
        }
#endif
        /* call PMSM speed control */
        g_sM4Drive.sSpeed.bIqPiLimFlag = g_sM4Drive.sFocPMSM.sIqPiParams.bLimFlag;
        MCS_PMSMFocCtrlSpeed(&g_sM4Drive.sSpeed);
        g_sM4Drive.sFocPMSM.sIDQReq.fltQ = g_sM4Drive.sSpeed.fltIqReq;

//        g_sM4Drive.sFocPMSM.fltIqFiltReq = BSF_update(&g_sM4Drive.sPosition.sNotchFilter, g_sM4Drive.sFocPMSM.sIDQReq.fltQ);
    }

    if(g_sM4Drive.eControl == kControlMode_PositionFOC)
    {
    	if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_ENC_CTRL)
    	{
        /* actual speed filter */
#ifdef USE_QDC_TO_SPEED
        g_sM4Drive.sSpeed.fltSpeedFilt = GDFLIB_FilterIIR1_FLT(g_sM4Drive.sSpeed.fltSpeed, &g_sM4Drive.sSpeed.sSpeedFilter);
#else
        g_sM4Drive.sSpeed.fltSpeedFilt = g_sM4Drive.sSpeed.fltSpeed;
#endif
    	}
    	else
    	{
    		g_sM4Drive.sSpeed.fltSpeedFilt = GDFLIB_FilterIIR1_FLT(g_sM4Drive.sSpeed.fltSpeed, &g_sM4Drive.sSpeed.sSpeedFilter);
    	}
        
        /* pass required speed values lower than nominal speed */
        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedCmd) > g_sM4Drive.sFaultThresholds.fltSpeedNom))
        {
            /* set required speed to nominal speed if over speed command > speed nominal */
            if (g_sM4Drive.sSpeed.fltSpeedCmd > 0.0F)
                g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.sFaultThresholds.fltSpeedNom;
            else
                g_sM4Drive.sSpeed.fltSpeedCmd = MLIB_Neg_FLT(g_sM4Drive.sFaultThresholds.fltSpeedNom);
        }

        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedRamp) < g_sM4Drive.sFaultThresholds.fltSpeedMin)&&
           (g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_SENSORLESS_CTRL))
            M4_TransRunSpinFreewheel();
          
        /* Actual position */                 
        MCDRV_GetRotorDeltaRev(&g_sM4QdcSensor);
        g_sM4Drive.sPosition.i32Q16PosFdbk = g_sM4QdcSensor.i32Q16DeltaRev;

#if FEATURE_MC_LOOP_BANDWIDTH_TEST_ENABLE
        if(ui16SinPosCmdSwitchM4 == 1)
        {
        	g_sM4Drive.sPosition.i32Q16PosCmd = i32Q16PosCmdTestM4;
        }
#endif
        /* Set up speed feed-forward environment when switching from speed-control to position-control */
        if(g_sM4Drive.eControl_1 == kControlMode_SpeedFOC)
        {
        	if(g_sM4Drive.sPosition.bIsRandomPosition == FALSE)
        	{
        		g_sM4Drive.sPosition.i32Q16PosRef_1 = g_sM4Drive.sPosition.i32Q16PosCmd;
        	}
        	else
        	{
        		g_sM4Drive.sPosition.sCurveRef.sPosRamp.f32State = g_sM4Drive.sPosition.i32Q16PosCmd;
        		g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64M_1 = 0;
        		g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64M = 0;
        		g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64Out = ((int64_t)g_sM4Drive.sPosition.i32Q16PosCmd)<<32;
        		g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64Out_1 = ((int64_t)g_sM4Drive.sPosition.i32Q16PosCmd)<<32;
        		g_sM4Drive.sPosition.i32Q16PosRef_1 = g_sM4Drive.sPosition.i32Q16PosCmd;
        	}
        }

        /* Call PMSM position control */
        MCS_PMSMFocCtrlPosition(&g_sM4Drive.sPosition);
        
        /* Speed command is equal to position controller output */
        g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.sPosition.fltSpeedRef;
        g_sM4Drive.sSpeed.sSpeedRampParams.fltState = g_sM4Drive.sSpeed.fltSpeedCmd; // Bypass speed ramp

        /* Call PMSM speed control */
        g_sM4Drive.sSpeed.bIqPiLimFlag = g_sM4Drive.sFocPMSM.sIqPiParams.bLimFlag;
        MCS_PMSMFocCtrlSpeed(&g_sM4Drive.sSpeed);
        g_sM4Drive.sFocPMSM.sIDQReq.fltQ = g_sM4Drive.sSpeed.fltIqReq;

    }
 
}

/*!
 * @brief Free-wheel process called in slow state machine as Run sub state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_StateRunFreewheelSlow(void)
{
    /* wait until free-wheel time passes */
    if (--g_sM4Drive.ui16CounterState == 0)
    {
        /* switch to sub state READY */
        M4_TransRunFreewheelReady();
    }
}

/*!
 * @brief Transition from Calib to Ready state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunCalibReady(void)
{
    /* Type the code to do when going from the RUN CALIB to the RUN READY sub-state */

    /* set 50% PWM duty cycle */
    g_sM4Drive.sFocPMSM.sDutyABC.f16A = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16B = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16C = FRAC16(0.5);

    /* switch to sub state READY */
    g_eM4StateRun = kRunState_Ready;
}

/*!
 * @brief Transition from Calib to Measure state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunCalibMeasure(void)
{
    /* Type the code to do when going from the RUN CALIB to the RUN MEASURE sub-state */
    /* Initialise measurement */
  
    /* Set all measurement as inactive */
    g_sMID.sMIDAlignment.ui16Active     = FALSE;
    g_sMID.sMIDPwrStgChar.ui16Active    = FALSE;
    g_sMID.sMIDRs.ui16Active            = FALSE;
    g_sMID.sMIDLs.ui16Active            = FALSE;
    g_sMID.sMIDKe.ui16Active            = FALSE;
    g_sMID.sMIDPp.ui16Active            = FALSE;
    g_sMID.sMIDMech.ui16Active          = FALSE;
    
    /* I/O pointers */
    g_sMID.sIO.pf16PosElExt = &(g_sM4Drive.sFocPMSM.f16PosElExt);
    g_sMID.sIO.pfltId       = &(g_sM4Drive.sFocPMSM.sIDQ.fltD);
    g_sMID.sIO.pfltIq       = &(g_sM4Drive.sFocPMSM.sIDQ.fltQ);
    g_sMID.sIO.pfltIdReq    = &(g_sM4Drive.sFocPMSM.sIDQReq.fltD);
    g_sMID.sIO.pfltIqReq    = &(g_sM4Drive.sFocPMSM.sIDQReq.fltQ);
    g_sMID.sIO.pfltUdReq    = &(g_sM4Drive.sFocPMSM.sUDQReq.fltD);
    g_sMID.sIO.pfltUqReq    = &(g_sM4Drive.sFocPMSM.sUDQReq.fltQ);
    g_sMID.sIO.pfltUDCbus   = &(g_sM4Drive.sFocPMSM.fltUDcBusFilt);
    g_sMID.sIO.pfltEd       = &(g_sM4Drive.sFocPMSM.sBemfObsrv.sEObsrv.fltD);
    g_sMID.sIO.pfltEq       = &(g_sM4Drive.sFocPMSM.sBemfObsrv.sEObsrv.fltQ);
    g_sMID.sIO.pfltSpeedEst = &(g_sM4Drive.sFocPMSM.fltSpeedElEst);
    g_sMID.sIO.pf16PosElEst = &(g_sM4Drive.sFocPMSM.f16PosElEst);
    g_sMID.sIO.pf16PosElExt = &(g_sM4Drive.sFocPMSM.f16PosElExt);

    /* Ls measurement init */
    g_sMID.sMIDLs.fltUdMax   = MLIB_Mul_FLT(MID_K_MODULATION_RATIO, g_sM4Drive.sFocPMSM.fltUDcBusFilt);
    g_sMID.sMIDLs.fltFreqMax = (float_t)g_sM4Drive.ui16FastCtrlLoopFreq / 2U;

    /* Ke measurement init */
    g_sMID.sMIDKe.fltFreqMax = (float_t)g_sM4Drive.ui16FastCtrlLoopFreq / 2U;

    /* Pp measurement init */
    g_sMID.sMIDPp.fltFreqMax = (float_t)g_sM4Drive.ui16FastCtrlLoopFreq / 2U;
    
    /* PwrStg char init */
    g_sMID.sMIDPwrStgChar.ui16NumOfChPnts = MID_CHAR_CURRENT_POINT_NUMBERS;

    /* During the measurement motor is driven open-loop */
    g_sM4Drive.sFocPMSM.bOpenLoop = TRUE;

    /* Reset DONE & ACK of all MID states */
    g_sMIDCtrl.uiCtrl = 0;

    /* First state in MID state machine will be kMID_Start */
    g_sMIDCtrl.eState = kMID_Start;

    /* Sub-state RUN MEASURE */
    g_eM4StateRun = kRunState_Measure;
}

/*!
 * @brief Transition from Measure to Ready state
 *
 * @param void  No input parameter
 *
 * @return None
 */

RAM_FUNC_CRITICAL static void M4_TransRunMeasureReady(void)
{
    /* Type the code to do when going from the RUN CALIB to the RUN READY sub-state */
    /* Set off measurement */
    g_sMID.ui16EnableMeasurement = 0;

    /* set 50% PWM duty cycle */
    g_sM4Drive.sFocPMSM.sDutyABC.f16A = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16B = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16C = FRAC16(0.5);

    /* disable passing external electrical position to FOC */
    g_sM4Drive.sFocPMSM.bPosExtOn = FALSE;

    /* swith to sub state READY */
    g_eM4StateRun = kRunState_Ready;
}

/*!
 * @brief Transition from Ready to Align state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunReadyAlign(void)
{
    /* Type the code to do when going from the RUN kRunState_Ready to the RUN kRunState_Align sub-state */
    /* Alignment duration set-up */
    g_sM4Drive.ui16CounterState = g_sM4Drive.sAlignment.ui16Time;
    /* Counter of half alignment duration */
    g_sM4Drive.sAlignment.ui16TimeHalf = MLIB_ShR_F16(g_sM4Drive.sAlignment.ui16Time, 1);

    /* set required alignment voltage to Ud */
    g_sM4Drive.sFocPMSM.sUDQReq.fltD = g_sM4Drive.sAlignment.fltUdReq;
    g_sM4Drive.sFocPMSM.sUDQReq.fltQ = 0.0F;

    /* enable passing required position to FOC */
    g_sM4Drive.sFocPMSM.bPosExtOn = TRUE;

    /* disable current FOC */
    g_sM4Drive.sFocPMSM.bCurrentLoopOn = FALSE;
    
    /* enable Open loop mode in main control structure */
    g_sM4Drive.sFocPMSM.bOpenLoop = TRUE;

    if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_SENSORLESS_CTRL)
    {
    	g_sM4Drive.sSpeed.fltIqFwdGain = 0; // Disable Iq feedforward in sensorless control
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltPGain = M4_SPEED_PI_PROP_SENSORLESS_GAIN;
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltIGain = M4_SPEED_PI_INTEG_SENSORLESS_GAIN;
    }
    else
    {
    	g_sM4Drive.sSpeed.fltIqFwdGain = M4_SPEED_LOOP_IQ_FWD_GAIN;
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltPGain = M4_SPEED_PI_PROP_GAIN;
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltIGain = M4_SPEED_PI_INTEG_GAIN;
    }

    g_sM4Drive.sFocPMSM.sDutyABC.f16A = FRAC16(M4_BOOTSTRAP_DUTY);
    g_sM4Drive.sFocPMSM.sDutyABC.f16B = FRAC16(M4_BOOTSTRAP_DUTY);
    g_sM4Drive.sFocPMSM.sDutyABC.f16C = FRAC16(M4_BOOTSTRAP_DUTY);

    /* Enable PWM output */
    MCDRV_eFlexPwm3PhOutEnable(&g_sM4Pwm3ph);

    /* Sub-state RUN ALIGN */
    g_eM4StateRun = kRunState_Align;
}

/*!
 * @brief Transition from Align to Startup state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunAlignStartup(void)
{
    /* Type the code to do when going from the RUN kRunState_Align to the RUN kRunState_Startup sub-state */
    /* initialize encoder driver */
	MCDRV_GetRotorInitPos(&g_sM4QdcSensor, MLIB_Conv_F32s(g_sM4Drive.sAlignment.f16PosAlign));
	MCDRV_GetRotorInitRev(&g_sM4QdcSensor);
  
    /* clear application parameters */
    M4_ClearFOCVariables();

    /* pass required speed to open loop start-up structure */
    if (g_sM4Drive.sSpeed.fltSpeedCmd > 0.0F)
        g_sM4Drive.sStartUp.fltSpeedReq = g_sM4Drive.sStartUp.fltSpeedCatchUp;
    else
        g_sM4Drive.sStartUp.fltSpeedReq = MLIB_Neg_FLT(g_sM4Drive.sStartUp.fltSpeedCatchUp);

    /* enable position merge in openloop startup  */
    if((g_sM4Drive.eControl!= kControlMode_VoltageOpenloop)&&(g_sM4Drive.eControl!= kControlMode_CurrentOpenloop))
    {
    	g_sM4Drive.sStartUp.bMergeFlag = TRUE;
    }
    else
    {
    	g_sM4Drive.sStartUp.bMergeFlag = FALSE;
    }

    /* enable Open loop mode in main control structure */
    g_sM4Drive.sStartUp.bOpenLoop = TRUE;
    g_sM4Drive.sFocPMSM.bOpenLoop = TRUE;

    /* enable Open loop mode in FOC module */
    g_sM4Drive.sFocPMSM.bPosExtOn = TRUE;

    g_sM4Drive.sFocPMSM.ui16SectorSVM = M4_SVM_SECTOR_DEFAULT;
    GDFLIB_FilterIIR1Init_FLT(&g_sM4Drive.sSpeed.sSpeedFilter);

    /* Go to sub-state RUN STARTUP */
    g_eM4StateRun = kRunState_Startup;
}

/*!
 * @brief Transition from Align to Spin state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunAlignSpin(void)
{
    /* Type the code to do when going from the RUN STARTUP to the RUN SPIN sub-state */
    /* initialize encoder driver */
	MCDRV_GetRotorInitPos(&g_sM4QdcSensor, MLIB_Conv_F32s(g_sM4Drive.sAlignment.f16PosAlign));

    /* Ensure the feedback position is reset to zero whenever state machine goes to spin */
    MCDRV_GetRotorInitRev(&g_sM4QdcSensor);

    /* Ensure position command starts from zero */
    g_sM4Drive.sPosition.sCurveRef.sPosRamp.f32State = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64M = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64M_1 = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64Out_1 = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64Out = 0;
  
    g_sM4Drive.sFocPMSM.bPosExtOn = TRUE;                                        /* enable passing external electrical position from encoder to FOC */
    g_sM4Drive.sFocPMSM.bOpenLoop = FALSE;                                       /* disable parallel running openloop and estimator */
  
    g_sM4Drive.sFocPMSM.ui16SectorSVM    = M4_SVM_SECTOR_DEFAULT;
    g_sM4Drive.sFocPMSM.sIDQReq.fltD     = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQReq.fltQ     = 0.0F;
    
    g_sM4Drive.sSpeed.fltSpeedRamp = 0;
    g_sM4Drive.sSpeed.fltSpeedRamp_1 = 0;
    g_sM4Drive.sSpeed.sIqFwdFilter.fltFltBfrX[0] = 0;
    g_sM4Drive.sSpeed.sIqFwdFilter.fltFltBfrY[0] = 0;

    M4_ClearFOCVariables();

    /* To switch to the RUN SPIN sub-state */
    g_eM4StateRun = kRunState_Spin;
}

RAM_FUNC_CRITICAL static void M4_TransRunReadySpin(void)
{
    g_sM4Drive.sFocPMSM.bPosExtOn = TRUE;                                        /* enable passing external electrical position from encoder to FOC */
    g_sM4Drive.sFocPMSM.bOpenLoop = FALSE;                                       /* disable parallel running openloop and estimator */

    g_sM4Drive.sFocPMSM.ui16SectorSVM    = M4_SVM_SECTOR_DEFAULT;
    g_sM4Drive.sFocPMSM.sIDQReq.fltD     = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQReq.fltQ     = 0.0F;

    M4_ClearFOCVariables();

    /* Ensure the feedback position is reset whenever state machine goes to spin */
    MCDRV_GetRotorInitRev(&g_sM4QdcSensor);

    /* Ensure position command starts from zero */
    g_sM4Drive.sPosition.sCurveRef.sPosRamp.f32State = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64M = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64M_1 = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64Out_1 = 0;
    g_sM4Drive.sPosition.sCurveRef.sTrajFilter.i64Out = 0;

    g_sM4Drive.sSpeed.fltSpeedRamp = 0;
    g_sM4Drive.sSpeed.fltSpeedRamp_1 = 0;
    g_sM4Drive.sSpeed.sIqFwdFilter.fltFltBfrX[0] = 0;
    g_sM4Drive.sSpeed.sIqFwdFilter.fltFltBfrY[0] = 0;

    /* Enable PWM output */
    MCDRV_eFlexPwm3PhOutEnable(&g_sM4Pwm3ph);

    if(g_sM4Drive.sMCATctrl.ui16PospeSensor == MCAT_SENSORLESS_CTRL)
    {
    	g_sM4Drive.sSpeed.fltIqFwdGain = 0; // Disable Iq feedforward in sensorless control
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltPGain = M4_SPEED_PI_PROP_SENSORLESS_GAIN;
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltIGain = M4_SPEED_PI_INTEG_SENSORLESS_GAIN;
    }
    else
    {
    	g_sM4Drive.sSpeed.fltIqFwdGain = M4_SPEED_LOOP_IQ_FWD_GAIN;
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltPGain = M4_SPEED_PI_PROP_GAIN;
    	g_sM4Drive.sSpeed.sSpeedPiParams.fltIGain = M4_SPEED_PI_INTEG_GAIN;
    }

    /* To switch to the RUN SPIN sub-state */
    g_eM4StateRun = kRunState_Spin;
}

/*!
 * @brief Transition from Align to Ready state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunAlignReady(void)
{
    /* Type the code to do when going from the RUN kRunState_Align to the RUN kRunState_Ready sub-state */
    
    /* Clear FOC accumulators */
    M4_ClearFOCVariables();

    /* Go to sub-state RUN READY */
    g_eM4StateRun = kRunState_Ready;
}

/*!
 * @brief Transition from Startup to Spin state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunStartupSpin(void)
{
    /* Type the code to do when going from the RUN kRunState_Startup to the RUN kRunState_Spin sub-state */
    /* for FOC control switch open loop off in DQ observer */
    if(g_sM4Drive.eControl!=kControlMode_Scalar)
    {    
        g_sM4Drive.sFocPMSM.bPosExtOn = FALSE; /* disable passing external electrical position to FOC */
        g_sM4Drive.sFocPMSM.bOpenLoop = FALSE; /* disable parallel running open-loop and estimator */
    }

    g_sM4Drive.sSpeed.sSpeedPiParams.fltIAccK_1 = g_sM4Drive.sFocPMSM.sIDQReq.fltQ;
    g_sM4Drive.sSpeed.sSpeedRampParams.fltState = g_sM4Drive.sSpeed.fltSpeedFilt;
    g_sM4Drive.sSpeed.fltSpeedRamp = g_sM4Drive.sSpeed.fltSpeedFilt;
    g_sM4Drive.sSpeed.fltSpeedRamp_1 = g_sM4Drive.sSpeed.fltSpeedFilt;
    g_sM4Drive.sSpeed.sIqFwdFilter.fltFltBfrX[0] = 0;
    g_sM4Drive.sSpeed.sIqFwdFilter.fltFltBfrY[0] = 0;

    /* To switch to the RUN kRunState_Spin sub-state */
    g_eM4StateRun = kRunState_Spin;
}

/*!
 * @brief Transition from Startup to Free-wheel state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunStartupFreewheel(void)
{
	MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);

    /* Free-wheel duration set-up */
    g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeFullSpeedFreeWheel;

    /* enter FREEWHEEL sub-state */
    g_eM4StateRun = kRunState_Freewheel;
}

/*!
 * @brief Transition from Spin to Free-wheel state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunSpinFreewheel(void)
{
    /* Type the code to do when going from the RUN SPIN to the RUN FREEWHEEL sub-state */
    /* set 50% PWM duty cycle */
    g_sM4Drive.sFocPMSM.sDutyABC.f16A = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16B = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16C = FRAC16(0.5);

    g_sM4Drive.sFocPMSM.ui16SectorSVM = M4_SVM_SECTOR_DEFAULT;

    MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);

    /* Generates a time gap before the alignment to assure the rotor is not rotating */
    g_sM4Drive.ui16CounterState = g_sM4Drive.ui16TimeFullSpeedFreeWheel;

    g_sM4Drive.sFocPMSM.sIDQReq.fltD = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQReq.fltQ = 0.0F;

    g_sM4Drive.sFocPMSM.sUDQReq.fltD = 0.0F;
    g_sM4Drive.sFocPMSM.sUDQReq.fltQ = 0.0F;

    g_sM4Drive.sFocPMSM.sIAlBe.fltAlpha = 0.0F;
    g_sM4Drive.sFocPMSM.sIAlBe.fltBeta = 0.0F;
    g_sM4Drive.sFocPMSM.sUAlBeReq.fltAlpha = 0.0F;
    g_sM4Drive.sFocPMSM.sUAlBeReq.fltBeta = 0.0F;

    /* enter FREEWHEEL sub-state */
    g_eM4StateRun = kRunState_Freewheel;
}

/*!
 * @brief Transition from Free-wheel to Ready state
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_TransRunFreewheelReady(void)
{
    /* Type the code to do when going from the RUN kRunState_FreeWheel to the RUN kRunState_Ready sub-state */
    /* clear application parameters */
    M4_ClearFOCVariables();

    MCDRV_eFlexPwm3PhOutDisable(&g_sM4Pwm3ph);

    /* Sub-state RUN READY */
    g_eM4StateRun = kRunState_Ready;
}

/*!
 * @brief Clear FOc variables in global variable
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_ClearFOCVariables(void)
{
    g_sM4Drive.sAlignment.ui16TimeHalf = 0U;

    /* Clear FOC variables */
    g_sM4Drive.sFocPMSM.sIABC.fltA = 0.0F;
    g_sM4Drive.sFocPMSM.sIABC.fltB = 0.0F;
    g_sM4Drive.sFocPMSM.sIABC.fltC = 0.0F;
    g_sM4Drive.sFocPMSM.sIAlBe.fltAlpha = 0.0F;
    g_sM4Drive.sFocPMSM.sIAlBe.fltBeta = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQ.fltD = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQ.fltQ = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQReq.fltD = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQReq.fltQ = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQError.fltD = 0.0F;
    g_sM4Drive.sFocPMSM.sIDQError.fltQ = 0.0F;
    g_sM4Drive.sFocPMSM.sDutyABC.f16A = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16B = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sDutyABC.f16C = FRAC16(0.5);
    g_sM4Drive.sFocPMSM.sUAlBeReq.fltAlpha = 0.0F;
    g_sM4Drive.sFocPMSM.sUAlBeReq.fltBeta = 0.0F;
    g_sM4Drive.sFocPMSM.sUDQReq.fltD = 0.0F;
    g_sM4Drive.sFocPMSM.sUDQReq.fltQ = 0.0F;
    g_sM4Drive.sFocPMSM.sAnglePosEl.fltSin = 0.0F;
    g_sM4Drive.sFocPMSM.sAnglePosEl.fltCos = 0.0F;
    g_sM4Drive.sFocPMSM.sAnglePosEl.fltSin = 0.0F;
    g_sM4Drive.sFocPMSM.sAnglePosEl.fltCos = 0.0F;
    g_sM4Drive.sFocPMSM.sIdPiParams.bLimFlag = FALSE;
    g_sM4Drive.sFocPMSM.sIqPiParams.bLimFlag = FALSE;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltIAccK_1 = 0.0F;
    g_sM4Drive.sFocPMSM.sIdPiParams.fltIAccK_1 = 0.0F;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltIAccK_1 = 0.0F;
    g_sM4Drive.sFocPMSM.sIqPiParams.fltIAccK_1 = 0.0F;
    GDFLIB_FilterIIR1Init_FLT(&g_sM4Drive.sFocPMSM.sSpeedElEstFilt);
    g_sM4Drive.sFocPMSM.bIdPiStopInteg = FALSE;
    g_sM4Drive.sFocPMSM.bIqPiStopInteg = FALSE;

    /* Clear Speed control state variables */
    g_sM4Drive.sSpeed.sSpeedRampParams.fltState = 0.0F;
    g_sM4Drive.sSpeed.fltSpeed = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedFilt = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedError = 0.0F;
    g_sM4Drive.sSpeed.fltSpeedRamp = 0.0F;
    g_sM4Drive.sSpeed.sSpeedPiParams.fltIAccK_1 = 0.0F;
    g_sM4Drive.sSpeed.sSpeedPiParams.bLimFlag = FALSE;
    g_sM4Drive.sSpeed.sSpeedFilter.fltFltBfrX[0] = 0.0F;
    g_sM4Drive.sSpeed.sSpeedFilter.fltFltBfrY[0] = 0.0F;
    g_sM4Drive.sSpeed.bSpeedPiStopInteg = FALSE;
    GDFLIB_FilterIIR1Init_FLT(&g_sM4Drive.sSpeed.sSpeedFilter);
    PIControllerDesatInit(&g_sM4Drive.sSpeed.sSpeedPiParamsDesat);

    /* Init Blocked rotor filter */
    GDFLIB_FilterMAInit_FLT(0.0F, &g_sM4Drive.msM4BlockedRotorUqFilt);

    /* Clear Scalar control variables */
    g_sM4Drive.sScalarCtrl.fltFreqRamp = 0.0F;
    g_sM4Drive.sScalarCtrl.f16PosElScalar = 0.0F;
    g_sM4Drive.sScalarCtrl.sUDQReq.fltD = 0.0F;
    g_sM4Drive.sScalarCtrl.sUDQReq.fltQ = 0.0F;
    g_sM4Drive.sScalarCtrl.sFreqIntegrator.f32IAccK_1 = 0;
    g_sM4Drive.sScalarCtrl.sFreqIntegrator.f16InValK_1 = 0;
    g_sM4Drive.sScalarCtrl.sFreqRampParams.fltState = 0.0F;

    /* Clear Startup variables */
    g_sM4Drive.sStartUp.f16PosMerged = 0;
    g_sM4Drive.sStartUp.f16PosEst = 0;
    g_sM4Drive.sStartUp.f16PosGen = 0;
    g_sM4Drive.sStartUp.f16RatioMerging = 0;
    g_sM4Drive.sStartUp.fltSpeedRampOpenLoop = 0.0F;
    g_sM4Drive.sStartUp.fltSpeedReq = 0.0F;
    g_sM4Drive.sStartUp.sSpeedIntegrator.f32IAccK_1 = 0;
    g_sM4Drive.sStartUp.sSpeedIntegrator.f16InValK_1 = 0;
    g_sM4Drive.sStartUp.sSpeedRampOpenLoopParams.fltState = 0.0F;

    BSF_init(&g_sM4Drive.sPosition.sNotchFilter);

    /* Clear BEMF and Tracking observers state variables */
    AMCLIB_PMSMBemfObsrvDQInit_A32fff(&g_sM4Drive.sFocPMSM.sBemfObsrv);
    AMCLIB_TrackObsrvInit_A32af(ACC32(0.0), &g_sM4Drive.sFocPMSM.sTo);
}

/*!
 * @brief Fault detention routine - check various faults
 *
 * @param void  No input parameter
 *
 * @return None
 */
RAM_FUNC_CRITICAL static void M4_FaultDetection(void)
{
    /* Clearing actual faults before detecting them again  */
    /* Clear all faults */
    FAULT_CLEAR_ALL(g_sM4Drive.sFaultIdPending);

    /* GD3000:   reset */
     if (g_sM4GD3000.ui8ResetRequest)
     {
         FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_GD3000_IN_RESET);
         g_sM4Drive.ui32FaultIdCapturedExt |= kMC_GD3000_Reset;
     }

     /* GD3000:   DC-bus over-current */
     if (g_sM4GD3000.sStatus.uStatus0.B.overCurrent)
     {
         FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_GD3000_OC);
         g_sM4Drive.ui32FaultIdCapturedExt |= kMC_GD3000_OverCurrent;
     }

     /* GD3000:   VLS under voltage */
     if (g_sM4GD3000.sStatus.uStatus0.B.lowVls)
     {
         FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_GD3000_VLS_UV);
         g_sM4GD3000.ui8ResetRequest = 1;
         g_sM4Drive.ui32FaultIdCapturedExt |= kMC_GD3000_LowVLS;
     }

     /* GD3000:   over-temperature */
     if (g_sM4GD3000.sStatus.uStatus0.B.overTemp)
     {
         FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_GD3000_OT);
         g_sM4Drive.ui32FaultIdCapturedExt |= kMC_GD3000_OverTemperature;
     }

     /* GD3000:   phase error */
     if (g_sM4GD3000.sStatus.uStatus0.B.phaseErr)
     {
         FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_GD3000_PHASE_ERR);
         g_sM4Drive.ui32FaultIdCapturedExt |= kMC_GD3000_PhaseError;
     }

     /* GD3000:   Desaturation detected */
     if (g_sM4GD3000.sStatus.uStatus0.B.desaturation)
     {
         FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_GD3000_DESAT);
         g_sM4Drive.ui32FaultIdCapturedExt |= kMC_GD3000_Desaturation;
     }

    /* Fault:   DC-bus over-current */
    if (MCDRV_eFlexPwm3PhFaultGet(&g_sM4Pwm3ph))
    {
        FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_I_DCBUS_OVER);
        g_sM4Drive.ui32FaultIdCapturedExt |= kMC_OverCurrent;
    }

    /* Fault:   DC-bus over-voltage */
    if (g_sM4Drive.sFocPMSM.fltUDcBusFilt > g_sM4Drive.sFaultThresholds.fltUDcBusOver)
    {
        FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_U_DCBUS_OVER);
        g_sM4Drive.ui32FaultIdCapturedExt |= kMC_OverDcBusVoltage;
    }

    /* Fault:   DC-bus under-voltage */
    if (g_sM4Drive.sFocPMSM.fltUDcBusFilt < g_sM4Drive.sFaultThresholds.fltUDcBusUnder)
    {
        FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_U_DCBUS_UNDER);
        g_sM4GD3000.ui8ResetRequest = 1;
        g_sM4Drive.ui32FaultIdCapturedExt |= kMC_UnderDcBusVoltage;
    }

    /* Check only in SPEED_FOC control, RUN state, kRunState_Spin and kRunState_FreeWheel sub-states */
    if((g_sM4Drive.eControl==kControlMode_SpeedFOC) &&
       (g_sM4Ctrl.eState==kSM_AppRun) &&
       (g_eM4StateRun==kRunState_Spin || g_eM4StateRun==kRunState_Freewheel) &&
       (g_sM4Drive.sMCATctrl.ui16PospeSensor==MCAT_SENSORLESS_CTRL))
    {
        /* Fault: Overload  */
        float_t fltSpeedFiltAbs = MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedFilt);
        float_t fltSpeedRampAbs = MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedRamp);

        if ((fltSpeedFiltAbs < g_sM4Drive.sFaultThresholds.fltSpeedMin) &&
            (fltSpeedRampAbs > g_sM4Drive.sFaultThresholds.fltSpeedMin) &&
            (g_sM4Drive.sSpeed.bSpeedPiStopInteg == TRUE))
        {
            FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_LOAD_OVER);
            g_sM4Drive.ui32FaultIdCapturedExt |= kMC_OverLoad;
        }

        /* Fault: Over-speed  */
        if ((MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedFilt) > g_sM4Drive.sFaultThresholds.fltSpeedOver) &&
            (MLIB_Abs_FLT(g_sM4Drive.sSpeed.fltSpeedCmd) > g_sM4Drive.sFaultThresholds.fltSpeedMin))
        {
            FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_SPEED_OVER);
            g_sM4Drive.ui32FaultIdCapturedExt |= kMC_OverSpeed;
        }

        /* Fault: Blocked rotor detection */
        /* filter of bemf Uq voltage */
        g_sM4Drive. fltBemfUqAvg = GDFLIB_FilterMA_FLT(g_sM4Drive.sFocPMSM.sBemfObsrv.sEObsrv.fltQ,
                                                      &g_sM4Drive.msM4BlockedRotorUqFilt);
        /* check the bemf Uq voltage threshold only in kRunState_Spin - RUN state */
        if ((MLIB_Abs_FLT(g_sM4Drive.fltBemfUqAvg) < g_sM4Drive.sFaultThresholds.fltUqBemf) &&
            (g_eM4StateRun == kRunState_Spin))
            g_sM4Drive.ui16BlockRotorCnt++;
        else
            g_sM4Drive.ui16BlockRotorCnt = 0U;
        /* for bemf voltage detected above limit longer than defined period number set blocked rotor fault*/
        if (g_sM4Drive.ui16BlockRotorCnt > g_sM4Drive.sFaultThresholds.ui16BlockedPerNum)
        {
            FAULT_SET(g_sM4Drive.sFaultIdPending, FAULT_ROTOR_BLOCKED);
            g_sM4Drive.ui16BlockRotorCnt = 0U;
            g_sM4Drive.ui32FaultIdCapturedExt |= kMC_OverLoad;
        }
    }
    /* pass fault to Fault ID Captured */
    g_sM4Drive.sFaultIdCaptured |= g_sM4Drive.sFaultIdPending;
}

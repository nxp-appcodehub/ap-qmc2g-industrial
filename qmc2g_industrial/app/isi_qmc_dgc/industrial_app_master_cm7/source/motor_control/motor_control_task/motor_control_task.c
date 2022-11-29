/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include <mcinit_qmc2g_imxrt1170.h>
#include "motor_control_task.h"
#include "m1_sm_ref_sol.h"
#include "m2_sm_ref_sol.h"
#include "m3_sm_ref_sol.h"
#include "m4_sm_ref_sol.h"
#include "fsl_common.h"
#include "freemaster.h"
#include "mlib.h"
#include "gdflib.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "mc_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"
#include "fsl_debug_console.h"
#include "api_motorcontrol_internal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile static uint16_t ui16ExeTime_fastloopM1,ui16ExeTime_fastloopM2,ui16ExeTime_fastloopM3,ui16ExeTime_fastloopM4;
uint16_t ui16CntSlowloopM1,ui16CntSlowloopM2,ui16CntSlowloopM3,ui16CntSlowloopM4;
uint16_t ui16CntFastloopM1,ui16CntFastloopM2,ui16CntFastloopM3,ui16CntFastloopM4;
static uint16_t ui16IndexCnt;
static uint32_t ui32PosCntCaptured;
static mc_motor_status_t sMotorStatusFromMotorLoopIsr;
mc_motor_command_t g_sMotorCmdTst; /* Motor commands from command queue */
volatile qmc_status_t eSetCmdStatus;
volatile static uint16_t ui16CntSetfastloopStatus;
static mc_motor_status_t sMotorStatusFromDataHub;
volatile static mc_motor_status_t sMotor1Status,sMotor2Status,sMotor3Status,sMotor4Status;
static uint16_t ui16CntPWM1SM0, ui16CntPWM1SM1, ui16CntPWM1SM2;
static uint16_t ui16ADC_ETC_err_flag;

TaskHandle_t g_getMotorStatus_task_handle;

/* Application and board ID  */
app_ver_t   g_sAppId = {
    "evk_imxrt1170",    /* board id */
    "pmsm",             /* motor type */
    MCRSP_VER,          /* sw version */
};
/* Structure used in FM to get required ID's */
app_ver_t   g_sAppIdFM;

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Motor1 slow loop ISR where speed and position loops are executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M1_slowloop_handler(void)
{
	// Clear flag
	PIT1->CHANNEL[0].TFLG = 1;
#if MOTOR1_CMD_FROM_FMSTR == 0
	// Get motor command from external sources
	g_sM1Drive.psMotorCmd = MC_GetMotorCommand_fromISR(kMC_Motor1);

	// Set up motor commands
	switch(g_sM1Drive.psMotorCmd->eControlMethodSel)
	{
	case kMC_ScalarControl:
		g_sM1Drive.eControl = kControlMode_Scalar;
		g_sM1Drive.sScalarCtrl.fltVHzGain = g_sM1Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain;
		g_sM1Drive.sScalarCtrl.fltFreqCmd = g_sM1Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency;
		break;
	case kMC_FOC_SpeedControl:
		g_sM1Drive.eControl = kControlMode_SpeedFOC;
		g_sM1Drive.sSpeed.fltSpeedCmd = g_sM1Drive.psMotorCmd->uSpeed_pos.fltSpeed * M1_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF;
		break;
	case kMC_FOC_PositionControl:
	default:
		g_sM1Drive.eControl = kControlMode_PositionFOC;
		if(g_sM1Drive.psMotorCmd->uSpeed_pos.sPosParam.bIsRandomPosition == true)
		{
			g_sM1Drive.sPosition.bIsRandomPosition = TRUE; // Don't generate trajectory in position loop

		}
		else
		{
			g_sM1Drive.sPosition.bIsRandomPosition = FALSE;
		}
		g_sM1Drive.sPosition.i32Q16PosCmd = g_sM1Drive.psMotorCmd->uSpeed_pos.sPosParam.uPosition.i32Raw;

		break;
	}

	// Turn on/off motor
    if(g_sM1Drive.psMotorCmd->eAppSwitch == kMC_App_On)
    {
    	g_bM1SwitchAppOnOff = TRUE;
    }
    else
    {
    	g_bM1SwitchAppOnOff = FALSE;
    }
#endif
    /* M1 Slow StateMachine call */
    SM_StateMachineSlow(&g_sM1Ctrl);

    sMotorStatusFromMotorLoopIsr.sSlow.eAppSwitch = g_bM1SwitchAppOnOff;
    sMotorStatusFromMotorLoopIsr.sSlow.fltSpeed = g_sM1Drive.sSpeed.fltSpeedFilt*M1_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF;
    sMotorStatusFromMotorLoopIsr.sSlow.uPosition.i32Raw = g_sM1Drive.sPosition.i32Q16PosFdbk;
    MC_SetSlowMotorStatus_fromISR(kMC_Motor1, &sMotorStatusFromMotorLoopIsr.sSlow);

	if(++ui16CntSlowloopM1 >= (uint16_t)(M1_SLOW_LOOP_FREQ))
	{
		ui16CntSlowloopM1 = 0;
	}
	__DSB();
}

/*!
 * @brief Motor2 slow loop ISR where speed and position loops are executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M2_slowloop_handler(void)
{
	// Clear flag
	PIT2->CHANNEL[0].TFLG = 1;
#if MOTOR2_CMD_FROM_FMSTR == 0
	// Get motor command from external sources
	g_sM2Drive.psMotorCmd = MC_GetMotorCommand_fromISR(kMC_Motor2);

	// Set up motor commands
	switch(g_sM2Drive.psMotorCmd->eControlMethodSel)
	{
	case kMC_ScalarControl:
		g_sM2Drive.eControl = kControlMode_Scalar;
		g_sM2Drive.sScalarCtrl.fltVHzGain = g_sM2Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain;
		g_sM2Drive.sScalarCtrl.fltFreqCmd = g_sM2Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency;
		break;
	case kMC_FOC_SpeedControl:
		g_sM2Drive.eControl = kControlMode_SpeedFOC;
		g_sM2Drive.sSpeed.fltSpeedCmd = g_sM2Drive.psMotorCmd->uSpeed_pos.fltSpeed * M1_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF;
		break;
	case kMC_FOC_PositionControl:
	default:
		g_sM2Drive.eControl = kControlMode_PositionFOC;
		if(g_sM2Drive.psMotorCmd->uSpeed_pos.sPosParam.bIsRandomPosition == true)
		{
			g_sM2Drive.sPosition.bIsRandomPosition = TRUE; // Don't generate trajectory in position loop

		}
		else
		{
			g_sM2Drive.sPosition.bIsRandomPosition = FALSE;
		}
		g_sM2Drive.sPosition.i32Q16PosCmd = g_sM2Drive.psMotorCmd->uSpeed_pos.sPosParam.uPosition.i32Raw;

		break;
	}

	// Turn on/off motor
    if(g_sM2Drive.psMotorCmd->eAppSwitch == kMC_App_On)
    {
    	g_bM2SwitchAppOnOff = TRUE;
    }
    else
    {
    	g_bM2SwitchAppOnOff = FALSE;
    }
#endif
    /* M1 Slow StateMachine call */
    SM_StateMachineSlow(&g_sM2Ctrl);

    sMotorStatusFromMotorLoopIsr.sSlow.eAppSwitch = g_bM2SwitchAppOnOff;
    sMotorStatusFromMotorLoopIsr.sSlow.fltSpeed = g_sM2Drive.sSpeed.fltSpeedFilt*M2_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF;
    sMotorStatusFromMotorLoopIsr.sSlow.uPosition.i32Raw = g_sM2Drive.sPosition.i32Q16PosFdbk;
    MC_SetSlowMotorStatus_fromISR(kMC_Motor2, &sMotorStatusFromMotorLoopIsr.sSlow);

	if(++ui16CntSlowloopM2 >= (uint16_t)(M2_SLOW_LOOP_FREQ))
	{
		ui16CntSlowloopM2 = 0;
	}
	__DSB();
}

/*!
 * @brief Motor3 slow loop ISR where speed and position loops are executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M3_slowloop_handler(void)
{
	// Clear flag
	TMR1->CHANNEL[0].SCTRL &= ~TMR_SCTRL_TCF_MASK;
#if MOTOR3_CMD_FROM_FMSTR == 0
	// Get motor command from external sources
	g_sM3Drive.psMotorCmd = MC_GetMotorCommand_fromISR(kMC_Motor3);

	// Set up motor commands
	switch(g_sM3Drive.psMotorCmd->eControlMethodSel)
	{
	case kMC_ScalarControl:
		g_sM3Drive.eControl = kControlMode_Scalar;
		g_sM3Drive.sScalarCtrl.fltVHzGain = g_sM3Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain;
		g_sM3Drive.sScalarCtrl.fltFreqCmd = g_sM3Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency;
		break;
	case kMC_FOC_SpeedControl:
		g_sM3Drive.eControl = kControlMode_SpeedFOC;
		g_sM3Drive.sSpeed.fltSpeedCmd = g_sM3Drive.psMotorCmd->uSpeed_pos.fltSpeed * M1_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF;
		break;
	case kMC_FOC_PositionControl:
	default:
		g_sM3Drive.eControl = kControlMode_PositionFOC;
		if(g_sM3Drive.psMotorCmd->uSpeed_pos.sPosParam.bIsRandomPosition == true)
		{
			g_sM3Drive.sPosition.bIsRandomPosition = TRUE; // Don't generate trajectory in position loop

		}
		else
		{
			g_sM3Drive.sPosition.bIsRandomPosition = FALSE;
		}
		g_sM3Drive.sPosition.i32Q16PosCmd = g_sM3Drive.psMotorCmd->uSpeed_pos.sPosParam.uPosition.i32Raw;

		break;
	}

	// Turn on/off motor
    if(g_sM3Drive.psMotorCmd->eAppSwitch == kMC_App_On)
    {
    	g_bM3SwitchAppOnOff = TRUE;
    }
    else
    {
    	g_bM3SwitchAppOnOff = FALSE;
    }
#endif
    /* M1 Slow StateMachine call */
    SM_StateMachineSlow(&g_sM3Ctrl);

    sMotorStatusFromMotorLoopIsr.sSlow.eAppSwitch = g_bM3SwitchAppOnOff;
    sMotorStatusFromMotorLoopIsr.sSlow.fltSpeed = g_sM3Drive.sSpeed.fltSpeedFilt*M3_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF;
    sMotorStatusFromMotorLoopIsr.sSlow.uPosition.i32Raw = g_sM3Drive.sPosition.i32Q16PosFdbk;
    MC_SetSlowMotorStatus_fromISR(kMC_Motor3, &sMotorStatusFromMotorLoopIsr.sSlow);

	if(++ui16CntSlowloopM3 >= (uint16_t)(M3_SLOW_LOOP_FREQ))
	{
		ui16CntSlowloopM3 = 0;
	}
	__DSB();
}

/*!
 * @brief Motor4 slow loop ISR where speed and position loops are executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M4_slowloop_handler(void)
{
	// Clear flag
	TMR2->CHANNEL[0].SCTRL &= ~TMR_SCTRL_TCF_MASK;
#if MOTOR4_CMD_FROM_FMSTR == 0
	// Get motor command from external sources
	g_sM4Drive.psMotorCmd = MC_GetMotorCommand_fromISR(kMC_Motor4);

	// Set up motor commands
	switch(g_sM4Drive.psMotorCmd->eControlMethodSel)
	{
	case kMC_ScalarControl:
		g_sM4Drive.eControl = kControlMode_Scalar;
		g_sM4Drive.sScalarCtrl.fltVHzGain = g_sM4Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlVHzGain;
		g_sM4Drive.sScalarCtrl.fltFreqCmd = g_sM4Drive.psMotorCmd->uSpeed_pos.sScalarParam.fltScalarControlFrequency;
		break;
	case kMC_FOC_SpeedControl:
		g_sM4Drive.eControl = kControlMode_SpeedFOC;
		g_sM4Drive.sSpeed.fltSpeedCmd = g_sM4Drive.psMotorCmd->uSpeed_pos.fltSpeed * M1_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF;
		break;
	case kMC_FOC_PositionControl:
	default:
		g_sM4Drive.eControl = kControlMode_PositionFOC;
		if(g_sM4Drive.psMotorCmd->uSpeed_pos.sPosParam.bIsRandomPosition == true)
		{
			g_sM4Drive.sPosition.bIsRandomPosition = TRUE; // Don't generate trajectory in position loop

		}
		else
		{
			g_sM4Drive.sPosition.bIsRandomPosition = FALSE;
		}
		g_sM4Drive.sPosition.i32Q16PosCmd = g_sM4Drive.psMotorCmd->uSpeed_pos.sPosParam.uPosition.i32Raw;

		break;
	}

	// Turn on/off motor
    if(g_sM4Drive.psMotorCmd->eAppSwitch == kMC_App_On)
    {
    	g_bM4SwitchAppOnOff = TRUE;
    }
    else
    {
    	g_bM4SwitchAppOnOff = FALSE;
    }
#endif
    /* M1 Slow StateMachine call */
    SM_StateMachineSlow(&g_sM4Ctrl);

    sMotorStatusFromMotorLoopIsr.sSlow.eAppSwitch = g_bM4SwitchAppOnOff;
    sMotorStatusFromMotorLoopIsr.sSlow.fltSpeed = g_sM4Drive.sSpeed.fltSpeedFilt*M4_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF;
    sMotorStatusFromMotorLoopIsr.sSlow.uPosition.i32Raw = g_sM4Drive.sPosition.i32Q16PosFdbk;
    MC_SetSlowMotorStatus_fromISR(kMC_Motor4, &sMotorStatusFromMotorLoopIsr.sSlow);

	if(++ui16CntSlowloopM4 >= (uint16_t)(M4_SLOW_LOOP_FREQ))
	{
		ui16CntSlowloopM4 = 0;
	}
	__DSB();
}

/*!
 * @brief Motor1 fast loop ISR where current loop is executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M1_fastloop_handler(void)
{


	RESET_TIMER1();
	// Clear flag
	ADC_ETC->DONE0_1_IRQ = ADC_ETC_DONE0_1_IRQ_TRIG4_DONE0_MASK;


	MCDRV_GetRotorCurrentRev(&g_sM1QdcSensor);
	MCDRV_GetRotorDeltaRev(&g_sM1QdcSensor);


    /*========================= State machine begin========================================================*/

    /* M1 State machine */
    SM_StateMachineFast(&g_sM1Ctrl);


    /*========================= State machine end =========================================================*/
    RESET_TIMER2();
    sMotorStatusFromMotorLoopIsr.sFast.eFaultStatus = g_sM1Drive.ui32FaultIdCapturedExt;
    g_sM1Drive.ui32FaultIdCapturedExt = kMC_NoFaultMC; // Clear captured fault Id
    sMotorStatusFromMotorLoopIsr.sFast.eMotorState = g_sM1Ctrl.eState;
    sMotorStatusFromMotorLoopIsr.sFast.fltIa = g_sM1Drive.sFocPMSM.sIABC.fltA;
    sMotorStatusFromMotorLoopIsr.sFast.fltIb = g_sM1Drive.sFocPMSM.sIABC.fltB;
    sMotorStatusFromMotorLoopIsr.sFast.fltIc = g_sM1Drive.sFocPMSM.sIABC.fltC;
    sMotorStatusFromMotorLoopIsr.sFast.fltVDcBus = g_sM1Drive.sFocPMSM.fltUDcBusFilt;
    sMotorStatusFromMotorLoopIsr.sFast.fltValpha = g_sM1Drive.sFocPMSM.sUAlBeReq.fltAlpha;
    sMotorStatusFromMotorLoopIsr.sFast.fltVbeta = g_sM1Drive.sFocPMSM.sUAlBeReq.fltBeta;
    MC_SetFastMotorStatus_fromISR(kMC_Motor1, &sMotorStatusFromMotorLoopIsr.sFast);

    ui16CntSetfastloopStatus = READ_TIMER2();
#if FEATURE_FREEMASTER_ENABLE
   /* Recorder of FreeMASTER*/
	FMSTR_Recorder(0);
#endif

    if(++ui16CntFastloopM1 >= (uint16_t)(M1_FAST_LOOP_FREQ))
    {
    	ui16CntFastloopM1 = 0;
#if FEATURE_TOOGLE_USER_LED_ENABLE
    	GPIO_PortToggle(BOARD_INITUSERLEDSPINS_USER_LED1_BOOT_CFG10_PERIPHERAL, 1<<BOARD_INITUSERLEDSPINS_USER_LED1_BOOT_CFG10_CHANNEL);
#endif
    }
    ui16ExeTime_fastloopM1 = READ_TIMER1();


    __DSB();
}

/*!
 * @brief Motor2 fast loop ISR where current loop is executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M2_fastloop_handler(void)
{

//	GPIO_PortSet(DEBUG_INITTESTIO_BUTTON1_GPIO, DEBUG_INITTESTIO_BUTTON1_GPIO_PIN_MASK);

	RESET_TIMER1();

	// Clear flag
	ADC_ETC->DONE0_1_IRQ = ADC_ETC_DONE0_1_IRQ_TRIG5_DONE1_MASK;


	MCDRV_GetRotorCurrentRev(&g_sM2QdcSensor);
	MCDRV_GetRotorDeltaRev(&g_sM2QdcSensor);


    /*========================= State machine begin========================================================*/

    /* M1 State machine */
    SM_StateMachineFast(&g_sM2Ctrl);

   	/*========================= State machine end =========================================================*/
    sMotorStatusFromMotorLoopIsr.sFast.eFaultStatus = g_sM2Drive.ui32FaultIdCapturedExt;
    g_sM2Drive.ui32FaultIdCapturedExt = kMC_NoFaultMC; // Clear captured fault Id
    sMotorStatusFromMotorLoopIsr.sFast.eMotorState = g_sM2Ctrl.eState;
    sMotorStatusFromMotorLoopIsr.sFast.fltIa = g_sM2Drive.sFocPMSM.sIABC.fltA;
    sMotorStatusFromMotorLoopIsr.sFast.fltIb = g_sM2Drive.sFocPMSM.sIABC.fltB;
    sMotorStatusFromMotorLoopIsr.sFast.fltIc = g_sM2Drive.sFocPMSM.sIABC.fltC;
    sMotorStatusFromMotorLoopIsr.sFast.fltVDcBus = g_sM2Drive.sFocPMSM.fltUDcBusFilt;
    sMotorStatusFromMotorLoopIsr.sFast.fltValpha = g_sM2Drive.sFocPMSM.sUAlBeReq.fltAlpha;
    sMotorStatusFromMotorLoopIsr.sFast.fltVbeta = g_sM2Drive.sFocPMSM.sUAlBeReq.fltBeta;
    MC_SetFastMotorStatus_fromISR(kMC_Motor2, &sMotorStatusFromMotorLoopIsr.sFast);

    if(++ui16CntFastloopM2 >= (uint16_t)(M2_FAST_LOOP_FREQ))
    {
    	ui16CntFastloopM2 = 0;
    }

    ui16ExeTime_fastloopM2 = READ_TIMER1();

    __DSB();
}

/*!
 * @brief Motor3 fast loop ISR where current loop is executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M3_fastloop_handler(void)
{
//	GPIO_PortSet(DEBUG_INITTESTIO_BUTTON1_GPIO, DEBUG_INITTESTIO_BUTTON1_GPIO_PIN_MASK);

	RESET_TIMER1();

	// Clear flag
	ADC_ETC->DONE2_3_ERR_IRQ = ADC_ETC_DONE2_3_ERR_IRQ_TRIG6_DONE2_MASK;


	MCDRV_GetRotorCurrentRev(&g_sM3QdcSensor);
	MCDRV_GetRotorDeltaRev(&g_sM3QdcSensor);


    /*========================= State machine begin========================================================*/

    /* M1 State machine */
    SM_StateMachineFast(&g_sM3Ctrl);

   	/*========================= State machine end =========================================================*/
    sMotorStatusFromMotorLoopIsr.sFast.eFaultStatus = g_sM3Drive.ui32FaultIdCapturedExt;
    g_sM3Drive.ui32FaultIdCapturedExt = kMC_NoFaultMC; // Clear captured fault Id
    sMotorStatusFromMotorLoopIsr.sFast.eMotorState = g_sM3Ctrl.eState;
    sMotorStatusFromMotorLoopIsr.sFast.fltIa = g_sM3Drive.sFocPMSM.sIABC.fltA;
    sMotorStatusFromMotorLoopIsr.sFast.fltIb = g_sM3Drive.sFocPMSM.sIABC.fltB;
    sMotorStatusFromMotorLoopIsr.sFast.fltIc = g_sM3Drive.sFocPMSM.sIABC.fltC;
    sMotorStatusFromMotorLoopIsr.sFast.fltVDcBus = g_sM3Drive.sFocPMSM.fltUDcBusFilt;
    sMotorStatusFromMotorLoopIsr.sFast.fltValpha = g_sM3Drive.sFocPMSM.sUAlBeReq.fltAlpha;
    sMotorStatusFromMotorLoopIsr.sFast.fltVbeta = g_sM3Drive.sFocPMSM.sUAlBeReq.fltBeta;
    MC_SetFastMotorStatus_fromISR(kMC_Motor3, &sMotorStatusFromMotorLoopIsr.sFast);

    if(++ui16CntFastloopM3 >= (uint16_t)(M3_FAST_LOOP_FREQ))
    {
    	ui16CntFastloopM3 = 0;
    }

    ui16ExeTime_fastloopM3 = READ_TIMER1();

    __DSB();
}

/*!
 * @brief Motor4 fast loop ISR where current loop is executed.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void M4_fastloop_handler(void)
{

//	GPIO_PortSet(DEBUG_INITTESTIO_BUTTON1_GPIO, DEBUG_INITTESTIO_BUTTON1_GPIO_PIN_MASK);
	RESET_TIMER1();

	// Clear flag
	ADC_ETC->DONE2_3_ERR_IRQ = ADC_ETC_DONE2_3_ERR_IRQ_TRIG7_DONE3_MASK;


	MCDRV_GetRotorCurrentRev(&g_sM4QdcSensor);
	MCDRV_GetRotorDeltaRev(&g_sM4QdcSensor);


    /*========================= State machine begin========================================================*/

    /* M1 State machine */
    SM_StateMachineFast(&g_sM4Ctrl);

   	/*========================= State machine end =========================================================*/
    sMotorStatusFromMotorLoopIsr.sFast.eFaultStatus = g_sM4Drive.ui32FaultIdCapturedExt;
    g_sM4Drive.ui32FaultIdCapturedExt = kMC_NoFaultMC; // Clear captured fault Id
    sMotorStatusFromMotorLoopIsr.sFast.eMotorState = g_sM4Ctrl.eState;
    sMotorStatusFromMotorLoopIsr.sFast.fltIa = g_sM4Drive.sFocPMSM.sIABC.fltA;
    sMotorStatusFromMotorLoopIsr.sFast.fltIb = g_sM4Drive.sFocPMSM.sIABC.fltB;
    sMotorStatusFromMotorLoopIsr.sFast.fltIc = g_sM4Drive.sFocPMSM.sIABC.fltC;
    sMotorStatusFromMotorLoopIsr.sFast.fltVDcBus = g_sM4Drive.sFocPMSM.fltUDcBusFilt;
    sMotorStatusFromMotorLoopIsr.sFast.fltValpha = g_sM4Drive.sFocPMSM.sUAlBeReq.fltAlpha;
    sMotorStatusFromMotorLoopIsr.sFast.fltVbeta = g_sM4Drive.sFocPMSM.sUAlBeReq.fltBeta;
    MC_SetFastMotorStatus_fromISR(kMC_Motor4, &sMotorStatusFromMotorLoopIsr.sFast);

    if(++ui16CntFastloopM4 >= (uint16_t)(M4_FAST_LOOP_FREQ))
    {
    	ui16CntFastloopM4 = 0;
    }

    ui16ExeTime_fastloopM4 = READ_TIMER1();

    __DSB();
}

/*!
 * @brief Motor1 encoder index ISR.
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void ENC1_IRQHandler(void)
{
	volatile static uint32_t ui32Dummy;

	ENC1->CTRL |= ENC_CTRL_XIRQ_MASK; // Clear flag
	ui32Dummy = ENC1->LPOS;
	ui32Dummy++;

	ui32PosCntCaptured = ENC1->LPOSH|((uint32_t)(ENC1->UPOSH)<<16);
	ui16IndexCnt++;

//	f32Pos = ((uint64_t)g_sM1QdcSensor.i32Q10Cnt2PosGain * ui32PosCntCaptured)>>10; // Q22.10 * Q32 = Q54.10, get rid of the last 10 fractional bits, keeping the last 32bits of Q54
//	                                                                       // think of this result as a Q1.31 format, which represents -pi ~ pi
//	this->f32PosMech = f32Pos - this->f32PosMechInit + this->f32PosMechOffset;
//	this->f32PosElec = (uint64_t)this->f32PosMech * this->ui16PolePair;
//	this->f16PosElec = MLIB_Conv_F16l(this->f32PosElec);

	__DSB();
}


/*!
 * @brief eFlexPWM1 SM0 VAL5 compare ISR. This is the time point where ADC should S/C motor1 currents
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void PWM1_0_IRQHandler(void)  // for debug
{
//	GPIO_PortToggle(DEBUG_INITTESTIO_BUTTON1_GPIO, DEBUG_INITTESTIO_BUTTON1_GPIO_PIN_MASK);
	if(++ui16CntPWM1SM0 >= (uint16_t)(M1_PWM_FREQ))
	{
		ui16CntPWM1SM0 = 0;
	}
	PWM1->SM[0].STS = PWM_STS_CMPF(0x20);
	__DSB();
}

/*!
 * @brief eFlexPWM1 SM1 VAL4&VAL5 compare ISR. This is the time point where ADC should S/C motor2&motor3 currents
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void PWM1_1_IRQHandler(void) // for debug
{
//	GPIO_PortToggle(DEBUG_INITTESTIO_BUTTON2_GPIO, DEBUG_INITTESTIO_BUTTON2_GPIO_PIN_MASK);
	if(++ui16CntPWM1SM1 >= (uint16_t)(M1_PWM_FREQ))
	{
		ui16CntPWM1SM1 = 0;
	}
	PWM1->SM[1].STS = PWM_STS_CMPF(0x30);
	__DSB();
}

/*!
 * @brief eFlexPWM1 SM2 VAL4 compare ISR. This is the time point where ADC should S/C motor4 currents
 *
 * @param None
 *
 * @return None
 */
RAM_FUNC_CRITICAL void PWM1_2_IRQHandler(void) // for debug
{
//	GPIO_PortToggle(DEBUG_INITTESTIO_BUTTON2_GPIO, DEBUG_INITTESTIO_BUTTON2_GPIO_PIN_MASK);
	if(++ui16CntPWM1SM2 >= (uint16_t)(M1_PWM_FREQ))
	{
		ui16CntPWM1SM2 = 0;
	}
	PWM1->SM[2].STS = PWM_STS_CMPF(0x10);
	__DSB();
}

/*!
 * @brief ADC_ETC Error ISR. Reset ADC_ETC when there is anything wrong with ADC triggering sequence due to CPU overloaded or something else
 *
 * @param None
 *
 * @return None
 */
 
RAM_FUNC_CRITICAL void ADC_ETC_ERROR_IRQ_IRQHandler(void)
{
	// Clear flag
	ADC_ETC->DONE2_3_ERR_IRQ = 0x00ff0000;

	adc_etc_init();

	ui16ADC_ETC_err_flag++;

	__DSB();

}

/*!
 * @brief Get 4 motor status from status queue. Data hub task is responsible to put motor status into the queue constantly.
          MC_DequeueMotorStatus() is invoked in getMotorStatusTask() every 30ms to get 4 motors status from a motor status queue.
          (a) Motor fast loop and slow loop ISRs put motor status into an internal memory. Internal memory won't be updated if DataHubTask() is reading it.
          (b) DataHubTask() is unblocked every 100ms to get motor status from this internal memory, and then put them into the motor status queue.
          This task must be executed more frequently than Data hub task, otherwise the status queue can be full.
 *
 * @param None
 *
 * @return None
 */

#if FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB

void getMotorStatusTask(void *pvParameters)
{
	qmc_status_t sStatus;

	qmc_msg_queue_handle_t *ptr;

	TickType_t xLastWakeTime;

	xLastWakeTime = xTaskGetTickCount();

	sStatus = MC_GetNewStatusQueueHandle(&ptr, 1); // every element in the queue is needed

	if(sStatus != kStatus_QMC_Ok)
	{
		PRINTF("Fail to get Status Queue handler");
		while(1);
	}

	while(1)
	{

		sStatus = MC_DequeueMotorStatus(ptr, 0, &sMotorStatusFromDataHub);

		switch(sMotorStatusFromDataHub.eMotorId)
		{
		case kMC_Motor1:
			sMotor1Status.sFast = sMotorStatusFromDataHub.sFast;
			sMotor1Status.sSlow = sMotorStatusFromDataHub.sSlow;
			break;
		case kMC_Motor2:
			sMotor2Status.sFast = sMotorStatusFromDataHub.sFast;
			sMotor2Status.sSlow = sMotorStatusFromDataHub.sSlow;
			break;
		case kMC_Motor3:
			sMotor3Status.sFast = sMotorStatusFromDataHub.sFast;
			sMotor3Status.sSlow = sMotorStatusFromDataHub.sSlow;
			break;
		case kMC_Motor4:
			sMotor4Status.sFast = sMotorStatusFromDataHub.sFast;
			sMotor4Status.sSlow = sMotorStatusFromDataHub.sSlow;
			break;
		default:
			PRINTF("Motor Id is not correct!");
			break;
		}

		if((sStatus != kStatus_QMC_ErrNoMsg)&&(sStatus != kStatus_QMC_Ok))
		{
			PRINTF("Fail to get Status Queue contents");
			while(1);
		}
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30));
	}

}
#endif

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/**********************************************************************/
// File Name: {FM_project_loc}/../src/projects/twrkv58f/pmsm_appconfig.h 
//
// Date:  November 15, 2017, 14:7:31
//
// Automatically generated file for static configuration of the PMSM FOC application
/**********************************************************************/

#ifndef __M2_PMSM_APPCONFIG_H
#define __M2_PMSM_APPCONFIG_H

#include "trigonometric.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MOTOR2_CMD_FROM_FMSTR 0 // 0: from external source; 1: from freemaster

// HW configurations
//-----------------------------------------------------------------------------------------------------------
//                       Channel_number        Side          Command slot number
#define M2_ADC1_IA       {    1,               SIDE_A,           4          }
#define M2_ADC2_IA       {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }
#define M2_ADC1_IB       {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }
#define M2_ADC2_IB       {    1,               SIDE_A,           4          }
#define M2_ADC1_IC       {    4,               SIDE_A,           5          }
#define M2_ADC2_IC       {    4,               SIDE_A,           5          }
#define M2_ADC1_VDC      {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }
#define M2_ADC2_VDC      {    5,               SIDE_B,           6          }
#define M2_ADC1_AUX      {    2,               SIDE_B,           6          }
#define M2_ADC2_AUX      {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }

#define M2_ADC_AVG_NUM          2      /* 2^AVG_NUM conversions averaged */
#define M2_ADC_TRIGGER_DELAY 	1.25   /* [us] */
#define M2_ADC_AVERAGE_TIME     1.0225 /* [us], 0.2556us per sample */

/* Assignment of eFlexPWM channels to motor 1 phases
 * 0 - PWM channels A0&B0 - sub-module 0
 * 1 - PWM channels A1&B1 - sub-module 1
 * 2 - PWM channels A2&B2 - sub-module 2
 */
#define M2_PWM_PAIR_PHA (0)
#define M2_PWM_PAIR_PHB (1)
#define M2_PWM_PAIR_PHC (2)

/* Over Current Fault detection */
#define M2_FAULT_NUM (0)


#define M2_M_2310P_LN_04K

//#define M2_M_2311S_LN_08K

// Motor Parameters 
//-----------------------------------------------------------------------------------------------------------  
#define M2_MOTOR_PP 		(4)
#ifdef M2_M_2310P_LN_04K
#define M2_LD				(0.0002)  /* [H], d-axis inductance */
#define M2_LQ				(0.0002)	  /* [H], q-axis inductance */
#define M2_R				(0.36)	  /* [ohm], phase resistance */
#define M2_ENCODER_LINES	(1000)	  	 /* Encoder lines per mechanical revolution */
#endif

#ifdef M2_M_2311S_LN_08K
#define M2_LD				(0.001465)  /* [H], d-axis inductance */
#define M2_LQ				(0.001465)	  /* [H], q-axis inductance */
#define M2_R				(1.38)	  /* [ohm], phase resistance */
#define M2_ENCODER_LINES	(2000)	  	 /* Encoder lines per mechanical revolution */
#endif

#define M2_N_NOM 			(6000.0F) /* [RPM], motor nominal mechanical speed */
#define M2_I_PH_NOM 		(3.0F)    /* [A], motor nominal current */

// Application Scales 
//-----------------------------------------------------------------------------------------------------------  
#define M2_I_MAX 		(16.665F) /* [A], defined by HW, phase current scale */
#define M2_U_DCB_MAX 	(60.08F)  /* [V], defined by HW, DC bus voltage scale */
#define M2_N_MAX 		(6500.0F) /* [RPM], rotor mechanical speed scale */


#define M2_N_NOM_RAD	(2*PI*M2_N_NOM*M2_MOTOR_PP/60.0)	/* [rad/s], electrical nominal angular speed */
#define M2_U_MAX 		(M2_U_DCB_MAX/1.732F)  				/* [V], phase voltage scale */
#define M2_FREQ_MAX 	(M2_MOTOR_PP*M2_N_MAX/60.0F)  		/* [Hz], electrical speed scale */
#define M2_RAD_MAX		(2*PI*M2_N_MAX*M2_MOTOR_PP/60.0F)	/* [rad/s], electrical angular speed scale */
#define M2_SPEED_FRAC_TO_ANGULAR_COEFF  (float_t)(2*PI*M2_N_MAX*M2_MOTOR_PP/60.0) /* A coefficient that converts scaled speed to electrical angular speed in unit of rad/s */
#define M2_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF (float_t)(2*PI*M2_MOTOR_PP/60.0)
#define M2_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF (float_t)(60.0F / (M2_MOTOR_PP * 2.0F * FLOAT_PI))

// Fault thresholds
//-----------------------------------------------------------------------------------------------------------
#define M2_U_DCB_TRIP 			(28.0F)  	/* [V], brake is on when DC bus reaches this voltage */
#define M2_U_DCB_UNDERVOLTAGE 	(19.0F)  	/* [V], DC bus under voltage threshold */
#define M2_U_DCB_OVERVOLTAGE 	(30.8F)  	/* [V], DC bus over voltage threshold */
#define M2_N_OVERSPEED 			(4500.0F) 	/* [RPM], mechanical over speed threshold */
#define M2_N_MIN 				(300.0F)   	/* [RPM], the minimum mechanical speed that is required for speed command when sensorless approach is used */
  
#define M2_N_OVERSPEED_RAD		(2*PI*M2_N_OVERSPEED*M2_MOTOR_PP/60.0) /* [rad/s], electrical angular over speed threshold */
#define M2_N_MIN_RAD			(2*PI*M2_N_MIN*M2_MOTOR_PP/60.0)	   /* [rad/s], electrical angular minimum speed to ensure sensorless observer can work */

// Control loop frequency
//-----------------------------------------------------------------------------------------------------------
#define M2_PWM_FREQ               (16000.0)   /* [Hz], PWM frequency */
#define M2_FOC_FREQ_VS_PWM_FREQ   (1)         /* FOC calculation is called every n-th PWM reload */
#define M2_PWM_DEADTIME           (0.8)       /* [us], Output PWM deadtime value in micro-seconds */
#define M2_FAST_LOOP_FREQ		  (float_t)(M2_PWM_FREQ/M2_FOC_FREQ_VS_PWM_FREQ)  /* [Hz], fast loop control frequency */
#define M2_SLOW_LOOP_FREQ		  (8000.0F)   /* [Hz], slow loop control frequency */

// DC bus voltage filter
//-----------------------------------------------------------------------------------------------------------
#define M2_UDCB_CUTOFF_FREQ  	(160.0F)   /* [Hz], cutoff frequency of IIR1 low pass filter */

#define M2_UDCB_IIR_B0			WARP(M2_UDCB_CUTOFF_FREQ, M2_FAST_LOOP_FREQ)/(WARP(M2_UDCB_CUTOFF_FREQ, M2_FAST_LOOP_FREQ) + 2.0F)
#define M2_UDCB_IIR_B1			WARP(M2_UDCB_CUTOFF_FREQ, M2_FAST_LOOP_FREQ)/(WARP(M2_UDCB_CUTOFF_FREQ, M2_FAST_LOOP_FREQ) + 2.0F)
#define M2_UDCB_IIR_A1			(1.0F - M2_UDCB_IIR_B0 - M2_UDCB_IIR_B1)

  
// Mechanical alignment 
//-----------------------------------------------------------------------------------------------------------  
#define M2_ALIGN_VOLTAGE 		(1.0F)	/* [v], alignment voltage vector length */
#define M2_ALIGN_DURATION_TIME	(0.5F)  /* [s], alignment stage duration */
#define M2_BOOTSTRAP_CHARGE_TIME (0.05) /* [s], charging bootstrap capacitor duration, must be smaller than alignment duration */
#define M2_BOOTSTRAP_DUTY       (0.1)   /* [n/a], duty used in bootstrap capacitor charging */
#define M2_ALIGN_DURATION		(uint16_t)(M2_ALIGN_DURATION_TIME * M2_FAST_LOOP_FREQ)
#define M2_BOOTSTRAP_CHARGE_DURATION   (uint16_t)(M2_BOOTSTRAP_CHARGE_TIME * M2_FAST_LOOP_FREQ)

  
// Application counters 
//-----------------------------------------------------------------------------------------------------------
#define M2_CALIB_DURATION_TIME		(0.5F) /* [s], phase currents calibration stage duration */
#define M2_FAULT_DURATION_TIME  	(6.0F) /* [s], Fault state duration after all faults disappear  */
#define M2_FREEWHEEL_DURATION_TIME 	(1.5F) /* [s], freewheel state duration after motor is stopped from running */

#define M2_CALIB_DURATION			(uint16_t)(M2_CALIB_DURATION_TIME * M2_SLOW_LOOP_FREQ)
#define M2_FAULT_DURATION			(uint16_t)(M2_FAULT_DURATION_TIME * M2_SLOW_LOOP_FREQ)
#define M2_FREEWHEEL_DURATION		(uint16_t)(M2_FREEWHEEL_DURATION_TIME * M2_SLOW_LOOP_FREQ)

  
// Miscellaneous 
//-----------------------------------------------------------------------------------------------------------  
#define M2_E_BLOCK_TRH 			(1.4F)  /* [v], estimated bemf threshold on Q-axis. */
#define M2_E_BLOCK_PER 			(2000)  /* Motor is deemed as blocked when bemf on Q-axis is less than the threshold for this number of times in fast loop  */
#define M2_BLOCK_ROT_FAULT_SH   (1.0F/5) /* filter window */

//Current Loop Control - compared with 2rd order system
//----------------------------------------------------------------------
#define M2_CLOOP_ATT			(0.707F) 	/* attenuation */
#ifdef M2_M_2310P_LN_04K
#define M2_CLOOP_FREQ			(2000.0F)	/* [Hz], oscillating frequency */
#endif

#ifdef M2_M_2311S_LN_08K
#define M2_CLOOP_FREQ			(1200.0F)	/* [Hz], oscillating frequency */
#endif

//Current Controller Output Limit       
#define M2_CLOOP_LIMIT                     (0.95) /* Voltage output limitation, based on real time available maximum phase voltage amplitude */

//D-axis Controller - Parallel type     
#define M2_D_KP_GAIN                       (2*M2_CLOOP_ATT*2*PI*M2_CLOOP_FREQ*M2_LD - M2_R)
#define M2_D_KI_GAIN                       (2*PI*M2_CLOOP_FREQ*2*PI*M2_CLOOP_FREQ*M2_LD/M2_FAST_LOOP_FREQ)
//Q-axis Controller - Parallel type     
#define M2_Q_KP_GAIN                       (2*M2_CLOOP_ATT*2*PI*M2_CLOOP_FREQ*M2_LQ - M2_R)
#define M2_Q_KI_GAIN                       (2*PI*M2_CLOOP_FREQ*2*PI*M2_CLOOP_FREQ*M2_LQ/M2_FAST_LOOP_FREQ)


//Speed Loop Control                    
//----------------------------------------------------------------------
//Speed Controller - Parallel type      
#define M2_SPEED_PI_PROP_GAIN              (0.025F)//(0.0197F)		/* proportional gain */
#define M2_SPEED_PI_INTEG_GAIN             (0.0002F)//(0.00065685F)	/* integral gain */
#define M2_SPEED_PI_PROP_SENSORLESS_GAIN              (0.005F)		/* proportional gain */
#define M2_SPEED_PI_INTEG_SENSORLESS_GAIN             (0.00002F)	/* integral gain */
#define M2_SPEED_PI_DESAT_GAIN			   (0.5F)
#define M2_SPEED_LOOP_HIGH_LIMIT           (3.0F)			/* [A], current output upper limitation */
#define M2_SPEED_LOOP_LOW_LIMIT            (-3.0F)			/* [A], current output lower limitation */
#define M2_CL_SPEED_RAMP			   	   (8000.0F) 		/* [RPM/s], (mechanical) speed accelerating rate during closed-loop */
#define M2_SPEED_CUTOFF_FREQ    		   (100.0F)          /* [Hz], cutoff frequency of IIR1 low pass filter for speed from sensor or observer */
#define M2_SPEED_LOOP_IQ_FWD_GAIN 		   (0.5F)		    /* [A], Iq feed forward gain */

#define M2_CUR_FWD_CUTOFF_FREQ  		   (160.0F)   /* [Hz], cutoff frequency of IIR1 low pass filter */

#define M2_CUR_FWD_IIR_B0				   WARP(M2_CUR_FWD_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ)/(WARP(M2_CUR_FWD_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ) + 2.0F)
#define M2_CUR_FWD_IIR_B1				   WARP(M2_CUR_FWD_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ)/(WARP(M2_CUR_FWD_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ) + 2.0F)
#define M2_CUR_FWD_IIR_A1				   (1.0F - M2_CUR_FWD_IIR_B0 - M2_CUR_FWD_IIR_B1)

#define M2_CL_SPEED_RAMP_RAD			   (2.0F*PI*M2_CL_SPEED_RAMP*M2_MOTOR_PP/60.0) /* transfer mechanical speed in RPM to electrical angular speed */
#define M2_SPEED_RAMP_UP  				   (M2_CL_SPEED_RAMP_RAD/M2_SLOW_LOOP_FREQ)
#define M2_SPEED_RAMP_DOWN  			   (M2_CL_SPEED_RAMP_RAD/M2_SLOW_LOOP_FREQ)

#define M2_SPEED_IIR_B0				   	   WARP(M2_SPEED_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ)/(WARP(M2_SPEED_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ) + 2.0F)
#define M2_SPEED_IIR_B1				       WARP(M2_SPEED_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ)/(WARP(M2_SPEED_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ) + 2.0F)
#define M2_SPEED_IIR_A1				       (1.0F - M2_SPEED_IIR_B0 - M2_SPEED_IIR_B1)


// Position loop control
//-----------------------------------------------------------------------------------------------------------
#define M2_QDC_TRAJECTORY_FILTER_FREQ		4.0 		 /* [Hz], cutoff frequency of a 2nd order IIR filter to get a smoothed position reference */
#define M2_QDC_POSITION_RAMP           		20.0 		 /* [Revolutions/s], position ramping rate for a position command */
#define M2_QDC_POSITION_CTRL_P_GAIN			4.0			 /* proportional gain for position controller */
#define M2_QDC_POSITION_CTRL_LIMIT	   		4000.0F		 /* [RPM], mechanical speed - position controller output upper limit. Lower limit is its negative value */
#define M2_QDC_POSITION_CTRL_SPEED_FWD_GAIN 1.0F		 /* Speed feed forward gain */


#define M2_QDC_POSITION_RAMP_UP_FRAC	 	((M2_QDC_POSITION_RAMP/M2_SLOW_LOOP_FREQ)*65535)  /* Q16.16 format */
#define M2_QDC_POSITION_RAMP_DOWN_FRAC	 	((M2_QDC_POSITION_RAMP/M2_SLOW_LOOP_FREQ)*65535)  /* Q16.16 format */
#define M2_QDC_POSITION_CTRL_LIMIT_FRAC		FRAC16(M2_QDC_POSITION_CTRL_LIMIT/M2_N_MAX)
#define M2_QDC_TRAJECTORY_FILTER_FREQ_FRAC  FRAC32(2*PI*M2_QDC_TRAJECTORY_FILTER_FREQ/M2_SLOW_LOOP_FREQ)
#define M2_QDC_POSITION_CTRL_P_GAIN_FRAC	ACC32(M2_QDC_POSITION_CTRL_P_GAIN)

  
// Position & Speed Sensors Module 
//-----------------------------------------------------------------------------------------------------------  

#define M2_QDC_TIMER_PRESCALER				6 //10			 /* Prescaler for the timer within QDC, the prescaling value is 2^Mx_QDC_TIMER_PRESCALER */
#define M2_QDC_CLOCK	 					240000000	 /* [Hz], QDC module clock, which is the bus clock of the system */
#define M2_QDC_SPEED_FILTER_CUTOFF_FREQ 	100.0 		 /* [Hz], cutoff frequency of IIR1 low pass filter for calculated raw speed out of QDC HW feature */
#define M2_QDC_TO_ATT						(0.85F)		 /* attenuation for tracking observer, which is to estimate rotor speed from real QDC position */
#define M2_QDC_TO_FREQ						(300.0)		 /* [Hz], oscillating frequency for tracking observer */

#define M2_QDC_TIMER_FREQUENCY  			(M2_QDC_CLOCK/EXPON(2,M2_QDC_TIMER_PRESCALER)) 			/* [Hz], the clock frequency for the timer within QDC */
#define M2_SPEED_CAL_CONST  				((60.0*M2_QDC_TIMER_FREQUENCY/(4*M2_ENCODER_LINES*M2_N_MAX)) * 134217728)	/* A constant to calculate mechanical speed out of QDC HW feature, Q5.27 */
#define M2_QDC_TO_KP_GAIN					(2.0F*M2_QDC_TO_ATT*2*PI*M2_QDC_TO_FREQ)
#define M2_QDC_TO_KI_GAIN					(2*PI*M2_QDC_TO_FREQ * 2*PI*M2_QDC_TO_FREQ/M2_FAST_LOOP_FREQ)
#define M2_QDC_TO_THETA_GAIN				(1.0F/(PI*M2_FAST_LOOP_FREQ))


#define M2_QDC_SPEED_FILTER_IIR_B0				   	   WARP(M2_QDC_SPEED_FILTER_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ)/(WARP(M2_QDC_SPEED_FILTER_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ) + 2.0F)
#define M2_QDC_SPEED_FILTER_IIR_B1				       WARP(M2_QDC_SPEED_FILTER_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ)/(WARP(M2_QDC_SPEED_FILTER_CUTOFF_FREQ, M2_SLOW_LOOP_FREQ) + 2.0F)
#define M2_QDC_SPEED_FILTER_IIR_A1				       (1.0F - M2_QDC_SPEED_FILTER_IIR_B0 - M2_QDC_SPEED_FILTER_IIR_B1)
#define M2_QDC_SPEED_FILTER_IIR_B0_FRAC				   FRAC32(M2_QDC_SPEED_FILTER_IIR_B0/2)
#define M2_QDC_SPEED_FILTER_IIR_B1_FRAC				   FRAC32(M2_QDC_SPEED_FILTER_IIR_B1/2)
#define M2_QDC_SPEED_FILTER_IIR_A1_FRAC				   FRAC32(M2_QDC_SPEED_FILTER_IIR_A1/2)


// Sensorless BEMF DQ Observer 
//-----------------------------------------------------------------------------------------------------------  
#define M2_BEMF_DQ_CLOOP_ATT	(0.85F) 	/* attenuation for DQ observer */
#define M2_BEMF_DQ_CLOOP_FREQ	(1200.0F)//(300.0F)	/* [Hz], oscillating frequency for DQ observer */
#define M2_BEMF_DQ_TO_ATT		(0.85F)		/* attenuation for tracking observer */
#define M2_BEMF_DQ_TO_FREQ		(70.0)		/* [Hz], oscillating frequency for tracking observer */
#define M2_BEMF_DQ_TO_SPEED_CUTOFF_FREQ  	(637.0F)  /* [Hz], cutoff frequency of IIR1 low pass filter which is for the estimated speed */

#define M2_I_SCALE				(M2_LD/(M2_LD+(M2_R/M2_FAST_LOOP_FREQ)))
#define M2_U_SCALE				((1.0F/M2_FAST_LOOP_FREQ)/(M2_LD+(M2_R/M2_FAST_LOOP_FREQ)))
#define M2_E_SCALE				((1.0F/M2_FAST_LOOP_FREQ)/(M2_LD+(M2_R/M2_FAST_LOOP_FREQ)))
#define M2_WI_SCALE				((M2_LQ/M2_FAST_LOOP_FREQ)/(M2_LD+(M2_R/M2_FAST_LOOP_FREQ)))
#define M2_BEMF_DQ_KP_GAIN 		(2.0F*M2_BEMF_DQ_CLOOP_ATT*2*PI*M2_BEMF_DQ_CLOOP_FREQ*M2_LD - M2_R)
#define M2_BEMF_DQ_KI_GAIN 		(2*PI*M2_BEMF_DQ_CLOOP_FREQ * 2*PI*M2_BEMF_DQ_CLOOP_FREQ *M2_LD/M2_FAST_LOOP_FREQ)
#define M2_BEMF_DQ_TO_KP_GAIN			(2.0F*M2_BEMF_DQ_TO_ATT*2*PI*M2_BEMF_DQ_TO_FREQ)
#define M2_BEMF_DQ_TO_KI_GAIN			(2*PI*M2_BEMF_DQ_TO_FREQ * 2*PI*M2_BEMF_DQ_TO_FREQ/M2_FAST_LOOP_FREQ)
#define M2_BEMF_DQ_TO_THETA_GAIN		(1.0F/(PI*M2_FAST_LOOP_FREQ))

#define M2_TO_SPEED_IIR_B0		WARP(M2_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M2_FAST_LOOP_FREQ)/(WARP(M2_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M2_FAST_LOOP_FREQ) + 2.0F)
#define M2_TO_SPEED_IIR_B1		WARP(M2_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M2_FAST_LOOP_FREQ)/(WARP(M2_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M2_FAST_LOOP_FREQ) + 2.0F)
#define M2_TO_SPEED_IIR_A1		(1.0F - M2_TO_SPEED_IIR_B0 - M2_TO_SPEED_IIR_B1)

// OpenLoop startup
//-----------------------------------------------------------------------------------------------------------
#define M2_OL_START_SPEED_RAMP		(1000.0F) /* [RPM/s], (mechanical) speed accelerating rate during startup */
#define M2_OL_START_I 				(0.65F)   /* [A], current amplitude during startup */
#define M2_OL_MERG_SPEED			(400.0F)  /* [RPM], mechanical speed when motor merges from startup to closed-loop */
#define M2_MERG_TIME				(14.0)    /* [ms], merging duration when motor reaches speed  Mx_OL_MERG_SPEED。 Merging coefficient increases from 0 to 1 in this phase */

#define M2_OL_START_SPEED_RAMP_RAD	(2.0F*PI*M2_OL_START_SPEED_RAMP*M2_MOTOR_PP/60.0) /* transfer mechanical speed in RPM to electrical angular speed */
#define M2_OL_START_RAMP_INC		(M2_OL_START_SPEED_RAMP_RAD/M2_FAST_LOOP_FREQ)
#define M2_MERG_SPEED_TRH			(2.0F*PI*M2_OL_MERG_SPEED*M2_MOTOR_PP/60.0)
#define M2_MERG_COEFF 				FRAC16(1000.0/(M2_MERG_TIME * M2_FAST_LOOP_FREQ))
  
// Control Structure Module - Scalar Control 
//-----------------------------------------------------------------------------------------------------------  
#define M2_SCALAR_VHZ_FACTOR_GAIN  	(0.08F)  				 /* [v/Hz], voltage against electrical frequency */
#define M2_SCALAR_FREQ_RAMP			(15.0F)			     	 /* [Hz/s], given frequency(motor electrical speed) increases in this rate */

#define M2_SCALAR_INTEG_GAIN		ACC32(2.0*M2_FREQ_MAX/M2_FAST_LOOP_FREQ) /* Integral gain for position generation. Position_frac += M2_SCALAR_INTEG_GAIN * given_freq_frac */
#define M2_SCALAR_RAMP_UP			(M2_SCALAR_FREQ_RAMP/M2_FAST_LOOP_FREQ)
#define M2_SCALAR_RAMP_DOWN			(M2_SCALAR_FREQ_RAMP/M2_FAST_LOOP_FREQ)


#endif

//End of generated file                 
/**********************************************************************/

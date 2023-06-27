/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
 
#ifndef __M1_PMSM_APPCONFIG_H
#define __M1_PMSM_APPCONFIG_H

#include "trigonometric.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MOTOR1_CMD_FROM_FMSTR 0 // 0: from external source; 1: from freemaster

// HW configurations
//-----------------------------------------------------------------------------------------------------------
/* Configuration table of ADC channels according to the input pin signals
 *
 * Proper ADC channel assignment needs to follow these rules:
 *   - At least one phase current must be assigned to both ADC modules
 *   - Two other phase current channels must be assigned to different ADC modules
 *   - Udcb and auxiliary channels must be assigned to different ADC modules
 */

/*
 *   Each LPADC has 15 command slots, from 1 to 15. Each command slot is associated with a channel number and its side
 *
 *   An example of ADC channel settings:
 *
 *   IA is assigned to ADC1_CH2A
 *   IB is assigned to ADC2_CH3A
 *   IC is assigned to ADC1_CH3B and ADC2_CH3B
 *   Udc is assigned to ADC1_CH2B
 *   Aux is assigned to ADC2_CH3A
 *
 *
 *
 *             Channel_number |  Side   | Command number
 *
 *   ADC1_IA:        2          SIDE_A        1
 *   ADC2_IA:     NOT_EXIST    NOT_EXIST    NOT_EXIST
 *   ADC1_IB:     NOT_EXIST    NOT_EXIST    NOT_EXIST
 *   ADC2_IB:        3          SIDE_A        1
 *   ADC1_IC:        3          SIDE_B        2
 *   ADC2_IC:        3          SIDE_B        2
 *   ADC1_VDC:       2          SIDE_B        3
 *   ADC2_VDC:    NOT_EXIST    NOT_EXIST    NOT_EXIST
 *   ADC1_AUX:    NOT_EXIST    NOT_EXIST    NOT_EXIST
 *   ADC2_AUX:       3          SIDE_A        3
 * */

//                       Channel_number        Side          Command slot number
#define M1_ADC1_IA       {    0,               SIDE_A,           1          }
#define M1_ADC2_IA       {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }
#define M1_ADC1_IB       {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }
#define M1_ADC2_IB       {    0,               SIDE_A,           1          }
#define M1_ADC1_IC       {    3,               SIDE_A,           2          }
#define M1_ADC2_IC       {    3,               SIDE_A,           2          }
#define M1_ADC1_VDC      {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }
#define M1_ADC2_VDC      {    4,               SIDE_B,           3          }
#define M1_ADC1_AUX      {    1,               SIDE_B,           3          }
#define M1_ADC2_AUX      {    NOT_EXIST,       NOT_EXIST,        NOT_EXIST  }

#define M1_ADC_AVG_NUM          2      /* 2^AVG_NUM conversions averaged */
#define M1_ADC_TRIGGER_DELAY 	1.25   /* [us] */
#define M1_ADC_AVERAGE_TIME     1.0225 /* [us], 0.2556us per sample */


/* Assignment of eFlexPWM channels to motor 1 phases
 * 0 - PWM channels A0&B0 - sub-module 0
 * 1 - PWM channels A1&B1 - sub-module 1
 * 2 - PWM channels A2&B2 - sub-module 2
 */
#define M1_PWM_PAIR_PHA (0)
#define M1_PWM_PAIR_PHB (1)
#define M1_PWM_PAIR_PHC (2)

/* Over Current Fault detection */
#define M1_FAULT_NUM (0)


#define M1_M_2310P_LN_04K

//#define M1_M_2311S_LN_08K

// Motor Parameters 
//-----------------------------------------------------------------------------------------------------------  
#define M1_MOTOR_PP 		(4)
#ifdef M1_M_2310P_LN_04K
#define M1_LD				(0.0002)  /* [H], d-axis inductance */
#define M1_LQ				(0.0002)	  /* [H], q-axis inductance */
#define M1_R				(0.36)	  /* [ohm], phase resistance */
#define M1_ENCODER_LINES	(1000)	  	 /* Encoder lines per mechanical revolution */
#endif

#ifdef M1_M_2311S_LN_08K
#define M1_LD				(0.001465)  /* [H], d-axis inductance */
#define M1_LQ				(0.001465)	  /* [H], q-axis inductance */
#define M1_R				(1.38)	  /* [ohm], phase resistance */
#define M1_ENCODER_LINES	(2000)	  	 /* Encoder lines per mechanical revolution */
#endif

#define M1_N_NOM 			(6000.0F) /* [RPM], motor nominal mechanical speed */
#define M1_I_PH_NOM 		(3.0F)    /* [A], motor nominal current */

// Application Scales 
//-----------------------------------------------------------------------------------------------------------  
#define M1_I_MAX 		(16.665F) /* [A], defined by HW, phase current scale */
#define M1_U_DCB_MAX 	(60.08F)  /* [V], defined by HW, DC bus voltage scale */
#define M1_N_MAX 		(6500.0F) /* [RPM], rotor mechanical speed scale */


#define M1_N_NOM_RAD	(2*PI*M1_N_NOM*M1_MOTOR_PP/60.0)	/* [rad/s], electrical nominal angular speed */
#define M1_U_MAX 		(M1_U_DCB_MAX/1.732F)  				/* [V], phase voltage scale */
#define M1_FREQ_MAX 	(M1_MOTOR_PP*M1_N_MAX/60.0F)  		/* [Hz], electrical speed scale */
#define M1_RAD_MAX		(2*PI*M1_N_MAX*M1_MOTOR_PP/60.0F)	/* [rad/s], electrical angular speed scale */
#define M1_SPEED_FRAC_TO_ANGULAR_COEFF  (float_t)(2*PI*M1_N_MAX*M1_MOTOR_PP/60.0) /* A coefficient that converts scaled speed to electrical angular speed in unit of rad/s */
#define M1_SPEED_MECH_RPM_TO_ELEC_ANGULAR_COEFF (float_t)(2*PI*M1_MOTOR_PP/60.0)
#define M1_SPEED_ELEC_ANGULAR_TO_MECH_RPM_COEFF (float_t)(60.0F / (M1_MOTOR_PP * 2.0F * FLOAT_PI))

// Fault thresholds
//-----------------------------------------------------------------------------------------------------------
#define M1_U_DCB_TRIP 			(28.0F)  	/* [V], brake is on when DC bus reaches this voltage */
#define M1_U_DCB_UNDERVOLTAGE 	(19.0F)  	/* [V], DC bus under voltage threshold */
#define M1_U_DCB_OVERVOLTAGE 	(30.8F)  	/* [V], DC bus over voltage threshold */
#define M1_N_OVERSPEED 			(4500.0F) 	/* [RPM], mechanical over speed threshold */
#define M1_N_MIN 				(300.0F)   	/* [RPM], the minimum mechanical speed that is required for speed command when sensorless approach is used */
  
#define M1_N_OVERSPEED_RAD		(2*PI*M1_N_OVERSPEED*M1_MOTOR_PP/60.0) /* [rad/s], electrical angular over speed threshold */
#define M1_N_MIN_RAD			(2*PI*M1_N_MIN*M1_MOTOR_PP/60.0)	   /* [rad/s], electrical angular minimum speed to ensure sensorless observer can work */

// Control loop frequency
//-----------------------------------------------------------------------------------------------------------
#define M1_PWM_FREQ               (16000.0)   /* [Hz], PWM frequency */
#define M1_FOC_FREQ_VS_PWM_FREQ   (1)         /* FOC calculation is called every n-th PWM reload */
#define M1_PWM_DEADTIME           (0.8)       /* [us], Output PWM deadtime value in micro-seconds */
#define M1_FAST_LOOP_FREQ		  (float_t)(M1_PWM_FREQ/M1_FOC_FREQ_VS_PWM_FREQ)  /* [Hz], fast loop control frequency */
#define M1_SLOW_LOOP_FREQ		  (8000.0F)   /* [Hz], slow loop control frequency */

// DC bus voltage filter
//-----------------------------------------------------------------------------------------------------------
#define M1_UDCB_CUTOFF_FREQ  	(160.0F)   /* [Hz], cutoff frequency of IIR1 low pass filter */

#define M1_UDCB_IIR_B0			WARP(M1_UDCB_CUTOFF_FREQ, M1_FAST_LOOP_FREQ)/(WARP(M1_UDCB_CUTOFF_FREQ, M1_FAST_LOOP_FREQ) + 2.0F)
#define M1_UDCB_IIR_B1			WARP(M1_UDCB_CUTOFF_FREQ, M1_FAST_LOOP_FREQ)/(WARP(M1_UDCB_CUTOFF_FREQ, M1_FAST_LOOP_FREQ) + 2.0F)
#define M1_UDCB_IIR_A1			(1.0F - M1_UDCB_IIR_B0 - M1_UDCB_IIR_B1)

  
// Mechanical alignment 
//-----------------------------------------------------------------------------------------------------------  
#define M1_ALIGN_VOLTAGE 		(1.0F)	/* [v], alignment voltage vector length */
#define M1_ALIGN_DURATION_TIME	(0.5F)  /* [s], alignment stage duration */
#define M1_BOOTSTRAP_CHARGE_TIME (0.05) /* [s], charging bootstrap capacitor duration, must be smaller than alignment duration */
#define M1_BOOTSTRAP_DUTY       (0.1)   /* [n/a], duty used in bootstrap capacitor charging */
#define M1_ALIGN_DURATION		(uint16_t)(M1_ALIGN_DURATION_TIME * M1_FAST_LOOP_FREQ)
#define M1_BOOTSTRAP_CHARGE_DURATION   (uint16_t)(M1_BOOTSTRAP_CHARGE_TIME * M1_FAST_LOOP_FREQ)

  
// Application counters 
//-----------------------------------------------------------------------------------------------------------
#define M1_CALIB_DURATION_TIME		(0.5F) /* [s], phase currents calibration stage duration */
#define M1_FAULT_DURATION_TIME  	(6.0F) /* [s], Fault state duration after all faults disappear  */
#define M1_FREEWHEEL_DURATION_TIME 	(1.5F) /* [s], freewheel state duration after motor is stopped from running */

#define M1_CALIB_DURATION			(uint16_t)(M1_CALIB_DURATION_TIME * M1_SLOW_LOOP_FREQ)
#define M1_FAULT_DURATION			(uint16_t)(M1_FAULT_DURATION_TIME * M1_SLOW_LOOP_FREQ)
#define M1_FREEWHEEL_DURATION		(uint16_t)(M1_FREEWHEEL_DURATION_TIME * M1_SLOW_LOOP_FREQ)

  
// Miscellaneous 
//-----------------------------------------------------------------------------------------------------------  
#define M1_E_BLOCK_TRH 			(1.4F)  /* [v], estimated bemf threshold on Q-axis. */
#define M1_E_BLOCK_PER 			(2000)  /* Motor is deemed as blocked when bemf on Q-axis is less than the threshold for this number of times in fast loop  */
#define M1_BLOCK_ROT_FAULT_SH   (1.0F/5) /* filter window */

//Current Loop Control - compared with 2rd order system
//----------------------------------------------------------------------
#define M1_CLOOP_ATT			(0.707F) 	/* attenuation */
#ifdef M1_M_2310P_LN_04K
#define M1_CLOOP_FREQ			(2000.0F)	/* [Hz], oscillating frequency */
#endif

#ifdef M1_M_2311S_LN_08K
#define M1_CLOOP_FREQ			(1200.0F)	/* [Hz], oscillating frequency */
#endif
//Current Controller Output Limit       
#define M1_CLOOP_LIMIT                     (0.95) /* Voltage output limitation, based on real time available maximum phase voltage amplitude */

//D-axis Controller - Parallel type     
#define M1_D_KP_GAIN                       (2*M1_CLOOP_ATT*2*PI*M1_CLOOP_FREQ*M1_LD - M1_R)
#define M1_D_KI_GAIN                       (2*PI*M1_CLOOP_FREQ*2*PI*M1_CLOOP_FREQ*M1_LD/M1_FAST_LOOP_FREQ)
//Q-axis Controller - Parallel type     
#define M1_Q_KP_GAIN                       (2*M1_CLOOP_ATT*2*PI*M1_CLOOP_FREQ*M1_LQ - M1_R)
#define M1_Q_KI_GAIN                       (2*PI*M1_CLOOP_FREQ*2*PI*M1_CLOOP_FREQ*M1_LQ/M1_FAST_LOOP_FREQ)


//Speed Loop Control                    
//----------------------------------------------------------------------
//Speed Controller - Parallel type      
#define M1_SPEED_PI_PROP_GAIN              (0.025F)//(0.0197F)		/* proportional gain */
#define M1_SPEED_PI_INTEG_GAIN             (0.0002F)//(0.00065685F)	/* integral gain */
#define M1_SPEED_PI_PROP_SENSORLESS_GAIN              (0.005F)		/* proportional gain */
#define M1_SPEED_PI_INTEG_SENSORLESS_GAIN             (0.00002F)	/* integral gain */
#define M1_SPEED_PI_DESAT_GAIN			   (0.5F)
#define M1_SPEED_LOOP_HIGH_LIMIT           (3.0F)			/* [A], current output upper limitation */
#define M1_SPEED_LOOP_LOW_LIMIT            (-3.0F)			/* [A], current output lower limitation */
#define M1_CL_SPEED_RAMP			   	   (8000.0F) 		/* [RPM/s], (mechanical) speed accelerating rate during closed-loop */
#define M1_SPEED_CUTOFF_FREQ    		   (100.0F)          /* [Hz], cutoff frequency of IIR1 low pass filter for speed from sensor or observer */
#define M1_SPEED_LOOP_IQ_FWD_GAIN 		   (0.5F)		    /* [A], Iq feed forward gain */

#define M1_CUR_FWD_CUTOFF_FREQ  		   (160.0F)   /* [Hz], cutoff frequency of IIR1 low pass filter */

#define M1_CUR_FWD_IIR_B0				   WARP(M1_CUR_FWD_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ)/(WARP(M1_CUR_FWD_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ) + 2.0F)
#define M1_CUR_FWD_IIR_B1				   WARP(M1_CUR_FWD_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ)/(WARP(M1_CUR_FWD_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ) + 2.0F)
#define M1_CUR_FWD_IIR_A1				   (1.0F - M1_CUR_FWD_IIR_B0 - M1_CUR_FWD_IIR_B1)

#define M1_CL_SPEED_RAMP_RAD			   (2.0F*PI*M1_CL_SPEED_RAMP*M1_MOTOR_PP/60.0) /* transfer mechanical speed in RPM to electrical angular speed */
#define M1_SPEED_RAMP_UP  				   (M1_CL_SPEED_RAMP_RAD/M1_SLOW_LOOP_FREQ)
#define M1_SPEED_RAMP_DOWN  			   (M1_CL_SPEED_RAMP_RAD/M1_SLOW_LOOP_FREQ)

#define M1_SPEED_IIR_B0				   	   WARP(M1_SPEED_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ)/(WARP(M1_SPEED_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ) + 2.0F)
#define M1_SPEED_IIR_B1				       WARP(M1_SPEED_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ)/(WARP(M1_SPEED_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ) + 2.0F)
#define M1_SPEED_IIR_A1				       (1.0F - M1_SPEED_IIR_B0 - M1_SPEED_IIR_B1)


// Position loop control
//-----------------------------------------------------------------------------------------------------------
#define M1_QDC_TRAJECTORY_FILTER_FREQ		4.0 		 /* [Hz], cutoff frequency of a 2nd order IIR filter to get a smoothed position reference */
#define M1_QDC_POSITION_RAMP           		20.0 		 /* [Revolutions/s], position ramping rate for a position command */
#define M1_QDC_POSITION_CTRL_P_GAIN			4.0			 /* proportional gain for position controller */
#define M1_QDC_POSITION_CTRL_LIMIT	   		4000.0F		 /* [RPM], mechanical speed - position controller output upper limit. Lower limit is its negative value */
#define M1_QDC_POSITION_CTRL_SPEED_FWD_GAIN 1.0F		 /* Speed feed forward gain */


#define M1_QDC_POSITION_RAMP_UP_FRAC	 	((M1_QDC_POSITION_RAMP/M1_SLOW_LOOP_FREQ)*65535)  /* Q16.16 format */
#define M1_QDC_POSITION_RAMP_DOWN_FRAC	 	((M1_QDC_POSITION_RAMP/M1_SLOW_LOOP_FREQ)*65535)  /* Q16.16 format */
#define M1_QDC_POSITION_CTRL_LIMIT_FRAC		FRAC16(M1_QDC_POSITION_CTRL_LIMIT/M1_N_MAX)
#define M1_QDC_TRAJECTORY_FILTER_FREQ_FRAC  FRAC32(2*PI*M1_QDC_TRAJECTORY_FILTER_FREQ/M1_SLOW_LOOP_FREQ)
#define M1_QDC_POSITION_CTRL_P_GAIN_FRAC	ACC32(M1_QDC_POSITION_CTRL_P_GAIN)

/*
 *  SOS Matrix:
1  -1.8417723178863525390625  1  1  -1.7986924648284912109375   0.97689568996429443359375
1  -1.8417723178863525390625  1  1  -1.84223783016204833984375  0.9795920848846435546875
1  -1.8417723178863525390625  1  1  -1.78430569171905517578125  0.946927726268768310546875
1  -1.8417723178863525390625  1  1  -1.8034856319427490234375   0.94955456256866455078125

Scale Values:
0.25994360446929931640625
1.082401275634765625
1.1156253814697265625
1.041685581207275390625
2.836890697479248046875
 *
 *
 * */
#define SOS0_B1 	-1.8417723178863525390625F
#define SOS0_B2 	1.0F
#define SOS0_B0 	1.0F
#define SOS0_A1 	-1.7986924648284912109375F
#define SOS0_A2 	0.97689568996429443359375F
#define SOS0_SCALE 	0.25994360446929931640625F

#define SOS1_B1 	-1.8417723178863525390625F
#define SOS1_B2 	1.0F
#define SOS1_B0 	1.0F
#define SOS1_A1 	-1.84223783016204833984375F
#define SOS1_A2 	0.9795920848846435546875F
#define SOS1_SCALE 	1.082401275634765625F

#define SOS2_B1 	-1.8417723178863525390625F
#define SOS2_B2 	1.0F
#define SOS2_B0 	1.0F
#define SOS2_A1 	-1.78430569171905517578125F
#define SOS2_A2 	0.946927726268768310546875F
#define SOS2_SCALE 	1.1156253814697265625F

#define SOS3_B1 	-1.8417723178863525390625F
#define SOS3_B2 	1.0F
#define SOS3_B0 	1.0F
#define SOS3_A1 	-1.8034856319427490234375F
#define SOS3_A2 	0.94955456256866455078125F
#define SOS3_SCALE 	1.041685581207275390625F

#define OUTPUT_SCALE 2.836890697479248046875F
  
// Position & Speed Sensors Module 
//-----------------------------------------------------------------------------------------------------------  

#define M1_QDC_TIMER_PRESCALER				6 //10			 /* Prescaler for the timer within QDC, the prescaling value is 2^Mx_QDC_TIMER_PRESCALER */
#define M1_QDC_CLOCK	 					240000000	 /* [Hz], QDC module clock, which is the bus clock of the system */
#define M1_QDC_SPEED_FILTER_CUTOFF_FREQ 	100.0 		 /* [Hz], cutoff frequency of IIR1 low pass filter for calculated raw speed out of QDC HW feature */
#define M1_QDC_TO_ATT						(0.85F)		 /* attenuation for tracking observer, which is to estimate rotor speed from real QDC position */
#define M1_QDC_TO_FREQ						(300.0)		 /* [Hz], oscillating frequency for tracking observer */

#define M1_QDC_TIMER_FREQUENCY  			(M1_QDC_CLOCK/EXPON(2,M1_QDC_TIMER_PRESCALER)) 			/* [Hz], the clock frequency for the timer within QDC */
#define M1_SPEED_CAL_CONST  				((60.0*M1_QDC_TIMER_FREQUENCY/(4*M1_ENCODER_LINES*M1_N_MAX)) * 134217728)	/* A constant to calculate mechanical speed out of QDC HW feature, Q5.27 */
#define M1_QDC_TO_KP_GAIN					(2.0F*M1_QDC_TO_ATT*2*PI*M1_QDC_TO_FREQ)
#define M1_QDC_TO_KI_GAIN					(2*PI*M1_QDC_TO_FREQ * 2*PI*M1_QDC_TO_FREQ/M1_FAST_LOOP_FREQ)
#define M1_QDC_TO_THETA_GAIN				(1.0F/(PI*M1_FAST_LOOP_FREQ))


#define M1_QDC_SPEED_FILTER_IIR_B0				   	   WARP(M1_QDC_SPEED_FILTER_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ)/(WARP(M1_QDC_SPEED_FILTER_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ) + 2.0F)
#define M1_QDC_SPEED_FILTER_IIR_B1				       WARP(M1_QDC_SPEED_FILTER_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ)/(WARP(M1_QDC_SPEED_FILTER_CUTOFF_FREQ, M1_SLOW_LOOP_FREQ) + 2.0F)
#define M1_QDC_SPEED_FILTER_IIR_A1				       (1.0F - M1_QDC_SPEED_FILTER_IIR_B0 - M1_QDC_SPEED_FILTER_IIR_B1)
#define M1_QDC_SPEED_FILTER_IIR_B0_FRAC				   FRAC32(M1_QDC_SPEED_FILTER_IIR_B0/2)
#define M1_QDC_SPEED_FILTER_IIR_B1_FRAC				   FRAC32(M1_QDC_SPEED_FILTER_IIR_B1/2)
#define M1_QDC_SPEED_FILTER_IIR_A1_FRAC				   FRAC32(M1_QDC_SPEED_FILTER_IIR_A1/2)


// Sensorless BEMF DQ Observer 
//-----------------------------------------------------------------------------------------------------------  
#define M1_BEMF_DQ_CLOOP_ATT	(0.85F) 	/* attenuation for DQ observer */
#define M1_BEMF_DQ_CLOOP_FREQ	(300.0F)	/* [Hz], oscillating frequency for DQ observer */
#define M1_BEMF_DQ_TO_ATT		(0.85F)		/* attenuation for tracking observer */
#define M1_BEMF_DQ_TO_FREQ		(70.0)		/* [Hz], oscillating frequency for tracking observer */
#define M1_BEMF_DQ_TO_SPEED_CUTOFF_FREQ  	(637.0F)  /* [Hz], cutoff frequency of IIR1 low pass filter which is for the estimated speed */

#define M1_I_SCALE				(M1_LD/(M1_LD+(M1_R/M1_FAST_LOOP_FREQ)))
#define M1_U_SCALE				((1.0F/M1_FAST_LOOP_FREQ)/(M1_LD+(M1_R/M1_FAST_LOOP_FREQ)))
#define M1_E_SCALE				((1.0F/M1_FAST_LOOP_FREQ)/(M1_LD+(M1_R/M1_FAST_LOOP_FREQ)))
#define M1_WI_SCALE				((M1_LQ/M1_FAST_LOOP_FREQ)/(M1_LD+(M1_R/M1_FAST_LOOP_FREQ)))
#define M1_BEMF_DQ_KP_GAIN 		(2.0F*M1_BEMF_DQ_CLOOP_ATT*2*PI*M1_BEMF_DQ_CLOOP_FREQ*M1_LD - M1_R)
#define M1_BEMF_DQ_KI_GAIN 		(2*PI*M1_BEMF_DQ_CLOOP_FREQ * 2*PI*M1_BEMF_DQ_CLOOP_FREQ *M1_LD/M1_FAST_LOOP_FREQ)
#define M1_BEMF_DQ_TO_KP_GAIN			(2.0F*M1_BEMF_DQ_TO_ATT*2*PI*M1_BEMF_DQ_TO_FREQ)
#define M1_BEMF_DQ_TO_KI_GAIN			(2*PI*M1_BEMF_DQ_TO_FREQ * 2*PI*M1_BEMF_DQ_TO_FREQ/M1_FAST_LOOP_FREQ)
#define M1_BEMF_DQ_TO_THETA_GAIN		(1.0F/(PI*M1_FAST_LOOP_FREQ))

#define M1_TO_SPEED_IIR_B0		WARP(M1_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M1_FAST_LOOP_FREQ)/(WARP(M1_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M1_FAST_LOOP_FREQ) + 2.0F)
#define M1_TO_SPEED_IIR_B1		WARP(M1_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M1_FAST_LOOP_FREQ)/(WARP(M1_BEMF_DQ_TO_SPEED_CUTOFF_FREQ, M1_FAST_LOOP_FREQ) + 2.0F)
#define M1_TO_SPEED_IIR_A1		(1.0F - M1_TO_SPEED_IIR_B0 - M1_TO_SPEED_IIR_B1)

// OpenLoop startup
//-----------------------------------------------------------------------------------------------------------
#define M1_OL_START_SPEED_RAMP		(1000.0F) /* [RPM/s], (mechanical) speed accelerating rate during startup */
#define M1_OL_START_I 				(0.65F)   /* [A], current amplitude during startup */
#define M1_OL_MERG_SPEED			(400.0F)  /* [RPM], mechanical speed when motor merges from startup to closed-loop */
#define M1_MERG_TIME				(14.0)    /* [ms], merging duration when motor reaches speed  Mx_OL_MERG_SPEED。 Merging coefficient increases from 0 to 1 in this phase */

#define M1_OL_START_SPEED_RAMP_RAD	(2.0F*PI*M1_OL_START_SPEED_RAMP*M1_MOTOR_PP/60.0) /* transfer mechanical speed in RPM to electrical angular speed */
#define M1_OL_START_RAMP_INC		(M1_OL_START_SPEED_RAMP_RAD/M1_FAST_LOOP_FREQ)
#define M1_MERG_SPEED_TRH			(2.0F*PI*M1_OL_MERG_SPEED*M1_MOTOR_PP/60.0)
#define M1_MERG_COEFF 				FRAC16(1000.0/(M1_MERG_TIME * M1_FAST_LOOP_FREQ))
  
// Control Structure Module - Scalar Control 
//-----------------------------------------------------------------------------------------------------------  
#define M1_SCALAR_VHZ_FACTOR_GAIN  	(0.08F)  				 /* [v/Hz], voltage against electrical frequency */
#define M1_SCALAR_FREQ_RAMP			(15.0F)			     	 /* [Hz/s], given frequency(motor electrical speed) increases in this rate */

#define M1_SCALAR_INTEG_GAIN		ACC32(2.0*M1_FREQ_MAX/M1_FAST_LOOP_FREQ) /* Integral gain for position generation. Position_frac += M1_SCALAR_INTEG_GAIN * given_freq_frac */
#define M1_SCALAR_RAMP_UP			(M1_SCALAR_FREQ_RAMP/M1_FAST_LOOP_FREQ)
#define M1_SCALAR_RAMP_DOWN			(M1_SCALAR_FREQ_RAMP/M1_FAST_LOOP_FREQ)


#endif

/**********************************************************************/

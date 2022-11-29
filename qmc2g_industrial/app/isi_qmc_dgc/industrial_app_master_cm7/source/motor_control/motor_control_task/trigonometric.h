/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef MOTOR_CONTROL_TASK_TEKNIC_TRIGONOMETRIC_H_
#define MOTOR_CONTROL_TASK_TEKNIC_TRIGONOMETRIC_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define PI 3.1415926
#define EXPON3(x)	(x*x*x)
#define EXPON5(x)	(x*x*x*x*x)
#define EXPON7(x)	(x*x*x*x*x*x*x)
#define EXPON9(x)	(x*x*x*x*x*x*x*x*x)
#define EXPON11(x)	(x*x*x*x*x*x*x*x*x*x*x)

#define EXPON2(x)	(x*x)
#define EXPON4(x)	(x*x*x*x)
#define EXPON6(x)	(x*x*x*x*x*x)
#define EXPON8(x)	(x*x*x*x*x*x*x*x)
#define EXPON10(x)	(x*x*x*x*x*x*x*x*x*x)
#define EXPON12(x)	(x*x*x*x*x*x*x*x*x*x*x*x)
#define EXPON13(x)	(x*x*x*x*x*x*x*x*x*x*x*x*x)
#define EXPON14(x)	(x*x*x*x*x*x*x*x*x*x*x*x*x*x)
#define EXPON15(x)	(x*x*x*x*x*x*x*x*x*x*x*x*x*x*x)
#define EXPON(x,y)  EXPON_intermediate(x,y)
#define EXPON_intermediate(x,y)  EXPON##y(x)  // x^y

#define TAN(x) 		(x+EXPON3(x)/3.0F+2*EXPON5(x)/15.0F+17*EXPON7(x)/315.0F+62*EXPON9(x)/2835.0F+1382*EXPON11(x)/155925.0F)

#define WARP(x,y)		(2*TAN(2*PI*x/(2*y))) // x is cutoff frequency in Hz. y is fast loop frequency in Hz. (y/x) must be larger than 2

#endif /* MOTOR_CONTROL_TASK_TEKNIC_TRIGONOMETRIC_H_ */

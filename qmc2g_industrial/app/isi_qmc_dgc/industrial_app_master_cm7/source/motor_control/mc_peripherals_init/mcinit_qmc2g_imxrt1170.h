/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef MCINIT_QMC2G_IMXRT1170_H_
#define MCINIT_QMC2G_IMXRT1170_H_

#include <mc_hal_drivers/mcdrv_adc_imxrt117x.h>
#include <mc_hal_drivers/mcdrv_pwm3ph_pwma_imxrt117x.h>
#include <mc_hal_drivers/mcdrv_qdc_imxrt117x.h>
//#include "mcdrv_gd3000.h"
#include "fsl_gpio.h"
#include "mlib.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "m1_pmsm_appconfig.h"
#include "m2_pmsm_appconfig.h"
#include "m3_pmsm_appconfig.h"
#include "m4_pmsm_appconfig.h"

//typedef enum
//{
//	GD3000_ON_PSB = 0,
//	AFE_ON_PSB = 1,
//	ABS_ENC_ON_PSB = 2,
//	AFE_ON_DB = 3
//}spi_selection_t;

// SPI setting for GD3000
#define SPI_DATA_WIDTH 8
#define SPI_BAUDRATE   2000000  // [Hz]

#define OVERDRIVE_MODE

/* Version info */
#define MCRSP_VER       "0.0.1"        /* motor control package version */

/* Application info */
typedef struct _app_ver
{
    char    cBoardID[15];
    char    cMotorType[4];
    char    cAppVer[5];
}app_ver_t;



#define MASK_BIT15 0x8000
#define MASK_BIT14 0x4000
#define MASK_BIT13 0x2000
#define MASK_BIT12 0x1000
#define MASK_BIT11 0x0800
#define MASK_BIT10 0x0400
#define MASK_BIT9  0x0200
#define MASK_BIT8  0x0100
#define MASK_BIT7  0x0080
#define MASK_BIT6  0x0040
#define MASK_BIT5  0x0020
#define MASK_BIT4  0x0010
#define MASK_BIT3  0x0008
#define MASK_BIT2  0x0004
#define MASK_BIT1  0x0002
#define MASK_BIT0  0x0001

#define SIDE_A 	   0
#define SIDE_B 	   1
#define MOTOR_1	   1
#define MOTOR_2	   2
#define MOTOR_3	   3
#define MOTOR_4	   4


/******************************************************************************
 * Clock & PWM definition for motor 1
 ******************************************************************************/
#define M1_FAST_LOOP_TS           ((float_t)1.0/(float_t)(M1_FAST_LOOP_FREQ))
#define M1_SLOW_LOOP_TS           ((float_t)1.0/(float_t)(M1_SLOW_LOOP_FREQ))
#define M1_TIME_ONESEC_COUNT      (uint16_t)(M1_FAST_LOOP_FREQ)



/******************************************************************************
 * Clock & PWM definition for motor 2
 ******************************************************************************/
#define M2_FAST_LOOP_TS           ((float_t)1.0/(float_t)(M2_FAST_LOOP_FREQ))
#define M2_SLOW_LOOP_TS           ((float_t)1.0/(float_t)(M2_SLOW_LOOP_FREQ))
#define M2_TIME_ONESEC_COUNT      (uint16_t)(M2_FAST_LOOP_FREQ)



/******************************************************************************
 * Clock & PWM definition for motor 3
 ******************************************************************************/
#define M3_FAST_LOOP_TS           ((float_t)1.0/(float_t)(M3_FAST_LOOP_FREQ))
#define M3_SLOW_LOOP_TS           ((float_t)1.0/(float_t)(M3_SLOW_LOOP_FREQ))
#define M3_TIME_ONESEC_COUNT      (uint16_t)(M3_FAST_LOOP_FREQ)



/******************************************************************************
 * Clock & PWM definition for motor 4
 ******************************************************************************/
#define M4_FAST_LOOP_TS           ((float_t)1.0/(float_t)(M4_FAST_LOOP_FREQ))
#define M4_SLOW_LOOP_TS           ((float_t)1.0/(float_t)(M4_SLOW_LOOP_FREQ))
#define M4_TIME_ONESEC_COUNT      (uint16_t)(M4_FAST_LOOP_FREQ)




#define M1_FASTLOOP_TIMER_ENABLE() PWM_SM0123_RUN(&g_sM1Pwm3ph)
#define M2_FASTLOOP_TIMER_ENABLE() PWM_SM012_RUN(&g_sM2Pwm3ph)
#define M3_FASTLOOP_TIMER_ENABLE() PWM_SM012_RUN(&g_sM3Pwm3ph)
#define M4_FASTLOOP_TIMER_ENABLE() PWM_SM012_RUN(&g_sM4Pwm3ph)
#define M1_SLOWLOOP_TIMER_ENABLE() PIT1->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK
#define M2_SLOWLOOP_TIMER_ENABLE() PIT2->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK
#define M3_SLOWLOOP_TIMER_ENABLE() TMR1->CHANNEL[0].CTRL |= TMR_CTRL_CM(1)
#define M4_SLOWLOOP_TIMER_ENABLE() TMR2->CHANNEL[0].CTRL |= TMR_CTRL_CM(1)
#define RESET_TIMER1()             TMR1->CHANNEL[1].CNTR = 0
#define RESET_TIMER2()             TMR1->CHANNEL[2].CNTR = 0
#define RESET_TIMER3()             TMR1->CHANNEL[3].CNTR = 0
#define START_TIMER1()             TMR1->CHANNEL[1].CTRL |= TMR_CTRL_CM(1)
#define START_TIMER2()             TMR1->CHANNEL[2].CTRL |= TMR_CTRL_CM(1)
#define START_TIMER3()             TMR1->CHANNEL[3].CTRL |= TMR_CTRL_CM(1)
#define READ_TIMER1()              TMR1->CHANNEL[1].CNTR
#define READ_TIMER2()              TMR1->CHANNEL[2].CNTR
#define READ_TIMER3()              TMR1->CHANNEL[3].CNTR
//-------------------------------------------------------------------------------
extern void peripherals_manual_init(void);
//extern void SPI_device_select(spi_selection_t eChoice);
extern void adc_etc_init(void);

extern qdc_block_t g_sM1QdcSensor,g_sM2QdcSensor,g_sM3QdcSensor,g_sM4QdcSensor;
extern mcdrv_adc_t g_sM1AdcSensor,g_sM2AdcSensor,g_sM3AdcSensor,g_sM4AdcSensor;
extern mcdrv_pwm3ph_pwma_t g_sM1Pwm3ph,g_sM2Pwm3ph,g_sM3Pwm3ph,g_sM4Pwm3ph;

//extern GD3000_T g_sM1GD3000, g_sM2GD3000, g_sM3GD3000, g_sM4GD3000;

#endif /* MCINIT_QMC2G_IMXRT1170_H_ */

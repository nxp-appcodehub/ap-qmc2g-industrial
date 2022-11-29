/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef MOTOR_CONTROL_TASK_MOTOR_CONTROL_TASK_H_
#define MOTOR_CONTROL_TASK_MOTOR_CONTROL_TASK_H_

#include "qmc_features_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define M1_fastloop_handler    ADC_ETC_IRQ0_IRQHandler
#define M1_slowloop_handler    PIT1_IRQHandler
#define M2_fastloop_handler    ADC_ETC_IRQ1_IRQHandler
#define M2_slowloop_handler    PIT2_IRQHandler
#define M3_fastloop_handler    ADC_ETC_IRQ2_IRQHandler
#define M3_slowloop_handler    TMR1_IRQHandler
#define M4_fastloop_handler    ADC_ETC_IRQ3_IRQHandler
#define M4_slowloop_handler    TMR2_IRQHandler

#define M1_fastloop_irq		   ADC_ETC_IRQ0_IRQn
#define M1_slowloop_irq    	   PIT1_IRQn
#define M2_fastloop_irq        ADC_ETC_IRQ1_IRQn
#define M2_slowloop_irq        PIT2_IRQn
#define M3_fastloop_irq        ADC_ETC_IRQ2_IRQn
#define M3_slowloop_irq        TMR1_IRQn
#define M4_fastloop_irq        ADC_ETC_IRQ3_IRQn
#define M4_slowloop_irq        TMR2_IRQn

/*******************************************************************************
 * API
 ******************************************************************************/

extern void M1_fastloop_handler(void);
extern void M2_fastloop_handler(void);
extern void M3_fastloop_handler(void);
extern void M4_fastloop_handler(void);
extern void M1_slowloop_handler(void);
extern void M2_slowloop_handler(void);
extern void M3_slowloop_handler(void);
extern void M4_slowloop_handler(void);

#ifdef FEATURE_GET_MOTOR_STATUS_FROM_DATA_HUB
extern void getMotorStatusTask(void *pvParameters); // for test purpose
#endif

#endif /* MOTOR_CONTROL_TASK_MOTOR_CONTROL_TASK_H_ */

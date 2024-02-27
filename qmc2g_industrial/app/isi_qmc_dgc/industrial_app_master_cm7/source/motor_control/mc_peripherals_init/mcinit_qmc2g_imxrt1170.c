/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <mcinit_qmc2g_imxrt1170.h>
#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "FreeRTOSConfig.h"

static channel_table_t sChannelTableM1 = {M1_ADC1_IA, M1_ADC2_IA, M1_ADC1_IB, M1_ADC2_IB, M1_ADC1_IC, M1_ADC2_IC, M1_ADC1_VDC, M1_ADC2_VDC, M1_ADC1_AUX, M1_ADC2_AUX};
static channel_table_t sChannelTableM2 = {M2_ADC1_IA, M2_ADC2_IA, M2_ADC1_IB, M2_ADC2_IB, M2_ADC1_IC, M2_ADC2_IC, M2_ADC1_VDC, M2_ADC2_VDC, M2_ADC1_AUX, M2_ADC2_AUX};
static channel_table_t sChannelTableM3 = {M3_ADC1_IA, M3_ADC2_IA, M3_ADC1_IB, M3_ADC2_IB, M3_ADC1_IC, M3_ADC2_IC, M3_ADC1_VDC, M3_ADC2_VDC, M3_ADC1_AUX, M3_ADC2_AUX};
static channel_table_t sChannelTableM4 = {M4_ADC1_IA, M4_ADC2_IA, M4_ADC1_IB, M4_ADC2_IB, M4_ADC1_IC, M4_ADC2_IC, M4_ADC1_VDC, M4_ADC2_VDC, M4_ADC1_AUX, M4_ADC2_AUX};
qdc_block_t g_sM1QdcSensor,g_sM2QdcSensor,g_sM3QdcSensor,g_sM4QdcSensor;
mcdrv_adc_t g_sM1AdcSensor,g_sM2AdcSensor,g_sM3AdcSensor,g_sM4AdcSensor;
mcdrv_pwm3ph_pwma_t g_sM1Pwm3ph,g_sM2Pwm3ph,g_sM3Pwm3ph,g_sM4Pwm3ph;

/*******************************************************************************
 * Code
 ******************************************************************************/



/*!
 * @brief eFlexPWM1 initialization which controls Motor1. Synchronization with eFlexPWM2~4 and ADC_ETC are implemented here as well. Hal driver is initialized in the end.
 *
 * @param void  No input parameter
 *
 * @return None
 */



static void eFlexPWM1_init(void)
{
	int16_t  i16Tmp;

	CLOCK_EnableClock(kCLOCK_Pwm1);

	/* Sub-module counter sync and buffered registers reload setting */
	PWM1->SM[0].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM1->SM[0].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM1->SM[0].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM1->SM[0].CTRL2 = (PWM1->SM[0].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM1->SM[0].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM1->SM[0].CTRL2 = (PWM1->SM[0].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(0); // use local sync for SM0
	PWM1->SM[0].CTRL2 &= ~PWM_CTRL2_RELOAD_SEL_MASK; // use local reload signal

	PWM1->SM[1].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM1->SM[1].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM1->SM[1].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM1->SM[1].CTRL2 = (PWM1->SM[1].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(2);   // SM0's clock is used
	PWM1->SM[1].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM1->SM[1].CTRL2 = (PWM1->SM[1].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(2); // use master sync from SM0
	PWM1->SM[1].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	PWM1->SM[2].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM1->SM[2].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM1->SM[2].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM1->SM[2].CTRL2 = (PWM1->SM[2].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(2);   // SM0's clock is used
	PWM1->SM[2].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM1->SM[2].CTRL2 = (PWM1->SM[2].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(2); // use master sync from SM0
	PWM1->SM[2].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	PWM1->SM[3].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM1->SM[3].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM1->SM[3].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM1->SM[3].CTRL2 = (PWM1->SM[3].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(2);   // SM0's clock is used
	PWM1->SM[3].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM1->SM[3].CTRL2 = (PWM1->SM[3].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(2); // use master sync from SM0
	PWM1->SM[3].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	/* PWM frequency and duty setting */
#ifdef OVERDRIVE_MODE
	PWM1->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M1_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM1->SM[0].VAL1  = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M1_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#else
	PWM1->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M1_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM1->SM[0].VAL1  = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M1_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#endif
	PWM1->SM[0].VAL0 = 0;
	PWM1->SM[0].VAL2  = (int16_t)(PWM1->SM[0].INIT)/2; // 50% duty
	PWM1->SM[0].VAL3  = PWM1->SM[0].VAL1/2; // 50% duty

	PWM1->SM[1].INIT  = PWM1->SM[0].INIT;
	PWM1->SM[1].VAL1  = PWM1->SM[0].VAL1;
	PWM1->SM[1].VAL0 = 0;
	PWM1->SM[1].VAL2  = PWM1->SM[0].VAL2;
	PWM1->SM[1].VAL3  = PWM1->SM[0].VAL3;

	PWM1->SM[2].INIT  = PWM1->SM[0].INIT;
	PWM1->SM[2].VAL1  = PWM1->SM[0].VAL1;
	PWM1->SM[2].VAL0 = 0;
	PWM1->SM[2].VAL2  = PWM1->SM[0].VAL2;
	PWM1->SM[2].VAL3  = PWM1->SM[0].VAL3;

	PWM1->SM[3].INIT  = PWM1->SM[0].INIT;
	PWM1->SM[3].VAL1  = PWM1->SM[0].VAL1;
	PWM1->SM[3].VAL0 = 0;

	/* Deadtime setting */
#ifdef OVERDRIVE_MODE
	PWM1->SM[0].DTCNT0 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[0].DTCNT1 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[1].DTCNT0 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[1].DTCNT1 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[2].DTCNT0 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[2].DTCNT1 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
#else
	PWM1->SM[0].DTCNT0 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[0].DTCNT1 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[1].DTCNT0 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[1].DTCNT1 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[2].DTCNT0 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM1->SM[2].DTCNT1 = M1_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
#endif

	/* PWM2, PWM3 and PWM4 synchronization */
	PWM1->SM[3].VAL4 = (int16_t)(PWM1->SM[3].INIT)/2;  // 1/4 PWM period
	PWM1->SM[3].VAL5 = 0;                   // 1/2 PWM period
	PWM1->SM[0].VAL4 = PWM1->SM[0].VAL1/2;  // 3/4 PWM period
	PWM1->SM[0].TCTRL = 0;
	PWM1->SM[1].TCTRL = 0;
	PWM1->SM[2].TCTRL = 0;
	PWM1->SM[3].TCTRL = 0;
	PWM1->SM[3].TCTRL |= PWM_TCTRL_OUT_TRIG_EN(0x30); // Use VAL4, VAL5 of SM3 for trigger point - synchronize PWM2, PWM3
	PWM1->SM[0].TCTRL |= PWM_TCTRL_OUT_TRIG_EN(0x10); // Use VAL4 of SM0 for trigger point - synchronize PWM4

	/* ADC_ETC triggering point setting */
#ifdef OVERDRIVE_MODE
	i16Tmp = (int16_t)(((float_t)M1_ADC_TRIGGER_DELAY - M1_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000);
#else
	i16Tmp = (int16_t)(((float_t)M1_ADC_TRIGGER_DELAY - M1_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000);
#endif
	if(i16Tmp >= 0) // Setup ADC triggering point
	{
		PWM1->SM[0].VAL5 = PWM1->SM[0].INIT + i16Tmp;
	}
	else
	{
		PWM1->SM[0].VAL5 = PWM1->SM[0].VAL1 + i16Tmp;
	}
	PWM1->SM[0].TCTRL |= PWM_TCTRL_OUT_TRIG_EN(0x20); // Use VAL5 of SM0 for trigger point - M1 voltage and currents
	PWM1->SM[0].INTEN = PWM_INTEN_CMPIE(0x20); // for debug

#ifdef OVERDRIVE_MODE
	i16Tmp = (int16_t)(((float_t)M2_ADC_TRIGGER_DELAY - M2_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000);
#else
	i16Tmp = (int16_t)(((float_t)M2_ADC_TRIGGER_DELAY - M2_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000);
#endif
	PWM1->SM[1].VAL4 = (int16_t)(PWM1->SM[0].INIT)/2 + i16Tmp;
	PWM1->SM[1].TCTRL |= PWM_TCTRL_OUT_TRIG_EN(0x10); // Use VAL4 of SM1 for trigger point - M2 voltage and currents
	PWM1->SM[1].INTEN = PWM_INTEN_CMPIE(0x10); // for debug

#ifdef OVERDRIVE_MODE
	i16Tmp = (int16_t)(((float_t)M3_ADC_TRIGGER_DELAY - M3_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000);
#else
	i16Tmp = (int16_t)(((float_t)M3_ADC_TRIGGER_DELAY - M3_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000);
#endif
	PWM1->SM[1].VAL5 = PWM1->SM[0].VAL0 + i16Tmp;
	PWM1->SM[1].TCTRL |= PWM_TCTRL_OUT_TRIG_EN(0x20); // Use VAL5 of SM1 for trigger point - M3 voltage and currents
	PWM1->SM[1].INTEN |= PWM_INTEN_CMPIE(0x20); // for debug

#ifdef OVERDRIVE_MODE
	i16Tmp = (int16_t)(((float_t)M4_ADC_TRIGGER_DELAY - M4_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000);
#else
	i16Tmp = (int16_t)(((float_t)M4_ADC_TRIGGER_DELAY - M4_ADC_AVERAGE_TIME*0.5f)*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000);
#endif
	PWM1->SM[2].VAL4 = PWM1->SM[0].VAL1/2 + i16Tmp;
	PWM1->SM[2].TCTRL |= PWM_TCTRL_OUT_TRIG_EN(0x10); // Use VAL4 of SM2 for trigger point - M4 voltage and currents
	PWM1->SM[2].INTEN = PWM_INTEN_CMPIE(0x10); // for debug

	/* Fault protection setting: Fault protects PWMA&PWMB outputs of SM0~SM2 */
	PWM1->SM[0].DISMAP[0] = 0;
	PWM1->SM[1].DISMAP[0] = 0;
	PWM1->SM[2].DISMAP[0] = 0;

	PWM1->SM[0].DISMAP[0] = (PWM1->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M1_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M1_FAULT_NUM);
	PWM1->SM[1].DISMAP[0] = (PWM1->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M1_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M1_FAULT_NUM);
	PWM1->SM[2].DISMAP[0] = (PWM1->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M1_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M1_FAULT_NUM);

	PWM1->FCTRL = (PWM1->FCTRL & ~PWM_FCTRL_FLVL_MASK)|PWM_FCTRL_FLVL(1<<M1_FAULT_NUM); // A logic 1 on fault indicates a fault condition
	PWM1->FCTRL = (PWM1->FCTRL & ~PWM_FCTRL_FAUTO_MASK); // Manual clearing for fault0~3
	PWM1->FCTRL = (PWM1->FCTRL & ~PWM_FCTRL_FSAFE_MASK)|PWM_FCTRL_FSAFE(1<<M1_FAULT_NUM); // Safe mode for fault

	PWM1->FSTS = (PWM1->FSTS & ~PWM_FSTS_FFULL_MASK)|PWM_FSTS_FFULL(1<<M1_FAULT_NUM); // Full cycle recovery for fault0
	PWM1->FCTRL2 = (PWM1->FCTRL2 & ~PWM_FCTRL2_NOCOMB_MASK)|PWM_FCTRL2_NOCOMB(1<<M1_FAULT_NUM); // No combinational path for fault. Fault signal will be filtered
	PWM1->FFILT = PWM_FFILT_FILT_CNT(3)|PWM_FFILT_FILT_PER(2);

	/* End of PWM initialization, setting LDOK */
	PWM1->MCTRL = PWM_MCTRL_CLDOK(0x7); // Clear LDOK of SM0~SM2
	PWM1->MCTRL |= PWM_MCTRL_LDOK(0x7); // Set LDOK of SM0~SM2


    /* Initialize MC driver */
    g_sM1Pwm3ph.pui32PwmBaseAddress = (PWM_Type *)PWM1;

    g_sM1Pwm3ph.ui16PhASubNum = M1_PWM_PAIR_PHA; /* PWMA phase A sub-module number */
    g_sM1Pwm3ph.ui16PhBSubNum = M1_PWM_PAIR_PHB; /* PWMA phase B sub-module number */
    g_sM1Pwm3ph.ui16PhCSubNum = M1_PWM_PAIR_PHC; /* PWMA phase C sub-module number */

    g_sM1Pwm3ph.ui16FaultFixNum = M1_FAULT_NUM; /* PWMA fixed-value over-current fault number */
    g_sM1Pwm3ph.ui16FaultAdjNum = M1_FAULT_NUM; /* PWMA adjustable over-current fault number */

}

static void eFlexPWM2_init(void) // Synchronized with PWM1
{
	CLOCK_EnableClock(kCLOCK_Pwm2);

	/* Sub-module counter sync and buffered registers reload setting */
	PWM2->SM[0].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM2->SM[0].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM2->SM[0].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM2->SM[0].CTRL2 = (PWM2->SM[0].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM2->SM[0].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM2->SM[0].CTRL2 = (PWM2->SM[0].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM2->SM[0].CTRL2 &= ~PWM_CTRL2_RELOAD_SEL_MASK; // use local reload signal

	PWM2->SM[1].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM2->SM[1].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM2->SM[1].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM2->SM[1].CTRL2 = (PWM2->SM[1].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM2->SM[1].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM2->SM[1].CTRL2 = (PWM2->SM[1].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM2->SM[1].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	PWM2->SM[2].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM2->SM[2].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM2->SM[2].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM2->SM[2].CTRL2 = (PWM2->SM[2].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM2->SM[2].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM2->SM[2].CTRL2 = (PWM2->SM[2].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM2->SM[2].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	/* PWM frequency and duty setting */
#ifdef OVERDRIVE_MODE
	PWM2->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M2_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM2->SM[0].VAL1  = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M2_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#else
	PWM2->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M2_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM2->SM[0].VAL1  = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M2_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#endif
	PWM2->SM[0].VAL0 = 0;
	PWM2->SM[0].VAL2  = (int16_t)(PWM2->SM[0].INIT)/2; // 50% duty
	PWM2->SM[0].VAL3  = PWM2->SM[0].VAL1/2; // 50% duty

	PWM2->SM[1].INIT  = PWM2->SM[0].INIT;
	PWM2->SM[1].VAL1  = PWM2->SM[0].VAL1;
	PWM2->SM[1].VAL0 = 0;
	PWM2->SM[1].VAL2  = PWM2->SM[0].VAL2;
	PWM2->SM[1].VAL3  = PWM2->SM[0].VAL3;

	PWM2->SM[2].INIT  = PWM2->SM[0].INIT;
	PWM2->SM[2].VAL1  = PWM2->SM[0].VAL1;
	PWM2->SM[2].VAL0 = 0;
	PWM2->SM[2].VAL2  = PWM2->SM[0].VAL2;
	PWM2->SM[2].VAL3  = PWM2->SM[0].VAL3;

	/* Deadtime setting */
#ifdef OVERDRIVE_MODE
	PWM2->SM[0].DTCNT0 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[0].DTCNT1 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[1].DTCNT0 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[1].DTCNT1 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[2].DTCNT0 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[2].DTCNT1 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
#else
	PWM2->SM[0].DTCNT0 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[0].DTCNT1 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[1].DTCNT0 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[1].DTCNT1 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[2].DTCNT0 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM2->SM[2].DTCNT1 = M2_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
#endif

	/* Fault protection setting: Fault protects PWMA&PWMB outputs of SM0~SM2 */
	PWM2->SM[0].DISMAP[0] = 0;
	PWM2->SM[1].DISMAP[0] = 0;
	PWM2->SM[2].DISMAP[0] = 0;

	PWM2->SM[0].DISMAP[0] = (PWM2->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M2_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M2_FAULT_NUM);
	PWM2->SM[1].DISMAP[0] = (PWM2->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M2_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M2_FAULT_NUM);
	PWM2->SM[2].DISMAP[0] = (PWM2->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M2_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M2_FAULT_NUM);

	PWM2->FCTRL = (PWM2->FCTRL & ~PWM_FCTRL_FLVL_MASK)|PWM_FCTRL_FLVL(1<<M2_FAULT_NUM); // A logic 1 on fault indicates a fault condition
	PWM2->FCTRL = (PWM2->FCTRL & ~PWM_FCTRL_FAUTO_MASK); // Manual clearing for fault0~3
	PWM2->FCTRL = (PWM2->FCTRL & ~PWM_FCTRL_FSAFE_MASK)|PWM_FCTRL_FSAFE(1<<M2_FAULT_NUM); // Safe mode for fault

	PWM2->FSTS = (PWM2->FSTS & ~PWM_FSTS_FFULL_MASK)|PWM_FSTS_FFULL(1<<M2_FAULT_NUM); // Full cycle recovery for fault0
	PWM2->FCTRL2 = (PWM2->FCTRL2 & ~PWM_FCTRL2_NOCOMB_MASK)|PWM_FCTRL2_NOCOMB(1<<M2_FAULT_NUM); // No combinational path for fault. Fault signal will be filtered
	PWM2->FFILT = PWM_FFILT_FILT_CNT(3)|PWM_FFILT_FILT_PER(2);

	/* End of PWM initialization, setting LDOK */
	PWM2->MCTRL = PWM_MCTRL_CLDOK(0x7); // Clear LDOK of SM0~SM2
	PWM2->MCTRL |= PWM_MCTRL_LDOK(0x7); // Set LDOK of SM0~SM2

    /* Initialize MC driver */
    g_sM2Pwm3ph.pui32PwmBaseAddress = (PWM_Type *)PWM2;

    g_sM2Pwm3ph.ui16PhASubNum = M2_PWM_PAIR_PHA; /* PWMA phase A sub-module number */
    g_sM2Pwm3ph.ui16PhBSubNum = M2_PWM_PAIR_PHB; /* PWMA phase B sub-module number */
    g_sM2Pwm3ph.ui16PhCSubNum = M2_PWM_PAIR_PHC; /* PWMA phase C sub-module number */

    g_sM2Pwm3ph.ui16FaultFixNum = M2_FAULT_NUM; /* PWMA fixed-value over-current fault number */
    g_sM2Pwm3ph.ui16FaultAdjNum = M2_FAULT_NUM; /* PWMA adjustable over-current fault number */

}

static void eFlexPWM3_init(void) // Synchronized with PWM1
{
	CLOCK_EnableClock(kCLOCK_Pwm3);

	/* Sub-module counter sync and buffered registers reload setting */
	PWM3->SM[0].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM3->SM[0].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM3->SM[0].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM3->SM[0].CTRL2 = (PWM3->SM[0].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM3->SM[0].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM3->SM[0].CTRL2 = (PWM3->SM[0].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM3->SM[0].CTRL2 &= ~PWM_CTRL2_RELOAD_SEL_MASK; // use local reload signal

	PWM3->SM[1].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM3->SM[1].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM3->SM[1].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM3->SM[1].CTRL2 = (PWM3->SM[1].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM3->SM[1].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM3->SM[1].CTRL2 = (PWM3->SM[1].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM3->SM[1].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	PWM3->SM[2].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM3->SM[2].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM3->SM[2].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM3->SM[2].CTRL2 = (PWM3->SM[2].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM3->SM[2].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM3->SM[2].CTRL2 = (PWM3->SM[2].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM3->SM[2].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	/* PWM frequency and duty setting */
#ifdef OVERDRIVE_MODE
	PWM3->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M3_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM3->SM[0].VAL1  = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M3_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#else
	PWM3->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M3_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM3->SM[0].VAL1  = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M3_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#endif
	PWM3->SM[0].VAL0 = 0;
	PWM3->SM[0].VAL2  = (int16_t)(PWM3->SM[0].INIT)/2; // 50% duty
	PWM3->SM[0].VAL3  = PWM3->SM[0].VAL1/2; // 50% duty

	PWM3->SM[1].INIT  = PWM3->SM[0].INIT;
	PWM3->SM[1].VAL1  = PWM3->SM[0].VAL1;
	PWM3->SM[1].VAL0 = 0;
	PWM3->SM[1].VAL2  = PWM3->SM[0].VAL2;
	PWM3->SM[1].VAL3  = PWM3->SM[0].VAL3;

	PWM3->SM[2].INIT  = PWM3->SM[0].INIT;
	PWM3->SM[2].VAL1  = PWM3->SM[0].VAL1;
	PWM3->SM[2].VAL0 = 0;
	PWM3->SM[2].VAL2  = PWM3->SM[0].VAL2;
	PWM3->SM[2].VAL3  = PWM3->SM[0].VAL3;

	/* Deadtime setting */
#ifdef OVERDRIVE_MODE
	PWM3->SM[0].DTCNT0 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[0].DTCNT1 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[1].DTCNT0 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[1].DTCNT1 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[2].DTCNT0 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[2].DTCNT1 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
#else
	PWM3->SM[0].DTCNT0 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[0].DTCNT1 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[1].DTCNT0 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[1].DTCNT1 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[2].DTCNT0 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM3->SM[2].DTCNT1 = M3_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
#endif

	/* Fault protection setting: Fault protects PWMA&PWMB outputs of SM0~SM2 */
	PWM3->SM[0].DISMAP[0] = 0;
	PWM3->SM[1].DISMAP[0] = 0;
	PWM3->SM[2].DISMAP[0] = 0;

	PWM3->SM[0].DISMAP[0] = (PWM3->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M3_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M3_FAULT_NUM);
	PWM3->SM[1].DISMAP[0] = (PWM3->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M3_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M3_FAULT_NUM);
	PWM3->SM[2].DISMAP[0] = (PWM3->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M3_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M3_FAULT_NUM);

	PWM3->FCTRL = (PWM3->FCTRL & ~PWM_FCTRL_FLVL_MASK)|PWM_FCTRL_FLVL(1<<M3_FAULT_NUM); // A logic 1 on fault indicates a fault condition
	PWM3->FCTRL = (PWM3->FCTRL & ~PWM_FCTRL_FAUTO_MASK); // Manual clearing for fault0~3
	PWM3->FCTRL = (PWM3->FCTRL & ~PWM_FCTRL_FSAFE_MASK)|PWM_FCTRL_FSAFE(1<<M3_FAULT_NUM); // Safe mode for fault

	PWM3->FSTS = (PWM3->FSTS & ~PWM_FSTS_FFULL_MASK)|PWM_FSTS_FFULL(1<<M3_FAULT_NUM); // Full cycle recovery for fault0
	PWM3->FCTRL2 = (PWM3->FCTRL2 & ~PWM_FCTRL2_NOCOMB_MASK)|PWM_FCTRL2_NOCOMB(1<<M3_FAULT_NUM); // No combinational path for fault. Fault signal will be filtered
	PWM3->FFILT = PWM_FFILT_FILT_CNT(3)|PWM_FFILT_FILT_PER(2);

	/* End of PWM initialization, setting LDOK */
	PWM3->MCTRL = PWM_MCTRL_CLDOK(0x7); // Clear LDOK of SM0~SM2
	PWM3->MCTRL |= PWM_MCTRL_LDOK(0x7); // Set LDOK of SM0~SM2

    /* Initialize MC driver */
    g_sM3Pwm3ph.pui32PwmBaseAddress = (PWM_Type *)PWM3;

    g_sM3Pwm3ph.ui16PhASubNum = M3_PWM_PAIR_PHA; /* PWMA phase A sub-module number */
    g_sM3Pwm3ph.ui16PhBSubNum = M3_PWM_PAIR_PHB; /* PWMA phase B sub-module number */
    g_sM3Pwm3ph.ui16PhCSubNum = M3_PWM_PAIR_PHC; /* PWMA phase C sub-module number */

    g_sM3Pwm3ph.ui16FaultFixNum = M3_FAULT_NUM; /* PWMA fixed-value over-current fault number */
    g_sM3Pwm3ph.ui16FaultAdjNum = M3_FAULT_NUM; /* PWMA adjustable over-current fault number */

}

static void eFlexPWM4_init(void) // Synchronized with PWM1
{
	CLOCK_EnableClock(kCLOCK_Pwm4);

	/* Sub-module counter sync and buffered registers reload setting */
	PWM4->SM[0].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM4->SM[0].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM4->SM[0].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM4->SM[0].CTRL2 = (PWM4->SM[0].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM4->SM[0].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM4->SM[0].CTRL2 = (PWM4->SM[0].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM4->SM[0].CTRL2 &= ~PWM_CTRL2_RELOAD_SEL_MASK; // use local reload signal

	PWM4->SM[1].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM4->SM[1].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM4->SM[1].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM4->SM[1].CTRL2 = (PWM4->SM[1].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM4->SM[1].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM4->SM[1].CTRL2 = (PWM4->SM[1].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM4->SM[1].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	PWM4->SM[2].CTRL |= PWM_CTRL_HALF_MASK;      // half cycle reload
	PWM4->SM[2].CTRL &= ~PWM_CTRL_COMPMODE_MASK; // Edge is generated on counter "equal to" value register
	PWM4->SM[2].CTRL &= ~PWM_CTRL_LDMOD_MASK;    // buffered registers take effect at PWM reload signal when LDOK is set
	PWM4->SM[2].CTRL2 = (PWM4->SM[2].CTRL2 & ~PWM_CTRL2_CLK_SEL_MASK)|PWM_CTRL2_CLK_SEL(0);   // use IPBus clock
	PWM4->SM[2].CTRL2 &= ~PWM_CTRL2_INDEP_MASK;  // complementary mode
	PWM4->SM[2].CTRL2 = (PWM4->SM[2].CTRL2 & ~PWM_CTRL2_INIT_SEL_MASK)|PWM_CTRL2_INIT_SEL(3); // use external sync
	PWM4->SM[2].CTRL2 |= PWM_CTRL2_RELOAD_SEL_MASK; // use master reload from SM0

	/* PWM frequency and duty setting */
#ifdef OVERDRIVE_MODE
	PWM4->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M4_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM4->SM[0].VAL1  = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(2.0*M4_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#else
	PWM4->SM[0].INIT  = -((int16_t)(BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M4_PWM_FREQ))); // Get INIT value from given PWM frequency
	PWM4->SM[0].VAL1  = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(2.0*M4_PWM_FREQ)-1;             // Get VAL1 value from given PWM frequency
#endif
	PWM4->SM[0].VAL0 = 0;
	PWM4->SM[0].VAL2  = (int16_t)(PWM4->SM[0].INIT)/2; // 50% duty
	PWM4->SM[0].VAL3  = PWM4->SM[0].VAL1/2; // 50% duty

	PWM4->SM[1].INIT  = PWM4->SM[0].INIT;
	PWM4->SM[1].VAL1  = PWM4->SM[0].VAL1;
	PWM4->SM[1].VAL0 = 0;
	PWM4->SM[1].VAL2  = PWM4->SM[0].VAL2;
	PWM4->SM[1].VAL3  = PWM4->SM[0].VAL3;

	PWM4->SM[2].INIT  = PWM4->SM[0].INIT;
	PWM4->SM[2].VAL1  = PWM4->SM[0].VAL1;
	PWM4->SM[2].VAL0 = 0;
	PWM4->SM[2].VAL2  = PWM4->SM[0].VAL2;
	PWM4->SM[2].VAL3  = PWM4->SM[0].VAL3;

	/* Deadtime setting */
#ifdef OVERDRIVE_MODE
	PWM4->SM[0].DTCNT0 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[0].DTCNT1 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[1].DTCNT0 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[1].DTCNT1 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[2].DTCNT0 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[2].DTCNT1 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/1000000;
#else
	PWM4->SM[0].DTCNT0 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[0].DTCNT1 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[1].DTCNT0 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[1].DTCNT1 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[2].DTCNT0 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
	PWM4->SM[2].DTCNT1 = M4_PWM_DEADTIME*BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/1000000;
#endif

	/* Fault protection setting: Fault protects PWMA&PWMB outputs of SM0~SM2 */
	PWM4->SM[0].DISMAP[0] = 0;
	PWM4->SM[1].DISMAP[0] = 0;
	PWM4->SM[2].DISMAP[0] = 0;

	PWM4->SM[0].DISMAP[0] = (PWM4->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M4_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M4_FAULT_NUM);
	PWM4->SM[1].DISMAP[0] = (PWM4->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M4_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M4_FAULT_NUM);
	PWM4->SM[2].DISMAP[0] = (PWM4->SM[0].DISMAP[0] & ~(PWM_DISMAP_DIS0A_MASK|PWM_DISMAP_DIS0B_MASK))|PWM_DISMAP_DIS0A(1<<M4_FAULT_NUM)|PWM_DISMAP_DIS0B(1<<M4_FAULT_NUM);

	PWM4->FCTRL = (PWM4->FCTRL & ~PWM_FCTRL_FLVL_MASK)|PWM_FCTRL_FLVL(1<<M4_FAULT_NUM); // A logic 1 on fault indicates a fault condition
	PWM4->FCTRL = (PWM4->FCTRL & ~PWM_FCTRL_FAUTO_MASK); // Manual clearing for fault0~3
	PWM4->FCTRL = (PWM4->FCTRL & ~PWM_FCTRL_FSAFE_MASK)|PWM_FCTRL_FSAFE(1<<M4_FAULT_NUM); // Safe mode for fault

	PWM4->FSTS = (PWM4->FSTS & ~PWM_FSTS_FFULL_MASK)|PWM_FSTS_FFULL(1<<M4_FAULT_NUM); // Full cycle recovery for fault0
	PWM4->FCTRL2 = (PWM4->FCTRL2 & ~PWM_FCTRL2_NOCOMB_MASK)|PWM_FCTRL2_NOCOMB(1<<M4_FAULT_NUM); // No combinational path for fault. Fault signal will be filtered
	PWM4->FFILT = PWM_FFILT_FILT_CNT(3)|PWM_FFILT_FILT_PER(2);

	/* End of PWM initialization, setting LDOK */
	PWM4->MCTRL = PWM_MCTRL_CLDOK(0x7); // Clear LDOK of SM0~SM2
	PWM4->MCTRL |= PWM_MCTRL_LDOK(0x7); // Set LDOK of SM0~SM2

    /* Initialize MC driver */
    g_sM4Pwm3ph.pui32PwmBaseAddress = (PWM_Type *)PWM4;

    g_sM4Pwm3ph.ui16PhASubNum = M4_PWM_PAIR_PHA; /* PWMA phase A sub-module number */
    g_sM4Pwm3ph.ui16PhBSubNum = M4_PWM_PAIR_PHB; /* PWMA phase B sub-module number */
    g_sM4Pwm3ph.ui16PhCSubNum = M4_PWM_PAIR_PHC; /* PWMA phase C sub-module number */

    g_sM4Pwm3ph.ui16FaultFixNum = M4_FAULT_NUM; /* PWMA fixed-value over-current fault number */
    g_sM4Pwm3ph.ui16FaultAdjNum = M4_FAULT_NUM; /* PWMA adjustable over-current fault number */

}

static void lpadc_channel_assignment(channel_table_t *psChannel)
{
	if(psChannel->sLPADC1IA.ui16Cmd != NOT_EXIST)
	{
		LPADC1->CMD[psChannel->sLPADC1IA.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC1->CMD[psChannel->sLPADC1IA.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC1IA.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC1IA.ui16ChanNum);
		LPADC1->CMD[psChannel->sLPADC1IA.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC1IB.ui16Cmd != NOT_EXIST)
	{
		LPADC1->CMD[psChannel->sLPADC1IB.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC1->CMD[psChannel->sLPADC1IB.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC1IB.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC1IB.ui16ChanNum);
		LPADC1->CMD[psChannel->sLPADC1IB.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC1IC.ui16Cmd != NOT_EXIST)
	{
		LPADC1->CMD[psChannel->sLPADC1IC.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC1->CMD[psChannel->sLPADC1IC.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC1IC.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC1IC.ui16ChanNum);
		LPADC1->CMD[psChannel->sLPADC1IC.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC2IA.ui16Cmd != NOT_EXIST)
	{
		LPADC2->CMD[psChannel->sLPADC2IA.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC2->CMD[psChannel->sLPADC2IA.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC2IA.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC2IA.ui16ChanNum);
		LPADC2->CMD[psChannel->sLPADC2IA.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC2IB.ui16Cmd != NOT_EXIST)
	{
		LPADC2->CMD[psChannel->sLPADC2IB.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC2->CMD[psChannel->sLPADC2IB.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC2IB.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC2IB.ui16ChanNum);
		LPADC2->CMD[psChannel->sLPADC2IB.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC2IC.ui16Cmd != NOT_EXIST)
	{
		LPADC2->CMD[psChannel->sLPADC2IC.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC2->CMD[psChannel->sLPADC2IC.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC2IC.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC2IC.ui16ChanNum);
		LPADC2->CMD[psChannel->sLPADC2IC.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC1VDcb.ui16Cmd != NOT_EXIST)
	{
		LPADC1->CMD[psChannel->sLPADC1VDcb.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC1->CMD[psChannel->sLPADC1VDcb.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC1VDcb.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC1VDcb.ui16ChanNum);
		LPADC1->CMD[psChannel->sLPADC1VDcb.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	else if(psChannel->sLPADC2VDcb.ui16Cmd != NOT_EXIST)
	{
		LPADC2->CMD[psChannel->sLPADC2VDcb.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC2->CMD[psChannel->sLPADC2VDcb.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC2VDcb.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC2VDcb.ui16ChanNum);
		LPADC2->CMD[psChannel->sLPADC2VDcb.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	if(psChannel->sLPADC1Aux.ui16Cmd != NOT_EXIST)
	{
		LPADC1->CMD[psChannel->sLPADC1Aux.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC1->CMD[psChannel->sLPADC1Aux.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC1Aux.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC1Aux.ui16ChanNum);
		LPADC1->CMD[psChannel->sLPADC1Aux.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
	else if(psChannel->sLPADC2Aux.ui16Cmd != NOT_EXIST)
	{
		LPADC2->CMD[psChannel->sLPADC2Aux.ui16Cmd-1].CMDL &= ~ADC_CMDL_CSCALE_MASK; // Input is scaled to 30/64
		LPADC2->CMD[psChannel->sLPADC2Aux.ui16Cmd-1].CMDL |= ADC_CMDL_ABSEL(psChannel->sLPADC2Aux.ui16Side)|\
																					 ADC_CMDL_ADCH(psChannel->sLPADC2Aux.ui16ChanNum);
		LPADC2->CMD[psChannel->sLPADC2Aux.ui16Cmd-1].CMDH = ADC_CMDH_STS(1)|ADC_CMDH_AVGS(psChannel->ui16Average); // 5 cycles for sampling
	}
}

static void lpadc_init(void)
{
	CLOCK_EnableClock(kCLOCK_Lpadc1);
	CLOCK_EnableClock(kCLOCK_Lpadc2);

	LPADC1->CTRL = ADC_CTRL_RSTFIFO_MASK|ADC_CTRL_RST_MASK; // Reset fifo and wrapper logic
	LPADC1->CTRL = ADC_CTRL_TRIG_SRC(0);                    // ADC_ETC trigger only
	LPADC2->CTRL = ADC_CTRL_RSTFIFO_MASK|ADC_CTRL_RST_MASK; // Reset fifo and wrapper logic
	LPADC2->CTRL = ADC_CTRL_TRIG_SRC(0);                    // ADC_ETC trigger only

	LPADC1->CFG |= ADC_CFG_PWREN(1)|ADC_CFG_REFSEL(0)|ADC_CFG_PWRSEL(3); // External VREFH=1.8V, highest power consumption
	LPADC2->CFG |= ADC_CFG_PWREN(1)|ADC_CFG_REFSEL(0)|ADC_CFG_PWRSEL(3); // External VREFH=1.8V, highest power consumption

	g_sM1AdcSensor.psChannelAssignment = &sChannelTableM1;
	g_sM2AdcSensor.psChannelAssignment = &sChannelTableM2;
	g_sM3AdcSensor.psChannelAssignment = &sChannelTableM3;
	g_sM4AdcSensor.psChannelAssignment = &sChannelTableM4;
	g_sM1AdcSensor.psChannelAssignment->ui16Average = M1_ADC_AVG_NUM;
	g_sM2AdcSensor.psChannelAssignment->ui16Average = M2_ADC_AVG_NUM;
	g_sM3AdcSensor.psChannelAssignment->ui16Average = M3_ADC_AVG_NUM;
	g_sM4AdcSensor.psChannelAssignment->ui16Average = M4_ADC_AVG_NUM;
	lpadc_channel_assignment(g_sM1AdcSensor.psChannelAssignment);
	lpadc_channel_assignment(g_sM2AdcSensor.psChannelAssignment);
	lpadc_channel_assignment(g_sM3AdcSensor.psChannelAssignment);
	lpadc_channel_assignment(g_sM4AdcSensor.psChannelAssignment);



	LPADC1->TCTRL[0] = ADC_TCTRL_CMD_SEL(1)|ADC_TCTRL_HTEN_MASK; // Enable HW trigger for TRIG0 of ADC1, TCMD comes from ADC_ETC
	LPADC2->TCTRL[0] = ADC_TCTRL_CMD_SEL(1)|ADC_TCTRL_HTEN_MASK; // Enable HW trigger for TRIG0 of ADC2, TCMD comes from ADC_ETC

	LPADC1->TCTRL[1] = ADC_TCTRL_CMD_SEL(1)|ADC_TCTRL_HTEN_MASK; // Enable HW trigger for TRIG0 of ADC1, TCMD comes from ADC_ETC
	LPADC2->TCTRL[1] = ADC_TCTRL_CMD_SEL(1)|ADC_TCTRL_HTEN_MASK; // Enable HW trigger for TRIG0 of ADC2, TCMD comes from ADC_ETC


	LPADC1->CTRL |= ADC_CTRL_ADCEN_MASK;
	LPADC2->CTRL |= ADC_CTRL_ADCEN_MASK;
}


void adc_etc_init(void)
{
	lpadc_init();
	g_sM1AdcSensor.ui8MotorNum = MOTOR_1;
	g_sM2AdcSensor.ui8MotorNum = MOTOR_2;
	g_sM3AdcSensor.ui8MotorNum = MOTOR_3;
	g_sM4AdcSensor.ui8MotorNum = MOTOR_4;
	MCDRV_Curr3Ph2ShChanAssignInit(&g_sM1AdcSensor);
	MCDRV_Curr3Ph2ShChanAssignInit(&g_sM2AdcSensor);
	MCDRV_Curr3Ph2ShChanAssignInit(&g_sM3AdcSensor);
	MCDRV_Curr3Ph2ShChanAssignInit(&g_sM4AdcSensor);

	CLOCK_EnableClock(kCLOCK_Adc_Etc);

	ADC_ETC->CTRL = ADC_ETC_CTRL_SOFTRST_MASK; // Reset adc_etc
	ADC_ETC->CTRL &= ~ADC_ETC_CTRL_SOFTRST_MASK;

	ADC_ETC->CTRL = ADC_ETC_CTRL_TRIG_ENABLE(0xF); // TRIG0~3 are enabled

	ADC_ETC->TRIG[0].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[0].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[4].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[4].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[1].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[1].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[5].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[5].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[2].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[2].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[6].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[6].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[3].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[3].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2
	ADC_ETC->TRIG[7].TRIGn_CTRL = 0;
	ADC_ETC->TRIG[7].TRIGn_CTRL = ADC_ETC_TRIGn_CTRL_TRIG_CHAIN(1)|\
								  ADC_ETC_TRIGn_CTRL_SYNC_MODE_MASK|\
								  ADC_ETC_TRIGn_CTRL_TRIG_MODE(0); // HW trigger,sync mode, chain length is 2

	/*
	 *   Example for Motor1:
	 * 				                 	SEG0     SEG1
	 * 				LPADC1 on TRIG0 : IA_ADC1, UDC_ADC1
	 *
	 * 				LPADC2 on TRIG4 : IB_ADC2, IC_ADC2
	 *
	 *
	 * */
	ADC_ETC->TRIG[0].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM1AdcSensor.ui16ADC1Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM1AdcSensor.sCurrSec16.ui16ADC1Seg0CmdNum);
	ADC_ETC->TRIG[4].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM1AdcSensor.ui16ADC2Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM1AdcSensor.sCurrSec16.ui16ADC2Seg0CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_IE1(0)|ADC_ETC_TRIGn_CHAIN_1_0_IE1_EN_MASK; // Generates an interrupt on DONE0 when segment1 of TRIG4 is finished conversion

	ADC_ETC->TRIG[1].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM2AdcSensor.ui16ADC1Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM2AdcSensor.sCurrSec16.ui16ADC1Seg0CmdNum);
	ADC_ETC->TRIG[5].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM2AdcSensor.ui16ADC2Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM2AdcSensor.sCurrSec16.ui16ADC2Seg0CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_IE1(1)|ADC_ETC_TRIGn_CHAIN_1_0_IE1_EN_MASK; // Generates an interrupt on DONE1 when segment1 of TRIG5 is finished conversion

	ADC_ETC->TRIG[2].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM3AdcSensor.ui16ADC1Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM3AdcSensor.sCurrSec16.ui16ADC1Seg0CmdNum);
	ADC_ETC->TRIG[6].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM3AdcSensor.ui16ADC2Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM3AdcSensor.sCurrSec16.ui16ADC2Seg0CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_IE1(2)|ADC_ETC_TRIGn_CHAIN_1_0_IE1_EN_MASK; // Generates an interrupt on DONE2 when segment1 of TRIG6 is finished conversion

	ADC_ETC->TRIG[3].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM4AdcSensor.ui16ADC1Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM4AdcSensor.sCurrSec16.ui16ADC1Seg0CmdNum);
	ADC_ETC->TRIG[7].TRIGn_CHAIN_1_0 = ADC_ETC_TRIGn_CHAIN_1_0_B2B1_MASK|ADC_ETC_TRIGn_CHAIN_1_0_HWTS1(2)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL1(g_sM4AdcSensor.ui16ADC2Seg1CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_HWTS0(1)|ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(g_sM4AdcSensor.sCurrSec16.ui16ADC2Seg0CmdNum)|\
										ADC_ETC_TRIGn_CHAIN_1_0_B2B0_MASK|ADC_ETC_TRIGn_CHAIN_1_0_IE1(3)|ADC_ETC_TRIGn_CHAIN_1_0_IE1_EN_MASK; // Generates an interrupt on DONE3 when segment1 of TRIG7 is finished conversion


	NVIC_SetPriority(ADC_ETC_IRQ0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ADC_ETC_IRQ0_IRQn);
	NVIC_SetPriority(ADC_ETC_IRQ1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ADC_ETC_IRQ1_IRQn);
	NVIC_SetPriority(ADC_ETC_IRQ2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ADC_ETC_IRQ2_IRQn);
	NVIC_SetPriority(ADC_ETC_IRQ3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ADC_ETC_IRQ3_IRQn);

	NVIC_SetPriority(ADC_ETC_ERROR_IRQ_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ADC_ETC_ERROR_IRQ_IRQn);
}



static void qdc1_init(void)
{
	CLOCK_EnableClock(kCLOCK_Enc1);

	ENC1->LMOD = 4*M1_ENCODER_LINES-1;
	ENC1->UMOD = 0;
	ENC1->LPOS = 0;
	ENC1->UPOS = 0;
	ENC1->FILT = ENC_FILT_FILT_CNT(2)|ENC_FILT_FILT_PER(1);
	ENC1->CTRL2 |= ENC_CTRL2_MOD_MASK|ENC_CTRL2_REVMOD_MASK; // Enable modulo counting, and REV is controlled by modulo counting
	ENC1->CTRL3 = ENC_CTRL3_PMEN_MASK|ENC_CTRL3_PRSC(M1_QDC_TIMER_PRESCALER);
	ENC1->CTRL |= ENC_CTRL_XIE_MASK; // Enable index signal interrupt

	g_sM1QdcSensor.pQDC_base = ENC1;
	g_sM1QdcSensor.ui8MotorNum = MOTOR_1;
	MCDRV_QdcInit(&g_sM1QdcSensor);
	MCDRV_QdcSpeedCalInit(&g_sM1QdcSensor);
	MCDRV_QdcToSpeedCalInit(&g_sM1QdcSensor);
}

static void qdc2_init(void)
{
	CLOCK_EnableClock(kCLOCK_Enc2);

	ENC2->LMOD = 4*M2_ENCODER_LINES-1;
	ENC2->UMOD = 0;
	ENC2->LPOS = 0;
	ENC2->UPOS = 0;
	ENC2->FILT = ENC_FILT_FILT_CNT(2)|ENC_FILT_FILT_PER(1);
	ENC2->CTRL2 |= ENC_CTRL2_MOD_MASK|ENC_CTRL2_REVMOD_MASK; // Enable modulo counting, and REV is controlled by modulo counting
	ENC2->CTRL3 = ENC_CTRL3_PMEN_MASK|ENC_CTRL3_PRSC(M2_QDC_TIMER_PRESCALER);
	ENC2->CTRL |= ENC_CTRL_XIE_MASK; // Enable index signal interrupt

	g_sM2QdcSensor.pQDC_base = ENC2;
	g_sM2QdcSensor.ui8MotorNum = MOTOR_2;
	MCDRV_QdcInit(&g_sM2QdcSensor);
	MCDRV_QdcSpeedCalInit(&g_sM2QdcSensor);
	MCDRV_QdcToSpeedCalInit(&g_sM2QdcSensor);
}

static void qdc3_init(void)
{
	CLOCK_EnableClock(kCLOCK_Enc3);

	ENC3->LMOD = 4*M3_ENCODER_LINES-1;
	ENC3->UMOD = 0;
	ENC3->LPOS = 0;
	ENC3->UPOS = 0;
	ENC3->FILT = ENC_FILT_FILT_CNT(2)|ENC_FILT_FILT_PER(1);
	ENC3->CTRL2 |= ENC_CTRL2_MOD_MASK|ENC_CTRL2_REVMOD_MASK; // Enable modulo counting, and REV is controlled by modulo counting
	ENC3->CTRL3 = ENC_CTRL3_PMEN_MASK|ENC_CTRL3_PRSC(M3_QDC_TIMER_PRESCALER);
	ENC3->CTRL |= ENC_CTRL_XIE_MASK; // Enable index signal interrupt

	g_sM3QdcSensor.pQDC_base = ENC3;
	g_sM3QdcSensor.ui8MotorNum = MOTOR_3;
	MCDRV_QdcInit(&g_sM3QdcSensor);
	MCDRV_QdcSpeedCalInit(&g_sM3QdcSensor);
	MCDRV_QdcToSpeedCalInit(&g_sM3QdcSensor);
}

static void qdc4_init(void)
{
	CLOCK_EnableClock(kCLOCK_Enc4);

	ENC4->LMOD = 4*M4_ENCODER_LINES-1;
	ENC4->UMOD = 0;
	ENC4->LPOS = 0;
	ENC4->UPOS = 0;
	ENC4->FILT = ENC_FILT_FILT_CNT(2)|ENC_FILT_FILT_PER(1);
	ENC4->CTRL2 |= ENC_CTRL2_MOD_MASK|ENC_CTRL2_REVMOD_MASK; // Enable modulo counting, and REV is controlled by modulo counting
	ENC4->CTRL3 = ENC_CTRL3_PMEN_MASK|ENC_CTRL3_PRSC(M4_QDC_TIMER_PRESCALER);
	ENC4->CTRL |= ENC_CTRL_XIE_MASK; // Enable index signal interrupt

	g_sM4QdcSensor.pQDC_base = ENC4;
	g_sM4QdcSensor.ui8MotorNum = MOTOR_4;
	MCDRV_QdcInit(&g_sM4QdcSensor);
	MCDRV_QdcSpeedCalInit(&g_sM4QdcSensor);
	MCDRV_QdcToSpeedCalInit(&g_sM4QdcSensor);
}

static void pit1_Init(void) // For motor1 slow loop
{
	CLOCK_EnableClock(kCLOCK_Pit1);
	PIT1->MCR &= ~PIT_MCR_MDIS_MASK;
#ifdef OVERDRIVE_MODE
	PIT1->CHANNEL[0].LDVAL = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/M1_SLOW_LOOP_FREQ;
#else
	PIT1->CHANNEL[0].LDVAL = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/M1_SLOW_LOOP_FREQ;
#endif
	PIT1->CHANNEL[0].TFLG = 1; // Clear flag
	PIT1->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK; // Enable interrupt of channel0 timer

	NVIC_SetPriority(PIT1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
	NVIC_EnableIRQ(PIT1_IRQn);
}

static void pit2_Init(void) // For motor2 slow loop
{
	CLOCK_EnableClock(kCLOCK_Pit2);
	PIT2->MCR &= ~PIT_MCR_MDIS_MASK;
#ifdef OVERDRIVE_MODE
	PIT2->CHANNEL[0].LDVAL = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_LPSR_CLK_ROOT/M2_SLOW_LOOP_FREQ;
#else
	PIT2->CHANNEL[0].LDVAL = BOARD_BOOTCLOCKRUN_BUS_LPSR_CLK_ROOT/M2_SLOW_LOOP_FREQ;
#endif
	PIT2->CHANNEL[0].TFLG = 1; // Clear flag
	PIT2->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK; // Enable interrupt of channel0 timer

	NVIC_SetPriority(PIT2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
	NVIC_EnableIRQ(PIT2_IRQn);
}

static void qtimer1_0_init(void) // For motor3 slow loop
{
	CLOCK_EnableClock(kCLOCK_Qtimer1);

	TMR1->CHANNEL[0].CTRL = TMR_CTRL_PCS(0xB)|TMR_CTRL_CM(0)|TMR_CTRL_LENGTH_MASK; // IP bus clock/8
#ifdef OVERDRIVE_MODE
	TMR1->CHANNEL[0].COMP1 = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(8.0*M3_SLOW_LOOP_FREQ);
#else
	TMR1->CHANNEL[0].COMP1 = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(8.0*M3_SLOW_LOOP_FREQ);
#endif
	TMR1->CHANNEL[0].SCTRL |= TMR_SCTRL_TCFIE_MASK;

	NVIC_SetPriority(TMR1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
	NVIC_EnableIRQ(TMR1_IRQn);
}

static void qtimer2_0_init(void) // For motor4 slow loop
{
	CLOCK_EnableClock(kCLOCK_Qtimer2);

	TMR2->CHANNEL[0].CTRL = TMR_CTRL_PCS(0xB)|TMR_CTRL_CM(0)|TMR_CTRL_LENGTH_MASK; // IP bus clock/8
#ifdef OVERDRIVE_MODE
	TMR2->CHANNEL[0].COMP1 = BOARD_BOOTCLOCKOVERDRIVERUN_BUS_CLK_ROOT/(8.0*M4_SLOW_LOOP_FREQ);
#else
	TMR2->CHANNEL[0].COMP1 = BOARD_BOOTCLOCKRUN_BUS_CLK_ROOT/(8.0*M4_SLOW_LOOP_FREQ);
#endif
	TMR2->CHANNEL[0].SCTRL |= TMR_SCTRL_TCFIE_MASK;

	NVIC_SetPriority(TMR2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1);
	NVIC_EnableIRQ(TMR2_IRQn);
}

static void qtimer1_1_init(void)
{
	CLOCK_EnableClock(kCLOCK_Qtimer1);

	TMR1->CHANNEL[1].CTRL = TMR_CTRL_PCS(0x8); // IP bus clock
}

static void qtimer1_2_init(void)
{
	CLOCK_EnableClock(kCLOCK_Qtimer1);

	TMR1->CHANNEL[2].CTRL = TMR_CTRL_PCS(0x8); // IP bus clock
}

static void qtimer1_3_init(void)
{
	CLOCK_EnableClock(kCLOCK_Qtimer1);

	TMR1->CHANNEL[3].CTRL = TMR_CTRL_PCS(0x8); /* IP bus clock */
}


void peripherals_manual_init(void)
{
	/* Must enable clock gates of used peripherals before configuring them */
	eFlexPWM1_init();
	eFlexPWM2_init();
	eFlexPWM3_init();
	eFlexPWM4_init();
	adc_etc_init();
	pit1_Init();
	pit2_Init();
	qtimer1_0_init();
	qtimer2_0_init();
	qdc1_init();
	qdc2_init();
	qdc3_init();
	qdc4_init();
	qtimer1_1_init();
	qtimer1_2_init();
	qtimer1_3_init();
}




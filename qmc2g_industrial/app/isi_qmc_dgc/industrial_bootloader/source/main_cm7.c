/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/**
 * @file    main_cm7.c
 * @brief   Application entry point.
 */
#include <qmc2_boot.h>
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "resource_config_sbl.h"


#include "fsl_rdc.h"
#include "fsl_gpt.h"
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1176_cm7.h"
#include "fsl_cache.h"
#include "ksdk_mbedtls.h"

#if USE_DELAY_REQUIERD_FOR_HW_INIT
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define GPT_IRQ_ID             GPT2_IRQn
#define EXAMPLE_GPT            GPT2
#define EXAMPLE_GPT_IRQHandler GPT2_IRQHandler

volatile bool gptIsrFlag = true;

#endif
/*
 *
 * @brief   Application entry point.
 */
int main(void)
{
	/* Init board hardware. */
	BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();
    BOARD_UserLedsInit();
    RDC_Init(RDC);
#if (SSS_HAVE_HOSTCRYPTO_MBEDTLS)
    CRYPTO_InitHardware();
#endif /* defined(MBEDTLS) */

    // RDC for SBL only
    BOARD_InitBootTEE_Sbl();

    /* Print VTOR address to be aware of execution address. */
    PRINTF("\r\n\r\nVTOR address of the qmc2 Bootloader - SCB->VTOR: 0x%08X\r\n", SCB->VTOR);

    SCB_DisableDCache();

    (void)QMC2_BOOT_Main();

    return 0;
}

#if USE_DELAY_REQUIERD_FOR_HW_INIT

void EXAMPLE_GPT_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    GPT_ClearStatusFlags(EXAMPLE_GPT, kGPT_OutputCompare1Flag);

    gptIsrFlag = true;
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F, Cortex-M7, Cortex-M7F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U || __CORTEX_M == 7U)
    __DSB();
#endif
}


void SystemInitHook(void) {

#if USE_DELAY_REQUIERD_FOR_HW_INIT
	uint32_t gptFreq;
	gpt_config_t gptConfig;
	gptIsrFlag = false;

	/* Init OSC RC 16M */
	ANADIG_OSC->OSC_16M_CTRL |= ANADIG_OSC_OSC_16M_CTRL_EN_IRC4M16M_MASK;

	GPT_GetDefaultConfig(&gptConfig);
	gptConfig.clockSource = kGPT_ClockSource_Osc;
	gptConfig.enableRunInDbg = true;

	__DSB();

	/* Initialize GPT module */
	GPT_Init(EXAMPLE_GPT, &gptConfig);

	/* Divide GPT clock source frequency by 3 inside GPT module */
	GPT_SetClockDivider(EXAMPLE_GPT, 3);

	/* Get GPT clock frequency */
	gptFreq = 16000000U;

	/* GPT frequency is divided by 3 inside module */
	gptFreq /= 3;

	/* Set both GPT modules to 5 second duration */
	GPT_SetOutputCompareValue(EXAMPLE_GPT, kGPT_OutputCompare_Channel1, (gptFreq*5));

	/* Enable GPT Output Compare1 interrupt */
	GPT_EnableInterrupts(EXAMPLE_GPT, kGPT_OutputCompare1InterruptEnable);

	/* Enable at the Interrupt */
	EnableIRQ(GPT_IRQ_ID);

	__enable_irq();

	GPT_StartTimer(EXAMPLE_GPT);

	__DSB();

	while (gptIsrFlag == false);

	GPT_StopTimer(EXAMPLE_GPT);
	/* Enable GPT Output Compare1 interrupt */
	GPT_DisableInterrupts(EXAMPLE_GPT, kGPT_OutputCompare1InterruptEnable);
	/* Initialize GPT module */
	GPT_Deinit(EXAMPLE_GPT);

	__disable_irq();

	__DSB();

#endif

}

#endif

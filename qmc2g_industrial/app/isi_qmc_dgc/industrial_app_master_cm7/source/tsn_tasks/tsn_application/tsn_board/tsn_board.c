/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "fsl_clock.h"
#include "fsl_iomuxc.h"
#include "fsl_gpio.h"
#include "fsl_gpt.h"
#include "log.h"

#include "tsn_board.h"
#include "fsl_debug_console.h"
#include "board.h"

/*	BOARD_NetPhy1Reset
 *
 *  place of execution: flash
 *
 *  description: Initialize Ethernet PHY
 *
 *  params:
 *
 */
static void BOARD_NetPhy1Reset(void)
{
    /* For a complete PHY reset of RTL8211FDI-CG, this pin must be asserted low for at least 10ms. And
     * wait for a further 30ms(for internal circuits settling time) before accessing the PHY register */
    GPIO_WritePinOutput(GPIO6, 12, 0);
    SDK_DelayAtLeastUs(10000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    GPIO_WritePinOutput(GPIO6, 12, 1);
    SDK_DelayAtLeastUs(30000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
}


/*	ENET_QOS_SetSYSControl
 *
 *  place of execution: flash
 *
 *  description: Sets interconnection between MAC and PHY
 *
 *  params:     miiMode - selects between mii, rmii and rgmii
 *
 */
void ENET_QOS_SetSYSControl(enet_qos_mii_mode_t miiMode)
{
    if (miiMode == kENET_QOS_RgmiiMode)
    IOMUXC_GPR->GPR6 |= IOMUXC_GPR_GPR6_ENET_QOS_RGMII_EN_MASK; /* Set this bit to enable ENET_QOS RGMII TX clock output on TX_CLK pad. */

    IOMUXC_GPR->GPR6 |= (miiMode << 3U);
    IOMUXC_GPR->GPR6 |= IOMUXC_GPR_GPR6_ENET_QOS_CLKGEN_EN_MASK; /* Set this bit to enable ENET_QOS clock generation. */
}


/*	BOARD_InitEnetQosClock
 *
 *  place of execution: flash
 *
 *  description: Initialize ENET_QoS clocks
 *  				-ENET_QoS clock root
 *  				-ENET_Timer3 clock root
 *
 *  params:
 *
 */
void BOARD_InitEnetQosClock(void)
{
    clock_root_config_t rootCfg = {0};


    /* Generate 125M root clock for Enet QOS */
    /* 1G / 2 / 4 = 125M */
    rootCfg.mux = kCLOCK_ENET_QOS_ClockRoot_MuxSysPll1Div2;
    rootCfg.div = 4U;
    CLOCK_SetRootClock(kCLOCK_Root_Enet_Qos, &rootCfg);

    /* Generate 200M root clock for Enet Timer3 */
    /* 1G / 5 / 1 = 200M */
    rootCfg.mux = kCLOCK_ENET_QOS_ClockRoot_MuxSysPll1Div5;
    rootCfg.div = 1U;
    CLOCK_SetRootClock(kCLOCK_Root_Enet_Timer3, &rootCfg);
}


/*	BOARD_ConfigureForRunTimeStats
 *
 *  place of execution: flash
 *
 *  description: Initializes GPT timer used for statistics
 *
 *  params:
 *
 */
void BOARD_ConfigureForRunTimeStats(void)
{
    gpt_config_t gptConfig;

    GPT_GetDefaultConfig(&gptConfig);

    gptConfig.enableFreeRun = true;
    gptConfig.clockSource = kGPT_ClockSource_Periph;
    gptConfig.divider = 2048;

    /* Initialize GPT module */
    GPT_Init(BOARD_GPT_STATS, &gptConfig);

    /* Start Timer */
#if PRINT_LEVEL == VERBOSE_DEBUG
    PRINTF("Starting GPT timer(%x), frequency: %d\r\n", BOARD_GPT_STATS,
           CLOCK_GetRootClockFreq(kCLOCK_Root_Gpt4) / gptConfig.divider);
#endif
    GPT_StartTimer(BOARD_GPT_STATS);
}


/*	BOARD_GetRunTimeCounterValue
 *
 *  place of execution: ITCM
 *
 *  description: Returns GPT timer counter value. Used by FreeRTOS when
 *               configGENERATE_RUN_TIME_STATS macro is set
 *
 *  params:
 *
 */
uint32_t BOARD_GetRunTimeCounterValue()
{
    return GPT_GetCurrentTimerCount(BOARD_GPT_STATS);
}


/*	BOARD_InitNetInterfaces
 *
 *  place of execution: flash
 *
 *  description: Initialization of the network interfaces
 *
 *  params:
 *
 */
void BOARD_InitNetInterfaces(void)
{
    BOARD_InitEnetQosClock();
    BOARD_NetPhy1Reset();
}


/*	BOARD_InitNVIC
 *
 *  place of execution: flash
 *
 *  description: Initialize global interrupts related to TSN application
 *
 *  params:
 *
 */
void BOARD_InitNVIC(void)
{
 //   NVIC_SetPriority(ENET_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_SetPriority(ENET_1588_Timer_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
//    NVIC_SetPriority(ENET_1G_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
//    NVIC_SetPriority(ENET_1G_1588_Timer_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    NVIC_SetPriority(ENET_QOS_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
#if PRINT_LEVEL == VERBOSE_DEBUG
    NVIC_SetPriority(BOARD_UART_IRQ, configLIBRARY_LOWEST_INTERRUPT_PRIORITY);
#endif
    NVIC_SetPriority(GPT1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3);
    NVIC_SetPriority(GPT2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3);
    NVIC_SetPriority(GPT3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3);
}



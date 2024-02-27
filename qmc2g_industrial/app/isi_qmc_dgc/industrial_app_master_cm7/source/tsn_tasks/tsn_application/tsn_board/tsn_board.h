/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef TSN_TASKS_TSN_APPLICATION_TSN_BOARD_TSN_BOARD_H_
#define TSN_TASKS_TSN_APPLICATION_TSN_BOARD_TSN_BOARD_H_

#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_enet.h"
#include "fsl_enet_qos.h"
#include "stats_task.h"

#define BOARD_NUM_PORTS      1

#if CONFIG_STATS_TOTAL_CPU_LOAD
#define BOARD_IDLE_COUNT_PER_S 4317785.0F
#endif

#define BOARD_SAI_MCLK_HZ      12288000U
#define BOARD_GPT_1_CLK_EXT_FREQ    BOARD_SAI_MCLK_HZ

#define BOARD_GPT_1_CLK_SOURCE_TYPE kGPT_ClockSource_Periph

#define BOARD_GPT_STATS GPT4


void BOARD_InitNetInterfaces(void);
void BOARD_InitEnetQosClock(void);
void BOARD_InitNVIC(void);



#endif /* TSN_TASKS_TSN_APPLICATION_TSN_BOARD_TSN_BOARD_H_ */

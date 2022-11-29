/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef BOARD_SERVICE_TASKS_H
#define BOARD_SERVICE_TASKS_H

#include "mcdrv_gd3000.h"
#include "api_qmc_common.h"

/*!
 * @brief Initiates the Board Service Task.
 *
 */
qmc_status_t BoardServiceInit();

/*!
 * @brief Runs in an infinite loop. Reads GD3000 status registers, Temperatures on PSB, DB and MCU
 *
 * @param pvParameters unused
 */
void BoardServiceTask(void *pvParameters);

extern GD3000_t g_sM1GD3000;	/* Global Motor 1 GD3000 handle */
extern GD3000_t g_sM2GD3000;	/* Global Motor 2 GD3000 handle */
extern GD3000_t g_sM3GD3000;	/* Global Motor 3 GD3000 handle */
extern GD3000_t g_sM4GD3000;	/* Global Motor 4 GD3000 handle */

#endif /* BOARD_SERVICE_TASKS_H */

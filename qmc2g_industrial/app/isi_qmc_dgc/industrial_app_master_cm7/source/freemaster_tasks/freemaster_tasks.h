/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef FREEMASTER_TASKS_FREEMASTER_TASKS_H_
#define FREEMASTER_TASKS_FREEMASTER_TASKS_H_

#include "main_cm7.h"
#include "freemaster_serial_lpuart.h"
#include "api_motorcontrol_internal.h"

extern mc_motor_command_t g_sMotorCmdTst;
extern qmc_status_t eSetCmdStatus;

extern void init_freemaster_lpuart(void);
extern void FreemasterTask(void *pvParameters);

#endif /* FREEMASTER_TASKS_FREEMASTER_TASKS_H_ */

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
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

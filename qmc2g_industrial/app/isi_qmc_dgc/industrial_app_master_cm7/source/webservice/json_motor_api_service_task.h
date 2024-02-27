/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "api_qmc_common.h"
#include "api_motorcontrol.h"
#ifndef JSON_MOTOR_API_SERVICE_TASK_H
#define JSON_MOTOR_API_SERVICE_TASK_H
#include "FreeRTOS.h"
#include "projdefs.h"
#include "semphr.h"

/*!
 * @brief The Motor API service task. It updates motor status information for the motor API of the web service.
 *
 * @param pvParameters Unused.
 */
void JsonMotorAPIServiceTask(void *pvParameters);
extern mc_motor_status_t g_api_motor_status[4];
extern TaskHandle_t json_motor_api_service_task_handle;
extern SemaphoreHandle_t g_api_motor_semaphore;
#endif

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef WEBSERVICE_SERVICE_TASK_H
#define WEBSERVICE_SERVICE_TASK_H

#include <FreeRTOS.h>
#include <task.h>

#include "api_configuration.h"

TaskHandle_t g_webservice_logging_task_handle;

/**
 * @brief increment 4xx-5xx status code error counters
 *
 * @param status_code HTTP status code
 * 
 * will ignore other status codes
 */
void Webservice_IncrementErrorCounter(int status_code,config_id_t user);


/*!
 * @brief The Webservice Service Task logs accumulated webserver errors.
 *
 * @param pvParameters Unused.
 */

void WebserviceLoggingTask(void *pvParameters);

#endif

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CLOUD_SERVICE_CLOUD_SERVICE_H_
#define CLOUD_SERVICE_CLOUD_SERVICE_H_

#include <stdbool.h>

#include "qmc_features_config.h"
#include "api_qmc_common.h"

/*!
 * @brief The task for the pre-configured cloud service.
 *
 * Running the task includes fetching the parameters from configuration, connecting, and starting
 * the publishing task. Should be only called after the network interface (LWIP) is up and the
 * SE05x session is set up. LWIP should be configured to support DNS.
 */
void CloudServiceTask(void *pvParameters);

/*!
 * @brief Checks the state of the cloud service.
 *
 * @return TRUE if connected and running, FALSE otherwise.
 */
bool CLOUD_IsServiceRunning(void);
#endif /* FEATURE_CLOUD_AZURE_IOTHUB || FEATURE_CLOUD_GENERIC_MQTT */


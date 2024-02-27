/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "json_motor_api_service_task.h"
#include "api_qmc_common.h"
#include "fsl_common.h"
#include "qmc_features_config.h"

TaskHandle_t g_json_motor_api_service_task_handle;

mc_motor_status_t g_api_motor_status[4] = {0};
SemaphoreHandle_t g_api_motor_semaphore = NULL;
static StaticSemaphore_t xApiMotorSemaphoreBuffer;
/*!
 * @brief The Motor API service task. It updates motor status information for the motor API of the web service.
 *
 * @param pvParameters Unused.
 */
void JsonMotorAPIServiceTask(void *pvParameters)
{
    mc_motor_status_t mstatus      = {0};
    qmc_status_t status;
    qmc_msg_queue_handle_t *mqueue = NULL;

    g_api_motor_semaphore = xSemaphoreCreateMutexStatic(&xApiMotorSemaphoreBuffer);
    status                = MC_GetNewStatusQueueHandle(&mqueue, 10);

    g_api_motor_status[0].sFast.eMotorState = kMC_Init;
    g_api_motor_status[1].sFast.eMotorState = kMC_Init;
    g_api_motor_status[2].sFast.eMotorState = kMC_Init;
    g_api_motor_status[3].sFast.eMotorState = kMC_Init;

    if (status == kStatus_QMC_Ok && g_api_motor_semaphore)
    {
        for (;;)
        {
            /* Motor Status - begin */
            status = MC_DequeueMotorStatus(mqueue, 0, &mstatus);

            if (status != kStatus_QMC_ErrNoMsg)
            {
                if (status == kStatus_QMC_Ok && mstatus.eMotorId < ARRAY_SIZE(g_api_motor_status) &&
                    mstatus.eMotorId < MC_MAX_MOTORS)
                {
                    mc_motor_status_t *target_motor = &g_api_motor_status[mstatus.eMotorId];
                    /* copy over the motor status, ensure we are not interrupted by any other task. */
                    /* (especially the lwip task) */

                    /* a critical section should be fast enough. */
                    if (xSemaphoreTake(g_api_motor_semaphore, pdMS_TO_TICKS(MOTOR_STATUS_AND_LOGS_DELAY_AT_LEAST_MS)) ==
                        pdTRUE)
                    {
                        *target_motor = mstatus; /* copy over the struct */
                        xSemaphoreGive(g_api_motor_semaphore);
                    }
                }
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(MOTOR_STATUS_AND_LOGS_DELAY_AT_LEAST_MS));
            }
        }
    }
    vTaskSuspend(NULL);
}

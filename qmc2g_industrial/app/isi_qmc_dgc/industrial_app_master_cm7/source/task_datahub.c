/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "stdbool.h"
#include "api_qmc_common.h"
#include "api_motorcontrol.h"
#include "api_motorcontrol_internal.h"
#include "api_logging.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "timers.h"
#include "semphr.h"
#include "qmc_features_config.h"



/*******************************************************************************
 * Compile time checks
 ******************************************************************************/

#if( configUSE_16_BIT_TICKS != 0 )
    #error "DataHub task requires 32 bit ticks in order to allow for 24 bits per event group."
#endif

#if( DATAHUB_MAX_STATUS_QUEUES > 22 )
    /* 24 events per event group; first bit used for motor command queue */
    #error "Max. number of motor status queues must not exceed 22"
#endif



/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DATAHUB_COMMAND_QUEUE_ITEM_SIZE     (sizeof(mc_motor_command_t))
#define DATAHUB_STATUS_QUEUE_ITEM_SIZE      (sizeof(mc_motor_status_t))
#define DATAHUB_EVENTBIT_COMMAND_QUEUE      (1 << 0)
#define DATAHUB_EVENTBIT_STATUS_TIMER       (1 << 1)
#define DATAHUB_EVENTBIT_FIRST_STATUS_QUEUE (1 << 2)



/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void DataHubTask(void *pvParameters);
qmc_status_t DataHubInit(void);
static void statusSamplingTimerCallback(TimerHandle_t xTimer);



/*******************************************************************************
 * Globals
 ******************************************************************************/

bool                   g_isInitialized_DataHub = false;
uint32_t               g_motorStatusQueuePrescalers[DATAHUB_MAX_STATUS_QUEUES] = { 1U };
uint32_t               g_motorStatusQueuePrescalerCounters[DATAHUB_MAX_STATUS_QUEUES] = { 1U };
QueueHandle_t          g_motorCommandQueue = NULL;
QueueHandle_t          g_motorStatusQueues[DATAHUB_MAX_STATUS_QUEUES] = { NULL };
qmc_msg_queue_handle_t g_motorStatusQueueHandles[DATAHUB_MAX_STATUS_QUEUES] = { {NULL, NULL, 0} };
EventGroupHandle_t     g_motorQueueEventGroupHandle;
const EventBits_t      g_motorCommandQueueEventBit = DATAHUB_EVENTBIT_COMMAND_QUEUE;
TimerHandle_t          g_statusSamplingTimerHandle;
SemaphoreHandle_t      g_statusQueueMutexHandle;

static StaticQueue_t gs_commandQueue;
static StaticQueue_t gs_statusQueues[DATAHUB_MAX_STATUS_QUEUES];
static StaticEventGroup_t gs_motorQueueEventGroup;
static uint8_t       gs_commandQueueBuffer[DATAHUB_COMMAND_QUEUE_ITEM_SIZE * DATAHUB_COMMAND_QUEUE_LENGTH];
static uint8_t       gs_statusQueueBuffers[DATAHUB_MAX_STATUS_QUEUES][DATAHUB_STATUS_QUEUE_ITEM_SIZE * DATAHUB_STATUS_QUEUE_LENGTH];
static StaticTimer_t gs_statusSamplingTimer;
static StaticSemaphore_t gs_statusQueueMutex;

TaskHandle_t g_datahub_task_handle;
/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Initializes the DataHub task / Motor API.
 */
qmc_status_t DataHubInit(void)
{
	int i;

	/* initialize event group for queue events */
	g_motorQueueEventGroupHandle = xEventGroupCreateStatic(&gs_motorQueueEventGroup);
	if( NULL == g_motorQueueEventGroupHandle )
		return kStatus_QMC_Err;

	/* initialize message queues */
	g_motorCommandQueue = xQueueCreateStatic(DATAHUB_COMMAND_QUEUE_LENGTH, DATAHUB_COMMAND_QUEUE_ITEM_SIZE, gs_commandQueueBuffer, &gs_commandQueue);
    if( NULL == g_motorCommandQueue )
    	return kStatus_QMC_Err;
    vQueueAddToRegistry(g_motorCommandQueue, "motor commands");

    for(i=0; i<DATAHUB_MAX_STATUS_QUEUES; i++)
    {
    	g_motorStatusQueues[i] = xQueueCreateStatic(DATAHUB_STATUS_QUEUE_LENGTH, DATAHUB_STATUS_QUEUE_ITEM_SIZE, gs_statusQueueBuffers[i], &gs_statusQueues[i]);
        if( NULL == g_motorStatusQueues[i] )
        	return kStatus_QMC_Err;
        g_motorStatusQueueHandles[i].queueHandle = NULL;
        g_motorStatusQueueHandles[i].eventHandle = &g_motorQueueEventGroupHandle;
        g_motorStatusQueueHandles[i].eventMask   = (DATAHUB_EVENTBIT_FIRST_STATUS_QUEUE << i);
        vQueueAddToRegistry(g_motorStatusQueues[i], "motor status");
    }

    /* initialize mutex for queue handling protection */
    g_statusQueueMutexHandle = xSemaphoreCreateMutexStatic(&gs_statusQueueMutex);
    if( NULL == g_statusQueueMutexHandle )
    	return kStatus_QMC_Err;

    /* initialize timer for sampling motor status values */
    g_statusSamplingTimerHandle = xTimerCreateStatic("DataHub sampling", pdMS_TO_TICKS(DATAHUB_STATUS_SAMPLING_INTERVAL_MS),
    					                             pdTRUE, NULL, statusSamplingTimerCallback, &gs_statusSamplingTimer);

	g_isInitialized_DataHub = true;
	return kStatus_QMC_Ok;
}


/*!
 * @brief DataHub task
 *
 * The Data Hub task bridges best-effort and real-time parts of the motor control functionality.
 * It terminates one end of the message queues of the MotorAPI. Motor commands are de-queued
 * when they become available and are transferred to the shared memory (and hence to the motor
 * control algorithm). The current motor status for each motor is polled in a regular interval
 * and then pushed to each subscribed task through the corresponding registered message queue.
 * New motor status messages in the queue are announced by emitting the event MOTOR_STATUS_EVENT.
 */
void DataHubTask(void *pvParameters)
{
	while(!g_isInitialized_DataHub)
		vTaskDelay(pdMS_TO_TICKS(10));

	while (1)
	{
		EventBits_t events;
        events =  xEventGroupWaitBits(g_motorQueueEventGroupHandle, (DATAHUB_EVENTBIT_COMMAND_QUEUE | DATAHUB_EVENTBIT_STATUS_TIMER), pdFALSE, pdFALSE, portMAX_DELAY);

        /* process motor command events */
        if(events & DATAHUB_EVENTBIT_COMMAND_QUEUE)
        {
        	mc_motor_command_t cmd;

        	if(pdTRUE == xQueuePeek(g_motorCommandQueue, &cmd, 0))
        	{
        		/* execute motor command */
        		qmc_status_t status = MC_SetMotorCommand(&cmd);

        		if(kStatus_QMC_ErrInterrupted != status) /* if command was executed or failed */
        		{
        			/* pop command from queue */
        			xQueueReceive(g_motorCommandQueue, &cmd, 0);

        			/* clear event, if no commands are pending */
        			if( 0 == uxQueueMessagesWaiting(g_motorCommandQueue))
        				xEventGroupClearBits(g_motorQueueEventGroupHandle, DATAHUB_EVENTBIT_COMMAND_QUEUE);

        			if (kStatus_QMC_ErrBusy == status)
        			{
        				/* motor commands have been frozen and are being ignored */
        				continue;
        			}
        		    /* in case of an error, write a log message */
        			else if(kStatus_QMC_Ok != status)
        			{
                        log_record_t logEntryWithId = {
                            .rhead = {
                                .chksum = 0,
                                .uuid   = 0,
                                .ts = {
                                    .seconds      = 0,
                                    .milliseconds = 0
                                }
                            },
                            .type                           = kLOG_FaultDataWithID,
                            .data.faultDataWithID.source    = LOG_SRC_MotorControl,
                            .data.faultDataWithID.category  = LOG_CAT_General,
                            .data.faultDataWithID.motorId   = cmd.eMotorId,
							.data.faultDataWithID.eventCode = LOG_EVENT_QueueingCommandFailedInternal
                        };
                        LOG_QueueLogEntry(&logEntryWithId, false);
        			}
        			else
        			{
        				;
        			}
        		}
        	}
        }

        /* process status sampling timer events */
        if(events & DATAHUB_EVENTBIT_STATUS_TIMER)
        {
        	int i, k;
        	mc_motor_status_t motorStatus[MC_MAX_MOTORS];

        	/* retrieve motor status values */
        	for(k=0; k<MC_MAX_MOTORS; k++)
        	{
        		MC_GetMotorStatus(k, motorStatus+k);
        		motorStatus[k].eMotorId = k;
        	}

        	/* find active queues */
            for( i=0; i<DATAHUB_MAX_STATUS_QUEUES; i++)
            {
            	/* if queue is active and counter has expired */
            	if((g_motorStatusQueueHandles[i].queueHandle != NULL) && (0 == --g_motorStatusQueuePrescalerCounters[i]))
            	{
            		g_motorStatusQueuePrescalerCounters[i] = g_motorStatusQueuePrescalers[i]; /* reset counter */

            		/* send status values */
            		for(k=0; k<MC_MAX_MOTORS; k++)
            			xQueueSend(*(g_motorStatusQueueHandles[i].queueHandle), motorStatus+k, 0);

            		/* trigger corresponding new-status-event */
            		xEventGroupSetBits(*(g_motorStatusQueueHandles[i].eventHandle), g_motorStatusQueueHandles[i].eventMask);
            	}
            }

            /* clear timer event */
            xEventGroupClearBits(g_motorQueueEventGroupHandle, DATAHUB_EVENTBIT_STATUS_TIMER);
        }
	}
}

static void statusSamplingTimerCallback(TimerHandle_t xTimer)
{
	xEventGroupSetBits(g_motorQueueEventGroupHandle, DATAHUB_EVENTBIT_STATUS_TIMER);
}

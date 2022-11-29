/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "board.h"
#include "tsn_board.h"
#include "clock_config.h"
#include "stats.h"

#include "FreeRTOS.h"
#include "stats_task.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STATS_TASK_NAME       "stats"
#define STATS_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 185)
#define STATS_TASK_PRIORITY   5

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern unsigned int malloc_failed_count;
extern uint32_t idleCounter;

static struct StatsTask_Ctx StatsTask;

/*******************************************************************************
 * Code
 ******************************************************************************/

#if CONFIG_STATS_HEAP
/*	STATS_Heap
 *
 *  place of execution: ITCM
 *
 *  description: prints information about free and used heap size into console
 *               based on information from FreeRTOS
 *
 *  params:     no params
 *
 */
static void STATS_Heap(void)
{
    size_t FreeHeap, UsedHeap;

    FreeHeap = xPortGetFreeHeapSize();
    UsedHeap = configTOTAL_HEAP_SIZE - FreeHeap;

    PRINTF("Heap used: %u, free: %u, min free: %u\n",
           UsedHeap, FreeHeap,
           xPortGetMinimumEverFreeHeapSize());

    PRINTF("Malloc failed counter: %u\n\n", malloc_failed_count);
}
#endif

#if CONFIG_STATS_TOTAL_CPU_LOAD
/*	STATS_Heap
 *
 *  place of execution: ITCM
 *
 *  description: prints information about CPU usage based on BOARD_IDLE_COUNT_PER_S macro
 *
 *  params:     periodMs - used to compute CPU load
 *
 */
static void STATS_TotalCPULoad(unsigned int periodMs)
{
    static uint32_t lastIdleCounter = 0;
    uint32_t idleCnt = idleCounter;

    PRINTF("Total CPU load : %5.2f\n\n",
           100.0 - ((idleCnt - lastIdleCounter) / (BOARD_IDLE_COUNT_PER_S * (periodMs / 1000.0))) * 100.0);

    lastIdleCounter = idleCnt;
}
#endif

/*	STATS_TaskPeriodic
 *
 *  place of execution: ITCM
 *
 *  description: calls functions to print HEAP and CPU stats
 *
 *  params:     *Ctx - StatsTask_Ctx context (main context)
 */
static void STATS_TaskPeriodic(struct StatsTask_Ctx *Ctx)
{
    if (Ctx->PeriodicFn)
        Ctx->PeriodicFn(Ctx->PeriodicData);


#if CONFIG_STATS_HEAP
    STATS_Heap();
#endif

#if CONFIG_STATS_TOTAL_CPU_LOAD
    STATS_TotalCPULoad(Ctx->PeriodMs);
#endif

}


/*	STATS_Async
 *
 *  place of execution: ITCM
 *
 *  description: send data into the message queue
 *
 *  params:     (*Func)(void *Data) - pointer to function which will print the data into console
 *  			*Data - data, which will be printed
 */
int STATS_Async(void (*Func)(void *Data), void *Data)
{
    struct Async_Ctx *Ctx = &StatsTask.Async;
    struct Async_Msg Msg;

    Msg.Func = Func;
    Msg.Data = Data;

    return xQueueSend(Ctx->qHandle, &Msg, 0);
}


#if CONFIG_STATS_ASYNC
/*	STATS_AsyncInit
 *
 *  place of execution: flash
 *
 *  description: creates Queue which is used to store async messages. Queue size is defined
 *               by ASYNC_NUM_MSG macro
 *
 *  params:     *Ctx - pointer to Async_Ctx which is part of StatsTask_Ctx
 *
 */
static int STATS_AsyncInit(struct Async_Ctx *Ctx)
{
    Ctx->qHandle = xQueueCreateStatic(ASYNC_NUM_MSG, sizeof(struct Async_Msg),
                                      Ctx->qBuffer, &Ctx->qData);
    if (!Ctx->qHandle)
        goto err;

    return 0;

    err:
    	return -1;
}


/*	STATS_AsyncProcess
 *
 *  place of execution: ITCM
 *
 *  description: Receive data from message queue and print into the console
 *
 *  params:     *Ctx - pointer to Async_Ctx which is part of StatsTask_Ctx
 *  			*WaitMs - time to wait in ms
 */
static void STATS_AsyncProcess(struct Async_Ctx *Ctx, unsigned int WaitMs)
{
    struct Async_Msg Msg;
    TickType_t Last, Now;
    unsigned int Elapsed, Timeout;

    Timeout = pdMS_TO_TICKS(WaitMs);
    Last = xTaskGetTickCount();

    while (true) {
        if (xQueueReceive(Ctx->qHandle, &Msg, Timeout) == pdPASS)
        {
            Msg.Func(Msg.Data);

            Now = xTaskGetTickCount();
            Elapsed = Now - Last;

            if (Elapsed < Timeout)
            {
                Timeout -= Elapsed;
                Last = Now;
            }
            else
                break;
        }
        else
            break;
    }
}
#endif


/*	STATS_Task
 *
 *  place of execution: ITCM
 *
 *  description: stats task MAIN FUNCTION
 *               calls Async process which receives messages from queue
 *               calls Task Periodic which prints HEAP and CPU stats
 *
 *  params:     *pvParameters - any parameters
 */
static void STATS_Task(void *pvParameters)
{
    struct StatsTask_Ctx *Ctx = pvParameters;

    while (true) {

        STATS_AsyncProcess(&Ctx->Async, Ctx->PeriodMs);

        STATS_TaskPeriodic(Ctx);
    }
}


/*	STATS_TaskInit
 *
 *  place of execution: flash
 *
 *  description: if CONFIG_STATS_ASYNC, initialize asynch mechanism for messages
 *               creates task called "stats" which prints messages into console
 *
 *  params:     *PeriodicFn - pointer to user function
 *  			*Data - pointer to user data
 *  			 PeriodMs - period for internal CPU usage function
 */
int STATS_TaskInit(void (*PeriodicFn)(void *Data), void *Data, unsigned int PeriodMs)
{
    struct StatsTask_Ctx *Ctx = &StatsTask;

    Ctx->PeriodicFn = PeriodicFn;
    Ctx->PeriodicData = Data;
    Ctx->PeriodMs = PeriodMs;

#if CONFIG_STATS_ASYNC
    if (STATS_AsyncInit(&Ctx->Async) < 0)
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("STATS_AsyncInit failed \n");
#endif
#endif

    if (xTaskCreate(STATS_Task, STATS_TASK_NAME,
                    STATS_TASK_STACK_SIZE, Ctx,
                    STATS_TASK_PRIORITY, NULL) != pdPASS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("xTaskCreate(%s) failed\n", STATS_TASK_NAME);
#endif
        goto exit;
    }

    return 0;

exit:
    return -1;
}




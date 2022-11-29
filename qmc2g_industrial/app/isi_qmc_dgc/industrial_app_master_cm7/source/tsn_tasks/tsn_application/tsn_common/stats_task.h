/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _STATS_H_
#define _STATS_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "log.h"

#define ASYNC_NUM_MSG       16

#if PRINT_LEVEL == VERBOSE_DEBUG
#define CONFIG_STATS_HEAP           1
#else
#define CONFIG_STATS_HEAP           0
#endif
#define CONFIG_STATS_TOTAL_CPU_LOAD 0
#define CONFIG_STATS_ASYNC          1


struct Async_Msg {
    void (*Func)(void *Data);
    void *Data;
};

struct Async_Ctx {
    QueueHandle_t qHandle;
    uint8_t qBuffer[ASYNC_NUM_MSG * sizeof(struct Async_Msg)];
    StaticQueue_t qData;
};

struct StatsTask_Ctx {

    struct Async_Ctx Async;
    void (*PeriodicFn)(void *Data);
    void *PeriodicData;
    unsigned int PeriodMs;
};

int STATS_TaskInit(void (*PeriodicFn)(void *data), void *Data, unsigned int PeriodMs);
int STATS_Async(void (*Func)(void *Data), void *Data);


#endif /* _STATS_H_*/

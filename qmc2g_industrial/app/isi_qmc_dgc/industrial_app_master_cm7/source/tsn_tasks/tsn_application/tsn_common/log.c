/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "log.h"

uint64_t app_log_time_s;
unsigned int app_log_level = VERBOSE_INFO;


/*	app_log_update_time
 *
 *  place of execution: flash
 *
 *  description: updates time in nanoseconds for selected clock ID
 *
 *  params:   genavb_clock_id_t clk_id
 *
 */
void app_log_update_time(genavb_clock_id_t clk_id)
{
    uint64_t log_time;

    if (genavb_clock_gettime64(clk_id, &log_time) < 0)
        return;

    app_log_time_s = log_time / NSECS_PER_SEC;
}


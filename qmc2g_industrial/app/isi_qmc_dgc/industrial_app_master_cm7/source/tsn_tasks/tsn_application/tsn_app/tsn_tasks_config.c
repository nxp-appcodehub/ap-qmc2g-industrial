/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "cyclic_task.h"
#include "tsn_tasks_config.h"

struct tsn_stream tsn_streams[MAX_TSN_STREAMS] = {
    [0] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = 0,
            .priority = ISOCHRONOUS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
    [1] = {
        .address = {
            .ptype = PTYPE_L2,
            .vlan_id = 0,
            .priority = ISOCHRONOUS_DEFAULT_PRIORITY,
            .u.l2 = {
                .dst_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                .protocol = htons(ETHERTYPE_MOTOROLA),
            },
        },
    },
};

#define CYCLIC_TASK_DEFAULT_PARAMS(offset)            \
    {                                                 \
        .clk_id = GENAVB_CLOCK_GPTP_0_0,              \
        .priority = TASK_DEFAULT_PRIORITY,            \
        .stack_depth = TASK_DEFAULT_STACK_SIZE,       \
        .task_period_ns = APP_PERIOD_DEFAULT,         \
        .task_period_offset_ns = offset,              \
        .transfer_time_ns = NET_DELAY_OFFSET_DEFAULT, \
        .use_st = 1,                                  \
        .use_fp = 0,                                  \
        .sched_traffic_offset = SCHED_TRAFFIC_OFFSET, \
        .rx_buf_size = PACKET_SIZE,                   \
        .tx_buf_size = PACKET_SIZE,                   \
    }

struct cyclic_task cyclic_tasks[MAX_TASK_CONFIGS] = {

    [0] = {
        .type = CYCLIC_IO_DEVICE,
        .id = IO_DEVICE_0,
        .params = CYCLIC_TASK_DEFAULT_PARAMS(NET_DELAY_OFFSET_DEFAULT),
        .num_peers = 1,
        .rx_socket = {
            [0] = {
                .peer_id = (CONTROLLER_0-2),
                .stream_id = 0,
            },
        },
        .tx_socket = {
            .stream_id = 1,
        },
    },
};

/*	tsn_conf_get_stream
 *
 *  place of execution: flash
 *
 *  description: Returns stream with selected index from tsn_streams structure.
 *               Called during cyclic task initialization
 *
 *  params:      index - selects stream from the structure
 *
 */
struct tsn_stream *tsn_conf_get_stream(int index)
{
    if (index >= (sizeof(tsn_streams) / sizeof(struct tsn_stream)))
        return NULL;

    return &tsn_streams[index];
}


/*	tsn_conf_get_cyclic_task
 *
 *  place of execution: flash
 *
 *  description: Returns cyclic task with selected index from cyclic_tasks structure.
 *               Called in TsnInitTask. Currently only one task exists (IO_Device)
 *
 *  params:      index - selects task from the structure
 *
 */
struct cyclic_task *tsn_conf_get_cyclic_task(int index)
{
    struct cyclic_task *c_task;

    if (index >= (sizeof(cyclic_tasks) / sizeof(struct cyclic_task)))
        return NULL;

    c_task = &cyclic_tasks[index];

    return c_task;
}


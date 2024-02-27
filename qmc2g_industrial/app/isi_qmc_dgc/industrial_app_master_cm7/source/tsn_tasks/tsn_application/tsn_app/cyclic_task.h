/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CYCLIC_TASK_H_
#define _CYCLIC_TASK_H_

#include "tsn_task.h"
#include "tsn_tasks_config.h"


#define CYCLIC_STAT_PERIOD_SEC 5

struct socket_stats {
    bool pending;
    unsigned int valid_frames;
    unsigned int err_id;
    unsigned int err_ts;
    unsigned int err_underflow;
    int link_status;
    unsigned int traffic_latency_max;
    unsigned int traffic_latency_min;
    struct stats traffic_latency;
    struct hist traffic_latency_hist;
};

struct socket {
    int peer_id;
    int stream_id;
    struct socket_stats stats;
    struct socket_stats stats_snap;
    struct net_socket *net_sock;
};

struct cyclic_task {
    struct tsn_task *task;
    struct tsn_task_params params;
    int type;
    uint16_t id;
    int num_peers;
    struct socket rx_socket[MAX_PEERS];
    struct socket tx_socket;
    void (*net_rx_func)(void *ctx, int msg_id, int src_id, void *buf, int len);
    void (*loop_func)(void *ctx, int timer_status);
    void *ctx;
};


int cyclic_task_init(struct cyclic_task *c_task,
                     void (*net_rx_func)(void *ctx, int msg_id, int src_id, void *buf, int len),
                     void (*loop_func)(void *ctx, int timer_status), void *ctx);
int cyclic_task_start(struct cyclic_task *);
void cyclic_task_stop(struct cyclic_task *);
int cyclic_net_transmit(struct cyclic_task *c_task, uint16_t msg_id, void *buf, uint16_t len);
void cyclic_task_set_period(struct cyclic_task *c_task, unsigned int period_ns);

#endif /* _CYCLIC_TASK_H_ */

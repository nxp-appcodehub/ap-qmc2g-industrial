/*
 * Copyright 2019, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cyclic_task.h"
#include "tsn_tasks_config.h"

#include "stats_task.h"
#include "log.h"
#include "types.h"
#include "tsn_tasks.h"

/*	socket_stats_print
 *
 *  place of execution: ITCM
 *
 *  description: Compute statistics and print values into the console.
 *
 *
 *  params:      data - pointer to data, which will be print
 *
 */
static void socket_stats_print(void *data)
{
    struct socket *sock = data;

    stats_compute(&sock->stats_snap.traffic_latency);

#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("cyclic rx socket(%p) net_sock(%p) peer id: %d\n", sock, sock->net_sock, sock->peer_id);
    INF("valid frames  : %u\n", sock->stats_snap.valid_frames);
    INF("err id        : %u\n", sock->stats_snap.err_id);
    INF("err ts        : %u\n", sock->stats_snap.err_ts);
    INF("err underflow : %u\n", sock->stats_snap.err_underflow);
    INF("link %s\n", sock->stats_snap.link_status ? "up" : "down");

    stats_print(&sock->stats_snap.traffic_latency);
    hist_print(&sock->stats_snap.traffic_latency_hist);
#endif
    sock->stats_snap.pending = false;
}


/*	socket_stats_dump
 *
 *  place of execution: ITCM
 *
 *  description: Copy stats to stats_snap, reset statistics, call STATS_Async and pass the
 *               printing function and data
 *
 *
 *  params:      sock - pointer to data, which will be print
 *
 */
static void socket_stats_dump(struct socket *sock)
{
    if (sock->stats_snap.pending)
        return;

    memcpy(&sock->stats_snap, &sock->stats, sizeof(struct socket_stats));
    stats_reset(&sock->stats.traffic_latency);
    sock->stats_snap.pending = true;

    if (STATS_Async(socket_stats_print, sock) != pdTRUE)
        sock->stats_snap.pending = false;
}


/*	cyclic_stats_dump
 *
 *  place of execution: ITCM
 *
 *  description: Calls socket_stats_dump which pass the data to STATS_Async
 *
 *
 *  params:      c_task - cyclic task handle
 *
 */
static void cyclic_stats_dump(struct cyclic_task *c_task)
{
    int i;

    for (i = 0; i < c_task->num_peers; i++)
        socket_stats_dump(&c_task->rx_socket[i]);
}


/*	timer_callback
 *
 *  place of execution: ITCM
 *
 *  description: Used to send notification and wake-up TSN task.
 *
 *
 *  params:      data - cyclic task handle
				 count -
 *
 */
static void timer_callback(void *data, int count)
{
    struct tsn_task *task = (struct tsn_task *)data;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (count < 0)
        task->clock_discont = 1;

    vTaskNotifyGiveFromISR(task->handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*	cyclic_net_receive
 *
 *  place of execution: ITCM
 *
 *  description: Called from main cyclic function. Receives data using tsn_net_receive_sock.
 *               Check, if correctly received data. Saves common header from socket and inspects
 *               sched_time. If incorrect, increase appropriate counters and tries to receive
 *               again.
 *               If correct, updates status and history, update counters and call
 *               receive function callback.
 *
 *
 *  params:      c_task - cyclic task handle
 *
 */
static void cyclic_net_receive(struct cyclic_task *c_task)
{
    int i;
    int status;
    struct socket *sock;
    struct tsn_task *task = c_task->task;
    struct tsn_common_hdr *hdr;
    int rx_frame;
    int32_t traffic_latency;

    for (i = 0; i < c_task->num_peers; i++) {
        sock = &c_task->rx_socket[i];
        rx_frame = 0;
    retry:
        status = tsn_net_receive_sock(sock->net_sock);
        if (status == NET_NO_FRAME && !rx_frame)
        {
            sock->stats.err_underflow++;
        }
        if (status != NET_OK) {
            if (status == NET_ERR) {
                sock->stats.link_status = 0;
            }
            else {
                sock->stats.link_status = 1;
            }
            continue;
        }

        rx_frame = 1;

        hdr = tsn_net_sock_buf(sock->net_sock);
        if (hdr->sched_time != (tsn_task_get_time(task) - task->params->transfer_time_ns)) {
            sock->stats.err_ts++;
            goto retry;
        }

        if (hdr->src_id != sock->peer_id) {
            sock->stats.err_id++;
            goto retry;
        }

        traffic_latency = sock->net_sock->ts - hdr->sched_time;

        stats_update(&sock->stats.traffic_latency, traffic_latency);
        hist_update(&sock->stats.traffic_latency_hist, traffic_latency);

        if (traffic_latency > sock->stats.traffic_latency_max)
            sock->stats.traffic_latency_max = traffic_latency;

        if (traffic_latency < sock->stats.traffic_latency_min)
            sock->stats.traffic_latency_min = traffic_latency;

        sock->stats.valid_frames++;
        sock->stats.link_status = 1;

        if (c_task->net_rx_func)
            c_task->net_rx_func(c_task->ctx, hdr->msg_id, hdr->src_id,
                                hdr + 1, hdr->len);
    }
}

/*	main_cyclic
 *
 *  place of execution: ITCM
 *
 *  description: Function prepares header which contains msg_id, src_id, sched_time and length,
 *               copies data into buffer and call tsn_net_transmit_sock which sends the socket.
 *
 *
 *  params:      c_task - cyclic task handle
 *               msg_id - message id
 *               *buf - pointer to buffer which stores the data
 *               len - length of the buffer
 *
 */
int cyclic_net_transmit(struct cyclic_task *c_task, uint16_t msg_id, void *buf, uint16_t len)
{
    struct tsn_task *task = c_task->task;
    struct socket *sock = &c_task->tx_socket;
    struct tsn_common_hdr *hdr = tsn_net_sock_buf(sock->net_sock);
    int total_len = (len + sizeof(*hdr));
    int status;

    if (total_len >= task->params->tx_buf_size)
        goto err;

    hdr->msg_id = msg_id;
    hdr->src_id = c_task->id;
    hdr->sched_time = tsn_task_get_time(task);
    hdr->len = len;

    if (len)
        memcpy(hdr + 1, buf, len);

    sock->net_sock->len = total_len;

    status = tsn_net_transmit_sock(sock->net_sock);
    if (status != NET_OK)
        goto err;

    return 0;

err:
    return -1;
}


/*	main_cyclic
 *
 *  place of execution: ITCM
 *
 *  description: TSN task main function. Calls every 1ms.
 *               Start statistics (schedule error, processing time, total time).
 *               Calls cyclic_net_receive which receives the frame.
 *               Main loop function is called and statistics are end.
 *               Once in few seconds, statistics are printed into the console.
 *
 *
 *
 *
 *  params:      data - user data passed to the function
 *
 */
static void main_cyclic(void *data)
{
    struct cyclic_task *c_task = data;
    struct tsn_task *task = c_task->task;
    uint32_t notify;
    TickType_t timeout = pdMS_TO_TICKS(2000);
    unsigned int num_sched_stats = CYCLIC_STAT_PERIOD_SEC * (NSECS_PER_SEC / task->params->task_period_ns);

    while (true) {
        /*
         * Wait to be woken-up by the timer
         */
        notify = ulTaskNotifyTake(pdTRUE, timeout);
        if (!notify) {
            if (c_task->loop_func)
                c_task->loop_func(c_task->ctx, -1);

            cyclic_task_stop(c_task);
            cyclic_task_start(c_task);
            task->stats.sched_timeout++;
            continue;
        }

        if (task->clock_discont) {
            if (c_task->loop_func)
                c_task->loop_func(c_task->ctx, -1);

            task->clock_discont = 0;
            cyclic_task_stop(c_task);
            cyclic_task_start(c_task);
            task->stats.clock_discont++;
            continue;
        }

        tsn_task_stats_start(task);

        /*
         * Receive, frames should be available
         */
        cyclic_net_receive(c_task);

        /*
         * Main loop
         */
        if (c_task->loop_func)
            c_task->loop_func(c_task->ctx, 0);

        tsn_task_stats_end(task);



        if (!(task->stats.sched % num_sched_stats)) {
            app_log_update_time(task->params->clk_id);
            tsn_stats_dump(task);
            cyclic_stats_dump(c_task);
        }
    }
}


/*	cyclic_task_set_period
 *
 *  place of execution: flash
 *
 *  description: Sets task period and period offset according to the task type
 *
 *
 *  params:      c_task - cyclic task handle
 *  			 period_ns - sets task period (1ms in our application)
 *
 */
void cyclic_task_set_period(struct cyclic_task *c_task, unsigned int period_ns)
{
    struct tsn_task_params *params = &c_task->params;

    params->task_period_ns = period_ns;

    if(period_ns > APP_PERIOD_MAX )
    {
    	params->task_period_ns = APP_PERIOD_MAX;
    }
    else
    {
    	params->task_period_ns = period_ns;

    }

    params->transfer_time_ns = period_ns / 2;

    if (c_task->type == CYCLIC_CONTROLLER)
        params->task_period_offset_ns = 0;
    else
        params->task_period_offset_ns = period_ns / 2;
}


/*	cyclic_task_start
 *
 *  place of execution: ITCM
 *
 *  description: Check task settings and call tsn_task_start function,
 *               which starts the task timer
 *
 *
 *  params:      c_task - cyclic task handle
 *
 */
int cyclic_task_start(struct cyclic_task *c_task)
{
    unsigned int period_ns = c_task->params.task_period_ns;

    if (!period_ns || ((NSECS_PER_SEC / period_ns) * period_ns != NSECS_PER_SEC)) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("invalid task period(%u ns), needs to be an integer divider of 1 second\n", period_ns);
#endif
        return -1;
    }

    return tsn_task_start(c_task->task);
}


/*	cyclic_task_stop
 *
 *  place of execution: ITCM
 *
 *  description: Calls tsn_stop_task function which stops task timer
 *
 *
 *  params:      c_task - cyclic task handle
 *
 */
void cyclic_task_stop(struct cyclic_task *c_task)
{
    tsn_task_stop(c_task->task);
}


/*	cyclic_task_init
 *
 *  place of execution: flash
 *
 *  description: Prints cyclic task basic information into console
 *  			 Get transmit and receive stream (defined in tsn_task_config.c)
 *  			 Calls tsn_task_register function which creates FreeRTOS task.
 *  			 Initializes rx socket statistics
 *
 *
 *  params:      c_task - cyclic task handle
 *  			 net_rx_func - pointer to callback function called during receive process
 *  			 loop_func - pointer to the cyclic loop function
 *  			 *ctx - pointer to task context
 *
 */
int cyclic_task_init(struct cyclic_task *c_task,
                     void (*net_rx_func)(void *ctx, int msg_id, int src_id, void *buf, int len),
                     void (*loop_func)(void *ctx, int timer_status), void *ctx)
{
    struct tsn_task_params *params = &c_task->params;
    struct tsn_stream *rx_stream, *tx_stream;
    int i;
    int rc;
    char traf_latency[] = "traffic latency";

#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("cyclic task type: %d, id: %u\n\n", c_task->type, c_task->id);
    INF("task params\n");
    INF("task_period_ns        : %u\n", params->task_period_ns);
    INF("task_period_offset_ns : %u\n", params->task_period_offset_ns);
    INF("transfer_time_ns      : %u\n", params->transfer_time_ns);
    INF("sched_traffic_offset  : %u\n", params->sched_traffic_offset);
    INF("use_fp                : %u\n", params->use_fp);
    INF("use_st                : %u\n", params->use_st);
#endif
    tx_stream = tsn_conf_get_stream(c_task->tx_socket.stream_id);
    if (!tx_stream)
        goto err;

    memcpy(&params->tx_params[0].addr, &tx_stream->address,
           sizeof(struct net_address));
    params->num_tx_socket = 1;
    params->tx_params[0].addr.port = 0;

    /* Override default priority if scheduled traffic is disabled (strict priority transmit) */
    if (!params->use_st)
        params->tx_params[0].addr.priority = QOS_NETWORK_CONTROL_PRIORITY;

    for (i = 0; i < c_task->num_peers; i++) {
        rx_stream = tsn_conf_get_stream(c_task->rx_socket[i].stream_id);
        if (!rx_stream)
            goto err;

        memcpy(&params->rx_params[i].addr, &rx_stream->address,
               sizeof(struct net_address));
        params->rx_params[i].addr.port = 0;
        params->num_rx_socket++;
    }

    c_task->net_rx_func = net_rx_func;
    c_task->loop_func = loop_func;
    c_task->ctx = ctx;

    rc = tsn_task_register(&c_task->task, params, c_task->id, main_cyclic, c_task, timer_callback);
    if (rc < 0) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("tsn_task_register rc = %d\n", __func__, rc);
#endif
        goto err;
    }

    for (i = 0; i < c_task->num_peers; i++) {
        c_task->rx_socket[i].net_sock = tsn_net_sock_rx(c_task->task, i);
        c_task->rx_socket[i].stats.traffic_latency_min = 0xffffffff;

        stats_init(&c_task->rx_socket[i].stats.traffic_latency, 31, traf_latency, NULL);
        hist_init(&c_task->rx_socket[i].stats.traffic_latency_hist, 100, 1000);
    }

    c_task->tx_socket.net_sock = tsn_net_sock_tx(c_task->task, 0);

#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("success\n");
#endif

    return 0;

err:
    return -1;
}

/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tsn_task.h"

#include "stats_task.h"
#include "log.h"
#include "types.h"

#include "genavb.h"

#include "genavb/srp.h"
#include "genavb/qos.h"
#include "genavb/ether.h"
#include "freertos/os/timer.h"
#include "freertos/os/qos.h"

static void tsn_net_st_oper_config_print(struct tsn_task *task);

char proc_time[] = "processing time";
char total_time[] = "total time";
char sched_err[] = "sched err";



/*	tsn_task_stats_init
 *
 *  place of execution: flash
 *
 *  description: Initialize tsn task statistics
 *  				- scheduled error
 *  				- processing time
 *  				- total time
 *
 *  			 Initialize statistics history
 *  			 	- scheduled error
 *  				- processing time
 *  				- total time
 *
 *  params:      task - tsn_task context
 *
 */
void tsn_task_stats_init(struct tsn_task *task)
{


    stats_init(&task->stats.sched_err, 31, sched_err, NULL);
    hist_init(&task->stats.sched_err_hist, 100, 100);

    stats_init(&task->stats.proc_time, 31, proc_time, NULL);
    hist_init(&task->stats.proc_time_hist, 100, 1000);

    stats_init(&task->stats.total_time, 31, total_time, NULL);
    hist_init(&task->stats.total_time_hist, 100, 1000);

    task->stats.sched_err_max = 0;
}


/*	tsn_task_stats_start
 *
 *  place of execution: ITCM
 *
 *  description: This function is called in every cyclic task iteration (every 1ms) before
 *  			 packet receive is started.
 *  			 The function gets current time and computes schedule error. Based on the schedule
 *  			 error computation, function eventually updates error counters and then updates
 *  			 schedule error statistics and history.
 *
 *  params:   task - tsn_task context
 *
 */
void tsn_task_stats_start(struct tsn_task *task)
{
    uint64_t now;
    int32_t sched_err;

    task->stats.sched++;

    genavb_clock_gettime64(task->params->clk_id, &now);

    sched_err = now - task->sched_time;

    if (sched_err > (2 * task->params->task_period_ns))
        task->stats.sched_missed++;

    if (sched_err < 0) {
        task->stats.sched_early++;
        sched_err = -sched_err;
    }

    stats_update(&task->stats.sched_err, sched_err);
    hist_update(&task->stats.sched_err_hist, sched_err);

    if (sched_err > task->stats.sched_err_max)
        task->stats.sched_err_max = sched_err;

    task->sched_now = now;
}


/*	tsn_task_stats_end
 *
 *  place of execution: ITCM
 *
 *  description: This function is called in every cyclic task iteration (every 1ms) after
 *  			 cyclic task main loop is executed.
 *  			 The function gets current time and computes processing time and total time
 *  			 of the cyclic task. Then it updates processing time statistics and history and
 *  			 total time statistics and history.
 *
 *  params:   task - tsn_task context
 *
 */
void tsn_task_stats_end(struct tsn_task *task)
{
    uint64_t now;
    int32_t proc_time;
    int32_t total_time;

    genavb_clock_gettime64(task->params->clk_id, &now);

    proc_time = now - task->sched_now;
    total_time = now - task->sched_time;

    stats_update(&task->stats.proc_time, proc_time);
    hist_update(&task->stats.proc_time_hist, proc_time);

    stats_update(&task->stats.total_time, total_time);
    hist_update(&task->stats.total_time_hist, total_time);

    task->sched_time += task->params->task_period_ns;
}


/*	tsn_task_stats_print
 *
 *  place of execution: ITCM
 *
 *  description: This function computes statistics and prints it into the console.
 *               Call stats print and hist_print to print scheduled error, processing time
 *               and total time statistics and history into console.
 *               Call tsn_net_st_oper_config_print which prints information about scheduled time
 *               configuration
 *
 *  params:   data - data which will be printed
 *
 */
static void tsn_task_stats_print(void *data)
{
    struct tsn_task *task = data;

    stats_compute(&task->stats_snap.sched_err);
    stats_compute(&task->stats_snap.proc_time);
    stats_compute(&task->stats_snap.total_time);

#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("tsn task(%p)\n", task);
    INF("sched           : %u\n", task->stats_snap.sched);
    INF("sched early     : %u\n", task->stats_snap.sched_early);
    INF("sched missed    : %u\n", task->stats_snap.sched_missed);
    INF("sched timeout   : %u\n", task->stats_snap.sched_timeout);
    INF("clock discont   : %u\n", task->stats_snap.clock_discont);

    stats_print(&task->stats_snap.sched_err);
    hist_print(&task->stats_snap.sched_err_hist);

    stats_print(&task->stats_snap.proc_time);
    hist_print(&task->stats_snap.proc_time_hist);

    stats_print(&task->stats_snap.total_time);
    hist_print(&task->stats_snap.total_time_hist);
#endif

    if (task->params->use_st)
        tsn_net_st_oper_config_print(task);

    task->stats_snap.pending = false;
}


/*	tsn_task_stats_dump
 *
 *  place of execution: ITCM
 *
 *  description: This function copies data from stats to stats_snap,
 *               then resets scheduler error, procesing time and total time
 *  			 statistics using stats_reset function.
 *  			 Call tsn_task_stats_print via STATS_Async, which send statistics data
 *  			 into the queue.
 *
 *
 *  params:   task - tsn task context
 *
 */
static void tsn_task_stats_dump(struct tsn_task *task)
{
    if (task->stats_snap.pending)
        return;

    memcpy(&task->stats_snap, &task->stats, sizeof(struct tsn_task_stats));
    stats_reset(&task->stats.sched_err);
    stats_reset(&task->stats.proc_time);
    stats_reset(&task->stats.total_time);
    task->stats_snap.pending = true;

    if (STATS_Async(tsn_task_stats_print, task) != pdTRUE)
        task->stats_snap.pending = false;
}


/*	net_socket_stats_print
 *
 *  place of execution: ITCM
 *
 *  description: Prints net socket related statistics.
 *
 *
 *  params:   data - data which will be printed
 *
 */

static void net_socket_stats_print(void *data)
{
    struct net_socket *sock = data;
#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("net %s socket(%p) %d\n", sock->dir ? "tx" : "rx", sock, sock->id);
    INF("frames     : %u\n", sock->stats_snap.frames);
    INF("err        : %u\n", sock->stats_snap.err);
#endif
    sock->stats_snap.pending = false;
}


/*	net_socket_stats_dump
 *
 *  place of execution: ITCM
 *
 *  description: Copy data from stats to stats_snap.
 *  			 Call net_socket_stats_print via STATS_Async, which send statistics data
 *  			 into the queue.
 *
 *
 *  params:   sock - net socket data
 *
 */

void net_socket_stats_dump(struct net_socket *sock)
{
    if (sock->stats_snap.pending)
        return;

    memcpy(&sock->stats_snap, &sock->stats, sizeof(struct net_socket_stats));
    sock->stats_snap.pending = true;

    if (STATS_Async(net_socket_stats_print, sock) != pdTRUE)
        sock->stats_snap.pending = false;
}


/*	tsn_stats_dump
 *
 *  place of execution: ITCM
 *
 *  description: Call tsn_task_stats_dump and net_socket_stats_dump separately for
 *  			 tx sockets and rx sockets
 *
 *
 *  params:   task - tsn task context
 *
 */
void tsn_stats_dump(struct tsn_task *task)
{
    int i;

    tsn_task_stats_dump(task);

    for (i = 0; i < task->params->num_rx_socket; i++)
        net_socket_stats_dump(&task->sock_rx[i]);

    for (i = 0; i < task->params->num_tx_socket; i++)
        net_socket_stats_dump(&task->sock_tx[i]);
}


/*	tsn_net_receive_sock
 *
 *  place of execution: ITCM
 *
 *  description: Check the genavb_socket_rx length. Return status according to the
 *               received value.
 *
 *  params:   sock - net_socket data
 *
 *  return    status - NET_OK, when len > 0
 *  				 - NET_NO_FRAME or NET_ERR
 */
int tsn_net_receive_sock(struct net_socket *sock)
{
    struct tsn_task *task = container_of(sock, struct tsn_task, sock_rx[sock->id]);
    int len;
    int status;

    len = genavb_socket_rx(sock->genavb_rx, sock->buf, task->params->rx_buf_size, &sock->ts);
    if (len > 0) {
        status = NET_OK;
        sock->len = len;
        sock->stats.frames++;
    } else if (len == -GENAVB_ERR_SOCKET_AGAIN) {
        status = NET_NO_FRAME;
    } else {
        status = NET_ERR;
        sock->stats.err++;
    }

    return status;
}


/*	tsn_net_transmit_sock
 *
 *  place of execution: ITCM
 *
 *  description: Transmit genavb socket. Return NET_OK, when socket is
 *               transmitted correctly
 *
 *  params:   sock - net_socket data to transmit
 *
 *  return:   status - status of the genavb_socket_tx
 */
int tsn_net_transmit_sock(struct net_socket *sock)
{
    int rc;
    int status;

    rc = genavb_socket_tx(sock->genavb_tx, sock->buf, sock->len);
    if (rc == GENAVB_SUCCESS) {
        status = NET_OK;
        sock->stats.frames++;
    } else {
        status = NET_ERR;
        sock->stats.err++;
    }

    return status;
}


/*	frame_tx_time_ns
 *
 *  place of execution: ITCM
 *
 *  description: Returns the complete transmit time including MAC framing and physical
 *               layer overhead (802.3).
 *
 *  params:   frame_size - frame size without any framing
 *            speed_mbps - link speed in Mbps
 *
 *  return transmit time in nanoseconds
 */
static unsigned int frame_tx_time_ns(unsigned int frame_size, int speed_mbps)
{
    unsigned int eth_size;

    if(speed_mbps < 1 || speed_mbps > 1000)
    {
    	speed_mbps = 1000;
    }

    eth_size = sizeof(struct eth_hdr) + frame_size + ETHER_FCS;

    if (eth_size < ETHER_MIN_FRAME_SIZE)
        eth_size = ETHER_MIN_FRAME_SIZE;

    if(eth_size > ETHER_MTU)
    	eth_size = ETHER_MTU;

    eth_size += ETHER_IFG + ETHER_PREAMBLE;

    return (((1000 / speed_mbps) * eth_size * 8) + ST_TX_TIME_MARGIN);
}


/*	tsn_net_st_config_enable
 *
 *  place of execution: ITCM
 *
 *  description: Enables scheduled traffic for TSN task
 *
 *  params:   task - tsn task context
 *
 */
static void tsn_net_st_config_enable(struct tsn_task *task)
{
    struct genavb_st_config config;
    struct genavb_st_gate_control_entry gate_list[ST_LIST_LEN];
    struct net_address *addr = &task->params->tx_params[0].addr;
    unsigned int cycle_time = task->params->task_period_ns;
    uint8_t iso_traffic_prio = addr->priority;
    uint8_t tclass = priority_to_traffic_class_map(CFG_TRAFFIC_CLASS_MAX, CFG_SR_CLASS_MAX)[iso_traffic_prio];
    unsigned int iso_tx_time = frame_tx_time_ns(task->params->tx_buf_size, 1000) * ST_TX_TIME_FACTOR;
    int i;

    gate_list[0].gate_states = 1 << tclass;

    if (task->params->use_fp) {
        gate_list[0].operation = GENAVB_ST_SET_AND_HOLD_MAC;

        /*
         * Keep preemptable queues always open.
         * Match configuration done in tsn_net_fp_config_enable().
         */
        for (i = 0; i < tclass; i++)
            gate_list[0].gate_states |= 1 << i;
    } else {
        gate_list[0].operation = GENAVB_ST_SET_GATE_STATES;
    }

    gate_list[0].time_interval = iso_tx_time;

    if (task->params->use_fp)
        gate_list[1].operation = GENAVB_ST_SET_AND_RELEASE_MAC;
    else
        gate_list[1].operation = GENAVB_ST_SET_GATE_STATES;

    gate_list[1].gate_states = ~(1 << tclass);
    gate_list[1].time_interval = cycle_time - iso_tx_time;

    /* Scheduled traffic will start when (base_time + N * cycle_time) > now */
    config.enable = 1;
    config.base_time = task->params->task_period_offset_ns + task->params->sched_traffic_offset;
    config.cycle_time_p = cycle_time;
    config.cycle_time_q = NSECS_PER_SEC;
    config.cycle_time_ext = 0;
    config.list_length = ST_LIST_LEN;
    config.control_list = gate_list;

    if (genavb_st_set_admin_config(addr->port, task->params->clk_id, &config) < 0)
    {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_st_set_admin_config() error\n");
#endif
    }
    else
    {
#if PRINT_LEVEL == VERBOSE_DEBUG
    	INF("scheduled traffic config enabled\n");
#endif
    }

}


/*	tsn_net_st_config_disable
 *
 *  place of execution: ITCM
 *
 *  description: Disables scheduled trafic for TSN task
 *
 *  params:   task - tsn task context
 *
 */
static void tsn_net_st_config_disable(struct tsn_task *task)
{
    struct genavb_st_config config;
    struct net_address *addr = &task->params->tx_params[0].addr;

    config.enable = 0;

    if (genavb_st_set_admin_config(addr->port, task->params->clk_id, &config) < 0)
    {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_st_set_admin_config() error\n");
#endif
    }
    else
    {
#if PRINT_LEVEL == VERBOSE_DEBUG
        INF("scheduled traffic config disabled\n");
#endif
    }
}


/*	tsn_net_st_oper_config_print
 *
 *  place of execution: ITCM
 *
 *  description: Prints scheduled traffic related data into console
 *
 *  params:   task - tsn task context
 *
 */
static void tsn_net_st_oper_config_print(struct tsn_task *task)
{
#if PRINT_LEVEL == VERBOSE_DEBUG
    int i;
#endif
    struct genavb_st_config config;
    struct genavb_st_gate_control_entry gate_list[ST_LIST_LEN];
    struct net_address *addr = &task->params->tx_params[0].addr;

    config.control_list = gate_list;

    if (genavb_st_get_config(addr->port, GENAVB_ST_OPER, &config, ST_LIST_LEN) < 0) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_st_get_config() error\n");
#endif
        return;
    }
#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("base time   : %llu\n", config.base_time);
    INF("cycle time  : %u / %u\n", config.cycle_time_p, config.cycle_time_q);
    INF("ext time    : %u\n", config.cycle_time_ext);

    for (i = 0; i < config.list_length; i++)
    {
        INF("%u op: %u, interval: %u, gates: %b\n",
            i, gate_list[i].operation, gate_list[i].time_interval, gate_list[i].gate_states);
    }
#endif
}

/*	tsn_task_start
 *
 *  place of execution: ITCM
 *
 *  description: Gets current time, compute start time and align on cycle time and add offset,
 *               then starts genavb timer, which periodically wakes up the task.
 *
 *  params:   task - tsn task context
 *
 */
int tsn_task_start(struct tsn_task *task)
{
    uint64_t now, start_time;

    if (!task->timer)
        goto err;

    if (genavb_clock_gettime64(task->params->clk_id, &now) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_clock_gettime64() error\n");
#endif
        goto err;
    }

    /* Start time = rounded up second + 1 second */
    start_time = ((now + NSECS_PER_SEC / 2) / NSECS_PER_SEC + 1) * NSECS_PER_SEC;

    /* Align on cycle time and add offset */
    start_time = (start_time / task->params->task_period_ns) * task->params->task_period_ns + task->params->task_period_offset_ns;

    if (genavb_timer_start(task->timer, start_time,
                          (uint64_t)task->params->task_period_ns, GENAVB_TIMERF_ABS) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_timer_start() error\n");
#endif
        goto err;
    }

    task->sched_time = start_time + task->params->task_period_ns;

    if (task->params->use_st)
        tsn_net_st_config_enable(task);

    return 0;

err:
    return -1;
}


/*	tsn_task_stop
 *
 *  place of execution: ITCM
 *
 *  description: Stops genavb timer, which periodically wakes up the task.
 *
 *  params:   task - tsn task context
 *
 */
void tsn_task_stop(struct tsn_task *task)
{
    if (task->timer) {
        genavb_timer_stop(task->timer);

        if (task->params->use_st)
            tsn_net_st_config_disable(task);
    }
}


/*	tsn_task_net_init
 *
 *  place of execution: flash
 *
 *  description: Opens RX and TX genavb sockets
 *
 *  params:   task - tsn task context
 *
 */
static int tsn_task_net_init(struct tsn_task *task)
{
    int i, j, k;
    struct net_socket *sock;



    for (i = 0; i < task->params->num_rx_socket; i++) {
        sock = &task->sock_rx[i];
        sock->id = i;
        sock->dir = RX;

        if (genavb_socket_rx_open(&sock->genavb_rx, GENAVB_SOCKF_NONBLOCK,
                                  &task->params->rx_params[i]) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("genavb_socket_rx_open error\n");
#endif
            goto close_sock_rx;
        }

        sock->buf = pvPortMalloc(task->params->rx_buf_size);
        if (!sock->buf) {
            genavb_socket_rx_close(sock->genavb_rx);
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("error allocating rx_buff\n");
#endif
            goto close_sock_rx;
        }


    }

    for (j = 0; j < task->params->num_tx_socket; j++) {
        sock = &task->sock_tx[j];
        sock->id = j;
        sock->dir = TX;

        if (genavb_socket_tx_open(&sock->genavb_tx, 0, &task->params->tx_params[j]) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("genavb_socket_tx_open error\n");
#endif
            goto close_sock_tx;
        }

        sock->buf = pvPortMalloc(task->params->tx_buf_size);
        if (!sock->buf) {
            genavb_socket_tx_close(sock->genavb_tx);
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("error allocating tx_buff\n");
#endif
            goto close_sock_tx;
        }

    }

    return 0;

close_sock_tx:
    for (k = 0; k < j; k++) {
        sock = &task->sock_tx[k];

        vPortFree(sock->buf);
        genavb_socket_tx_close(sock->genavb_tx);
    }

close_sock_rx:
    for (k = 0; k < i; k++) {
        sock = &task->sock_rx[k];

        vPortFree(sock->buf);
        genavb_socket_rx_close(sock->genavb_rx);
    }

    return -1;
}


/*	tsn_task_net_exit
 *
 *  place of execution: flash
 *
 *  description: Closes RX and TX genavb sockets
 *
 *  params:   task - tsn task context
 *
 */
static void tsn_task_net_exit(struct tsn_task *task)
{
    int i;
    struct net_socket *sock;

    for (i = 0; i < task->params->num_rx_socket; i++) {
        sock = &task->sock_rx[i];

        vPortFree(sock->buf);
        genavb_socket_rx_close(sock->genavb_rx);
    }

    for (i = 0; i < task->params->num_tx_socket; i++) {
        sock = &task->sock_tx[i];

        vPortFree(sock->buf);
        genavb_socket_tx_close(sock->genavb_tx);
    }
}


/*	tsn_task_register
 *
 *  place of execution: flash
 *
 *  description: Call tsn_task_net_init to open sockets
 *               Call tsn_task_stats_init to initialize task statistics
 *               Creates FreeRTOS which is the main TSN communication task
 *               called every 1ms in our case.
 *               Creates genavb timer, which is used to wake-up the task every 1ms
 *
 *
 *  params:   task - tsn task handle
 *  		  params - tsn task parameters
 *  		  id - task id
 *  		  *main_loop - pointer to main loop function which is called in every task iteration
 *  		  *ctx - pointer to task context
 *  		  *timer_callback - pointer to timer callback function
 *
 */
int tsn_task_register(struct tsn_task **task, struct tsn_task_params *params,
                      int id, void (*main_loop)(void *), void *ctx,
                      void (*timer_callback)(void *, int))
{
    char task_name[20];
    char task_name_init[20] = {"tsn_task1"};

    char timer_name[16];
    char timer_name_init[16] = {"task0 timer"};
    int checkResult = 0;

    *task = pvPortMalloc(sizeof(struct tsn_task));
    if (!(*task))
        goto err;

    memset(*task, 0, sizeof(struct tsn_task));

    (*task)->id = id;
    (*task)->params = params;
    (*task)->ctx = ctx;

    checkResult = sprintf(task_name, "tsn task%1d", (*task)->id);

    if(checkResult < 0 || checkResult >= sizeof(task_name))
    {
    	memcpy(task_name, task_name_init, sizeof(task_name_init));
    }

    checkResult = sprintf(timer_name, "task%1d timer", (*task)->id);

    if(checkResult < 0 || checkResult >= sizeof(task_name))
    {
    	memcpy(timer_name, timer_name_init, sizeof(timer_name_init));
    }

    if (tsn_task_net_init(*task) < 0) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("tsn_task_net_init error\n");
#endif
        goto err_free;
    }

    tsn_task_stats_init(*task);

    if (main_loop) {
        if (xTaskCreate(main_loop, task_name, params->stack_depth,
                        ctx, params->priority, &(*task)->handle) != pdPASS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("xTaskCreate failed\n\r");
#endif
            goto net_exit;
        }
    }

    if (timer_callback) {
        if (genavb_timer_create(&(*task)->timer, params->clk_id, 0) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("genavb_timer_create() error\n");
#endif
            goto task_delete;
        }

        if (genavb_timer_set_callback((*task)->timer, timer_callback, *task) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
            ERR("genavb_timer_create() error\n");
#endif
            goto timer_destroy;
        }
    } else {
        (*task)->timer = NULL;
    }

    return 0;

timer_destroy:
    if ((*task)->timer)
        genavb_timer_destroy((*task)->timer);

task_delete:
    if (main_loop)
        vTaskDelete((*task)->handle);

net_exit:
    tsn_task_net_exit(*task);

err_free:
    vPortFree(*task);

err:
    return -1;
}

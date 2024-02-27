/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TSN_TASKS_CONFIG_H_
#define _TSN_TASKS_CONFIG_H_

#include "FreeRTOS.h"
#include "task.h"

#include "genavb/tsn.h"
#include "genavb/qos.h"
#include "api_qmc_common.h"

#define MAX_PEERS          2
#define MAX_TASK_CONFIGS   1
#define MAX_TSN_STREAMS    2
#define ETHERTYPE_MOTOROLA 0x818D
#define PACKET_SIZE        300

#define SCHED_TRAFFIC_OFFSET 250000

#define TASK_DEFAULT_STACK_SIZE   (configMINIMAL_STACK_SIZE + 256)
#define TASK_DEFAULT_PRIORITY     (configMAX_PRIORITIES - 1)
#define TASK_DEFAULT_QUEUE_LENGTH (8)

#define APP_PERIOD_DEFAULT              1000000
#define APP_PERIOD_MAX                  1000000000
#define APP_PERIOD_MIN                  100000
#define NET_DELAY_OFFSET_DEFAULT        (APP_PERIOD_DEFAULT / 2)


enum task_id {
    IO_DEVICE_0,
    IO_DEVICE_1,
	CONTROLLER_0,
    MAX_TASKS_ID
};

enum task_type {
    CYCLIC_CONTROLLER,
    CYCLIC_IO_DEVICE,
};

enum gm_capable {
	GM_CAPABILITY_DISABLED,
	GM_CAPABILITY_ENABLED,
};


struct tsn_stream {
    struct net_address address;
};

struct tsn_stream *tsn_conf_get_stream(int index);
struct cyclic_task *tsn_conf_get_cyclic_task(unsigned int index);

qmc_status_t Init_network_addresses();

#endif /* _TSN_TASKS_CONFIG_H_ */

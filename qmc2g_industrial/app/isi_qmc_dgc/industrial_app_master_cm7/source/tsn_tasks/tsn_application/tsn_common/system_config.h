/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * System configuration
 */
#ifndef _SYSTEM_CONFIG_H_
#define _SYSTEM_CONFIG_H_

#include "board.h"
#include "tsn_board.h"
#include "fsl_gpt.h"

struct board_config {
    unsigned int board_gpt_1_input_freq;
    gpt_clock_source_t board_gpt_1_clock_source;
};

struct net_config {
    uint8_t hw_addr[6];
    uint8_t ip_addr[4];
    uint8_t net_mask[4];
    uint8_t gw_addr[4];
    uint8_t dns_addr[4];
};

struct tsn_app_config {
    unsigned int role;
    unsigned int use_st;
    unsigned int use_fp;
    unsigned int period_ns;
    uint8_t gmCapable;
    uint8_t priority1;
};


struct system_config {
    const struct board_config board;
    struct tsn_app_config app;
    struct net_config net[BOARD_NUM_PORTS];
};

struct board_config *system_config_get_board(void);
struct net_config *system_config_get_net(int port_id);
struct tsn_app_config *system_config_get_tsn_app(void);


#endif /* _SYSTEM_CONFIG_H_ */

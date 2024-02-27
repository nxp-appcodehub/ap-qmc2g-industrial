/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "system_config.h"

#include <stdio.h>

extern struct system_config system_cfg;



/*	system_config_get_board
 *
 *  place of execution: flash
 *
 *  description: returns board configuration (board_gpt_1_input_freq and board_gpt_1_clock_source)
 *               stored in system_cfg structure
 *
 *  params:
 *
 *  return: content of the system_cfg.board
 *
 */
struct board_config *system_config_get_board(void)
{
    return (struct board_config *)&system_cfg.board;
}


/*	system_config_get_net
 *
 *  place of execution: flash
 *
 *  description: returns net configuration (HW addr, IP addr net mask and GW addr)
 *               stored in system_cfg structure
 *
 *  params: int port_id
 *
 *  return: content of the system_cfg.net
 *
 */
struct net_config *system_config_get_net(int port_id)
{
    if (port_id >= BOARD_NUM_PORTS)
        return NULL;

    return &system_cfg.net[port_id];
}


/*	system_config_get_tsn_app
 *
 *  place of execution: flash
 *
 *  description: returns tsn_app_config structure with basic application configuration
 *  			 this configuration is stored in system_cfg structure in  tsn_app/configs.c file
 *
 *  params:
 *
 */
struct tsn_app_config *system_config_get_tsn_app(void)
{
    struct tsn_app_config *config = &system_cfg.app;

    return config;
}


/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "log.h"
#include "genavb.h"

#include "FreeRTOS.h"

#include "genavb/config.h"
#include "genavb/genavb.h"
#include "genavb/log.h"
#include "genavb/stats.h"
#include "genavb/timer.h"

#include "system_config.h"

static struct genavb_handle *s_genavb_handle = NULL;
static struct port_stats port_stats[CFG_EP_DEFAULT_NUM_PORTS];

extern struct system_config system_cfg;


/*	gavb_stack_init
 *
 *  place of execution: flash
 *
 *  description: TSN stack initialization
 *
 *  params:
 *
 */
int gavb_stack_init(void)
{
    struct genavb_config *genavb_config;
    int rc = 0;

    /*Allocate memory for genavb_config structure*/
    genavb_config = pvPortMalloc(sizeof(struct genavb_config));

    /*Check, if memory has been allocated, otherwise exit*/
    if (!genavb_config) {
        rc = -1;
        goto exit;
    }

    /*Check, if s_genavb_handle does not exist, if already exists, function does not continue with initialization*/
    if (s_genavb_handle) {
        rc = 0;
        goto exit;
    }

    /*Gets a copy of the GenAVB default configuration. */
    genavb_get_default_config(genavb_config);

    /*Set log level for single parts of the stack*/
    genavb_config->management_config.log_level = GENAVB_LOG_LEVEL_ERR;
    genavb_config->srp_config.log_level = GENAVB_LOG_LEVEL_ERR;
    genavb_config->fgptp_config.log_level = GENAVB_LOG_LEVEL_ERR;


    /*Sets the GenAVB configuration (used at initialization).*/
    genavb_set_config(genavb_config);


    /*Adjust the fgptp configuration according to the application requirements*/
    /*All configuration must be done here manually. FS has been removed*/

     genavb_config->fgptp_config.domain_cfg[0].gmCapable = system_cfg.app.gmCapable;
     genavb_config->fgptp_config.domain_cfg[0].priority1 = system_cfg.app.priority1;


    /*Sets the GenAVB configuration (used at initialization).*/
    genavb_set_config(genavb_config);

    /*Call ganavb_init API function. This is the main API call, which initializes the tsn stack itself*/
    if ((rc = genavb_init(&s_genavb_handle, 0)) != GENAVB_SUCCESS) {
        s_genavb_handle = NULL;



#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_init() failed: %s\n", genavb_strerror(rc));
#endif
        rc = -1;
        goto exit;
    }

exit:
    if (genavb_config)
        vPortFree(genavb_config);

    return rc;
}


/*	gavb_stack_exit
 *
 *  place of execution: flash
 *
 *  description: TSN stack de-initialization
 *
 *  params:
 *
 */
int gavb_stack_exit(void)
{
    genavb_exit(s_genavb_handle);

    s_genavb_handle = NULL;

    return 0;
}


/*	gavb_port_stats_init
 *
 *  place of execution: flash
 *
 *  description: Initialize port statistics
 *
 *  params:
 *
 */
int gavb_port_stats_init(unsigned int port_id)
{
    int num;
    struct port_stats *stats = &port_stats[port_id];

    /*Check port number based on port_id*/
    num = genavb_port_stats_get_number(port_id);
    if (num < 0) {

#if PRINT_LEVEL == VERBOSE_DEBUG
    		ERR("genavb_port_stats_get_number() error %d\n", num);
#endif
        goto err;
    }

    /*Prepare stats configuration*/
    stats->num = num;
    stats->str_len = GENAVB_PORT_STATS_STR_LEN;
    stats->names = pvPortMalloc(num * GENAVB_PORT_STATS_STR_LEN);
    if (!stats->names) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("pvPortMalloc() failed\n");
#endif
        goto err;
    }

    if (genavb_port_stats_get_strings(port_id, stats->names, num * GENAVB_PORT_STATS_STR_LEN) != GENAVB_SUCCESS) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("genavb_port_stats_get_strings() failed\n");
#endif
        goto err;
    }

    stats->values = pvPortMalloc(num * sizeof(uint64_t));
    if (!stats->values) {
#if PRINT_LEVEL == VERBOSE_DEBUG
        ERR("pvPortMalloc() failed\n");
#endif
        goto err;
    }

    return 0;

err:
    if (stats->names)
        vPortFree(stats->names);

    return -1;
}


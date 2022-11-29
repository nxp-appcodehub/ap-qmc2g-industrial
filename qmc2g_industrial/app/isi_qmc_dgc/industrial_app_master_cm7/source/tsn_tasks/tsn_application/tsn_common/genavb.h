/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GENAVB_H_
#define _GENAVB_H_

#include "genavb/init.h"
#include "genavb/clock.h"

struct port_stats {
    char *names;
    uint64_t *values;
    int num;
    int str_len;
};


int gavb_port_stats_init(unsigned int port_id);
int gavb_stack_init(void);
int gavb_stack_exit(void);

#endif /* _GENAVB_H_ */

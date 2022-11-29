/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"

/*	sdk_printf
 *
 *  place of execution: ITCM
 *
 *  description: called in os_log function, defined in tsn_stack/freertos/log.c file
 *
 *  params: *fmt_s
 *
 */
int sdk_printf(const char *fmt_s, ...)
{
    va_list ap;
    int rc;

    va_start(ap, fmt_s);
    rc = VPRINTF(fmt_s, ap);
    va_end(ap);

    return rc;
}


/*	sdk_printf
 *
 *  place of execution: ITCM
 *
 *  description: called in os_log function, defined in tsn_stack/freertos/log.c file
 *
 *  params: const char *fmt_s
 *  		va_list ap
 *
 */
int sdk_vprintf(const char *fmt_s, va_list ap)
{
    return VPRINTF(fmt_s, ap);
}

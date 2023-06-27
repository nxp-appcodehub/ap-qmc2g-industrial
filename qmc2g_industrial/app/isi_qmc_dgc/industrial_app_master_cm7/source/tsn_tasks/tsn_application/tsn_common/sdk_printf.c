/*
 * Copyright 2021, 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "log.h"

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
#if PRINT_LEVEL == VERBOSE_DEBUG
    va_list ap;
    int rc;

    va_start(ap, fmt_s);
    rc = VPRINTF(fmt_s, ap);
    va_end(ap);

    return rc;
#else

    return 1;

#endif
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
#if PRINT_LEVEL == VERBOSE_DEBUG
    return VPRINTF(fmt_s, ap);
#else
    return 1;
#endif
}

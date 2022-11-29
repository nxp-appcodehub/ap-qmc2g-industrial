/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SDK_PRINTF_H_
#define _SDK_PRINTF_H_

#include <stdarg.h>

int sdk_printf(const char *fmt_s, ...);
int sdk_vprintf(const char *fmt_s, va_list ap);

#endif /* _SDK_PRINTF_H_ */

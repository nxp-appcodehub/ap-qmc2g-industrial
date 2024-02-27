/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
 
#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include "api_qmc_common.h"

qmc_status_t dispatcher_init();
qmc_status_t dispatcher_get_flash_lock( TickType_t ticks);
qmc_status_t dispatcher_release_flash_lock();
qmc_status_t dispatcher_read_memory( void *pdst, void *psrc, size_t size, TickType_t ticks);
qmc_status_t dispatcher_write_memory( void *pdst, void *psrc, size_t size, TickType_t ticks);
qmc_status_t dispatcher_erase_sectors( void *pdst, uint16_t sec_cn, TickType_t ticks);

#endif /* _DISPATCHER_H_ */

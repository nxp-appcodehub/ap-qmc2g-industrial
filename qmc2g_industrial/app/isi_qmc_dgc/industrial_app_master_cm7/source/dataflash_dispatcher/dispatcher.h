/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
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

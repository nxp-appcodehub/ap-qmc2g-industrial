/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*
 * A Considerable IoT HTTPd
 *
 * embedded http server
 * */

#ifndef CIT_HTTPD_H
#define CIT_HTTPD_H

#include <lwip/opt.h>
#include <lwip/ip_addr.h>
#include <lwip/altcp.h>
#include <lwip/tcpip.h>

#include "plug.h"

/**
 * @struct httpd configuration
 */
struct cit_httpd_config
{
    struct altcp_tls_config *tls;
    u16_t rule_cnt;              /*!< count of routes */
    const plug_rule_t *rule_vec; /*!< pointer to rule vector */
    const ip_addr_t *ip_addr;
    u16_t port; /*!< port */
    u8_t ip_type;
    void *arg;
};

/**
 * @brief start the server
 *
 * @param conf server configuration 
 */
void cit_httpd_init(struct cit_httpd_config *conf);

#ifndef _DEFAULT_SOURCE
#warning _DEFAULT_SOURCE not defined
#warning ensure strsep() is available
#endif

/**
 * Register default Extensions.
 * the http server uses plug_status internally
 * */
PLUG_EXTENSION_PROTOTYPE(plug_status);
PLUG_EXTENSION_PROTOTYPE(plug_rewrite_path);
PLUG_EXTENSION_PROTOTYPE(plug_redirect);

#endif /* CITSRV_H */

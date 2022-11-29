/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "lwip/dns.h"

#include "log.h"
#include "lwip_ethernetif.h"

#include "system_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/


/*	TsnInitTask
 *
 *  place of execution: flash
 *
 *  description: LWIP initialization. Part of the initialization is also LWIP TSN related features initialization.
 *               Main part of TSN initialization is called inside ethernetif_init which is parameter of the
 *               netif_add. This function creates LWIP net interface.
 *
 *  params:
 *
 */
void lwip_stack_init(void)
{
    static struct netif fsl_netif0;
    const struct net_config *net_cfg;
    ip4_addr_t fsl_netif0_ipaddr, fsl_netif0_netmask, fsl_netif0_gw, fsl_netif0_dns;
    int port_id = 0;

    /*Get configuration from system_cfg placed in configs.c*/
    net_cfg = system_config_get_net(port_id);

    IP4_ADDR(&fsl_netif0_ipaddr, net_cfg->ip_addr[0], net_cfg->ip_addr[1], net_cfg->ip_addr[2], net_cfg->ip_addr[3]);
    IP4_ADDR(&fsl_netif0_netmask, net_cfg->net_mask[0], net_cfg->net_mask[1], net_cfg->net_mask[2], net_cfg->net_mask[3]);
    IP4_ADDR(&fsl_netif0_gw, net_cfg->gw_addr[0], net_cfg->gw_addr[1], net_cfg->gw_addr[2], net_cfg->gw_addr[3]);
    IP4_ADDR(&fsl_netif0_dns, net_cfg->dns_addr[0], net_cfg->dns_addr[1], net_cfg->dns_addr[2], net_cfg->dns_addr[3]);

    tcpip_init(NULL, NULL);

    netif_add(&fsl_netif0, &fsl_netif0_ipaddr, &fsl_netif0_netmask, &fsl_netif0_gw,
              NULL, ethernetif_init, tcpip_input);
    netif_set_default(&fsl_netif0);
    netif_set_up(&fsl_netif0);

    dns_setserver(0, &fsl_netif0_dns);

#if PRINT_LEVEL == VERBOSE_DEBUG
    INF("************************************************\r\n");
    INF(" IPv4 Address     : %u.%u.%u.%u\r\n", ((u8_t *)&fsl_netif0_ipaddr)[0],
        ((u8_t *)&fsl_netif0_ipaddr)[1],
        ((u8_t *)&fsl_netif0_ipaddr)[2],
        ((u8_t *)&fsl_netif0_ipaddr)[3]);
    INF(" IPv4 Subnet mask : %u.%u.%u.%u\r\n", ((u8_t *)&fsl_netif0_netmask)[0],
        ((u8_t *)&fsl_netif0_netmask)[1],
        ((u8_t *)&fsl_netif0_netmask)[2],
        ((u8_t *)&fsl_netif0_netmask)[3]);
    INF(" IPv4 Gateway     : %u.%u.%u.%u\r\n", ((u8_t *)&fsl_netif0_gw)[0],
        ((u8_t *)&fsl_netif0_gw)[1],
        ((u8_t *)&fsl_netif0_gw)[2],
        ((u8_t *)&fsl_netif0_gw)[3]);
    INF("************************************************\r\n");
#endif
}

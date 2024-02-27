/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "system_config.h"

#include "genavb/tsn.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

/* Forward declarations. */
static void ethernetif_input(void *arg);

static struct genavb_socket_tx_params tx_params = {
    .addr = {
        .ptype = PTYPE_L2,
        .port = 0,
        .vlan_id = 0xffff,
        .priority = 0, // Best Effort
    },
};

static struct genavb_socket_rx_params rx_params = {
    .addr = {
        .ptype = PTYPE_OTHER,
        .port = 0,
    },
};

struct ethernetif {
    struct genavb_socket_tx *sock_tx;
    struct genavb_socket_rx *sock_rx;
};



/*	low_level_init
 *
 *  place of execution: flash
 *
 *  description: In this function, the hardware should be initialized.
 *               Called from ethernetif_init(). Creates lwip rx task
 *
 *  params: netif the already initialized lwip network interface structure
 *          for this ethernetif
 */
static err_t low_level_init(struct netif *netif)
{
    struct ethernetif *ethernetif = netif->state;
    int rc;
    const struct net_config *net_cfg;
    int port_id = 0;

    /*get port configuration, important for HW_ADDR*/
    net_cfg = system_config_get_net(port_id);

    /*Open TX and RX genavb sockets*/
    rc = genavb_socket_tx_open(&ethernetif->sock_tx, GENAVB_SOCKF_RAW, &tx_params);
    if (rc != GENAVB_SUCCESS)
        goto err;

    rc = genavb_socket_rx_open(&ethernetif->sock_rx, GENAVB_SOCKF_RAW, &rx_params);
    if (rc != GENAVB_SUCCESS)
        goto err_close_tx_socket;


    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    if (netif->hwaddr_len <= NETIF_MAX_HWADDR_LEN)
    {
    	/* set MAC hardware address, read from system_cfg */
    	memcpy(netif->hwaddr, net_cfg->hw_addr, netif->hwaddr_len);
    }
    else
    {
    	goto err;
    }

    /* maximum transfer unit */
    netif->mtu = ETHER_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /*create new task called "lwip rx"*/
    if (sys_thread_new("lwip rx", ethernetif_input, netif,
                       ETHERIF_THREAD_STACKSIZE, ETHERIF_THREAD_PRIO) == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("rx task init failed\n"));
        goto err_close_rx_socket;
    }

    LWIP_DEBUGF(NETIF_DEBUG, ("%s: success\n", __func__));

    return ERR_OK;

err_close_rx_socket:
    genavb_socket_rx_close(ethernetif->sock_rx);

err_close_tx_socket:
    genavb_socket_tx_close(ethernetif->sock_tx);

err:
    return ERR_IF;
}


/*	low_level_output
 *
 *  place of execution: ITCM
 *
 *  description: This function should do the actual transmission of the packet. The packet is
 *               contained in the pbuf that is passed to the function. This pbuf
 *               might be chained.
 *
 *  params: netif the lwip network interface structure for this ethernetif
 *          p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 *
 *  @return ERR_OK if the packet could be sent an err_t value if the packet couldn't be sent
 *
 *  @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *        strange results. You might consider waiting for space in the DMA queue
 *        to become available since the stack doesn't retry to send a packet
 *        dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q = p;
    struct ethernetif *ethernetif;
    err_t rc = ERR_OK;

    ethernetif = netif->state;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    LWIP_DEBUGF(NETIF_DEBUG, ("%s: %p %u\n", __func__, p->next, p->tot_len));

    /* If chained buf, copy fragments into a contiguous buffer */
    if (p->next) {
        q = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
        if (!q) {
            LWIP_DEBUGF(NETIF_DEBUG, ("%s: pbuf_alloc error\n", __func__));
            rc = ERR_MEM;
            goto exit;
        }

        rc = pbuf_copy(q, p);
        if (rc != ERR_OK) {
            LWIP_DEBUGF(NETIF_DEBUG, ("%s: pbuf_copy error\n", __func__));
            goto exit_free;
        }
    }

    /*Send genavb frame, check if correct*/
    rc = genavb_socket_tx(ethernetif->sock_tx, q->payload, q->tot_len);
    if (rc != GENAVB_SUCCESS) {
        LWIP_DEBUGF(NETIF_DEBUG, ("%s: genavb_socket_tx error(%d)\n", __func__, rc));
        rc = ERR_IF;
        goto exit_free;
    }

    LWIP_DEBUGF(NETIF_DEBUG, ("%s: genavb_socket_tx ok\n", __func__));

    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    if (((u8_t *)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
    } else {
        /* unicast packet */
        MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    }
    /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);

exit_free:
    if (p != q)
        pbuf_free(q);

exit:
    return rc;
}


/*	low_level_input
 *
 *  place of execution: ITCM
 *
 *  description: Should allocate a pbuf and transfer the bytes of the incoming
 *               packet from the interface into the pbuf.
 *
 *  params: netif the lwip network interface structure for this ethernetif
 *
 *  @return a pbuf filled with the received packet (including MAC header)
 *          NULL on memory error
 *
 */
static struct pbuf *low_level_input(struct netif *netif)
{
    struct ethernetif *ethernetif = netif->state;
    struct pbuf *p;
    int16_t len;

    len = PBUF_POOL_BUFSIZE;

    LWIP_DEBUGF(NETIF_DEBUG, ("%s:\n", __func__));

    /* We allocate a pbuf of maximum size */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (!p)
        goto err;

    /* No support for chained buffer */
    if (p->next != 0)
        goto err_pbuf_free;

    /* Read frame, blocking mode */
    len = genavb_socket_rx(ethernetif->sock_rx, p->payload, len, NULL);
    if (len < 0)
        goto err_pbuf_free;

    LWIP_DEBUGF(NETIF_DEBUG, ("%s: genavb_socket_rx ok, len:%d\n", __func__, len));

    p->tot_len = len;
    p->len = len;


    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    if (((u8_t *)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
        /* unicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }

    LINK_STATS_INC(link.recv);

    return p;

err_pbuf_free:
    pbuf_free(p);

err:
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);

    return NULL;
}


/*	ethernetif_input
 *
 *  place of execution: ITCM
 *
 *  description: This function should be called when a packet is ready to be read
 *               from the interface. It uses the function low_level_input() that
 *               should handle the actual reception of bytes from the network
 *               interface. Then the type of the received packet is determined and
 *               the appropriate input function is called.
 *
 *  params: void *arg
 *
 */
static void ethernetif_input(void *arg)
{
    struct pbuf *p;
    struct netif *netif = arg;

    while (true) {
        /* move received packet into a new pbuf */
        p = low_level_input(netif);

        /* if no packet could be read, silently ignore this */
        if (p != NULL) {
            /* pass all packets to ethernet_input, which decides what packets it supports */
            if (netif->input(p, netif) != ERR_OK) {
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                pbuf_free(p);
                p = NULL;
            }
        }
    }
}


/*	ethernetif_init
 *
 *  place of execution: flash
 *
 *  description: Should be called at the beginning of the program to set up the
 *               network interface. It calls the function low_level_init() to do the
 *               actual setup of the hardware.
 *
 *  params: netif the lwip network interface structure for this ethernetif
 *  @return ERR_OK if the loopif is initialized
 *          ERR_MEM if private data couldn't be allocated
 *          any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
    struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));


    /*Initialize memory for genavb sockets*/
    ethernetif = mem_malloc(sizeof(struct ethernetif));
    if (ethernetif == NULL) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
        return ERR_MEM;
    }

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;

    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    return low_level_init(netif);
}

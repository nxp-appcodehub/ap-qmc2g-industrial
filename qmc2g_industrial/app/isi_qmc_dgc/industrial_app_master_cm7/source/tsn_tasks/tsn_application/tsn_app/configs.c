/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "system_config.h"
#include "tsn_tasks_config.h"
#include "board.h"
#include "tsn_board.h"
#include "api_qmc_common.h"
#include "tsn_tasks_config.h"
#include "api_configuration.h"

static uint8_t mac_address [6];
static uint8_t ip_address [4];
static uint8_t ip_mask [4];
static uint8_t gw_address [4];
static uint8_t dns_address [4];
static uint16_t vlan_id_16;

extern struct tsn_stream  tsn_streams[];

struct system_config system_cfg = {
    .board = {
        .board_gpt_1_input_freq = BOARD_GPT_1_CLK_EXT_FREQ,
        .board_gpt_1_clock_source = BOARD_GPT_1_CLK_SOURCE_TYPE,
    },
    .net = {

        [0] = {
            .hw_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            .ip_addr = {0, 0, 0, 0},
            .net_mask = {0, 0, 0, 0},
            .gw_addr = {0, 0, 0, 0},
			.dns_addr = {0, 0, 0, 0},
        },

    },
    .app = {
            .role = IO_DEVICE_0,
            .use_st = 1,
            .use_fp = 0,
            .period_ns = APP_PERIOD_DEFAULT,
			.gmCapable = GM_CAPABILITY_DISABLED,
			.priority1 = 248,
    },

};

static uint16_t htons_func(uint16_t x)
{
	uint16_t tmp = 0;
	uint16_t tmp1 = 0;

	tmp = ((x & 0xFF00) >> 8);
	tmp1 = ((x & 0xFF) << 8);

	return (tmp | tmp1);
}


qmc_status_t Init_network_addresses()
{
	/*Set IP address*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_Ip, ip_address, sizeof(ip_address)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(system_cfg.net[0].ip_addr, ip_address, sizeof(system_cfg.net[0].ip_addr));

	/*Set IP address mask*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_Ip_mask, ip_mask, sizeof(ip_mask)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(system_cfg.net[0].net_mask, ip_mask, sizeof(system_cfg.net[0].net_mask));

	/*Set IP GW*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_Ip_GW, gw_address, sizeof(gw_address)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(system_cfg.net[0].gw_addr, gw_address, sizeof(system_cfg.net[0].gw_addr));

	/*Set IP DNS*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_Ip_DNS, dns_address, sizeof(dns_address)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(system_cfg.net[0].dns_addr, dns_address, sizeof(system_cfg.net[0].dns_addr));

	/*Set MAC address*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_MAC_address, mac_address, sizeof(mac_address)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(system_cfg.net[0].hw_addr, mac_address, sizeof(system_cfg.net[0].hw_addr));

	/*Set VLAN_ID*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_VLAN_ID, (unsigned char*)&vlan_id_16, sizeof(vlan_id_16)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}


	vlan_id_16 = htons_func(vlan_id_16);

	tsn_streams[0].address.vlan_id = vlan_id_16;
	tsn_streams[1].address.vlan_id = vlan_id_16;


	/*Set RX stream MAC address*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_TSN_RX_STREAM_MAC_ADDR, mac_address, sizeof(mac_address)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(tsn_streams[0].address.u.l2.dst_mac, mac_address, sizeof(tsn_streams[0].address.u.l2.dst_mac));

	/*Set TX stream MAC address*/
	if(CONFIG_GetBinValueById(kCONFIG_Key_TSN_TX_STREAM_MAC_ADDR, mac_address, sizeof(mac_address)) != kStatus_QMC_Ok)
	{
		return kStatus_QMC_Err;
	}

	memcpy(tsn_streams[1].address.u.l2.dst_mac, mac_address, sizeof(tsn_streams[1].address.u.l2.dst_mac));

	return kStatus_QMC_Ok;

}



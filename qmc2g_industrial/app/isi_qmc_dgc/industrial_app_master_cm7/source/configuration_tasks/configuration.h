/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include "api_qmc_common.h"
#include "stdbool.h"
#include "api_configuration.h"
//#include "lcrypto.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define CONFIGURATION_HASH_SIZE                 (32U)

typedef struct {
	config_id_t		id;
	uint8_t		 	key[CONFIG_MAX_KEY_LEN];
	uint8_t		    value[CONFIG_MAX_VALUE_LEN];
} cnf_record_t;

typedef struct {
	cnf_record_t   cnf_data[CONFIG_MAX_RECORDS] __ALIGNED(32);
} cnf_data_t;

typedef struct {
	union {
		uint8_t     hash[ CONFIGURATION_HASH_SIZE];
		uint32_t    chksum;
	};
	cnf_record_t   cnf_data[CONFIG_MAX_RECORDS] __ALIGNED(32);
} cnf_struct_t;



#endif /* _CONFIGURATION_H_ */

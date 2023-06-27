/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include "api_qmc_common.h"
#include "stdbool.h"
#include "api_configuration.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define CONFIGURATION_HASH_SIZE                 LCRYPTO_HASH_SIZE
#define CONFIGURATION_AES_KEY_SIZE              LCRYPTO_AES_KEY_SIZE
#define CONFIGURATION_AES_IV_SIZE               LCRYPTO_AES_IV_SIZE

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

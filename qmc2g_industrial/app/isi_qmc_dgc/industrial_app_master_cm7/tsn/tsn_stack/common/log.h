/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2020 NXP
* 
* NXP Confidential. This software is owned or controlled by NXP and may only 
* be used strictly in accordance with the applicable license terms.  By expressly 
* accepting such terms or by downloading, installing, activating and/or otherwise 
* using the software, you are agreeing that you have read, and that you agree to 
* comply with and are bound by, such license terms.  If you do not agree to be 
* bound by the applicable license terms, then you may not retain, install, activate 
* or otherwise use the software.
*/

/**
 @file
 @brief logging services
 @details print logging messages to the standard output
*/

#ifndef _COMMON_LOG_H_
#define _COMMON_LOG_H_

#include "genavb/log.h"

#include "common/config.h"

#include "os/clock.h"

/** Logging levels definition
 *
 */
typedef enum {
	LOG_CRIT = GENAVB_LOG_LEVEL_CRIT,
	LOG_ERR = GENAVB_LOG_LEVEL_ERR,
	LOG_INIT = GENAVB_LOG_LEVEL_INIT,
	LOG_INFO = GENAVB_LOG_LEVEL_INFO,
	LOG_DEBUG = GENAVB_LOG_LEVEL_DEBUG
} log_level_t;

/** Logging AVB component IDs
 *
 */
typedef enum {
	avtp_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_AVTP,
	avdecc_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_AVDECC,
	srp_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_SRP,
	maap_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_MAAP,
	common_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_COMMON,
	os_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_OS,
	gptp_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_GPTP,
	api_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_API,
	management_COMPONENT_ID = GENAVB_LOG_COMPONENT_ID_MGMT,
	max_COMPONENT_ID
} log_component_id_t;

/** Set the logging level for a component
 *
 * \return negative if error, 0 otherwise
 * \param id	component id
 * \param level	logging level to apply (see log_level_t definition)
 */
int log_level_set(unsigned int id, log_level_t level);

/** Update the time that will be displayed in log messages.
 * May be called at any time to provide the desired log timestamping accuracy.
 * \return none
 */
void log_update_time(os_clock_id_t clk_id);

extern log_level_t log_component_lvl[];
extern const char *log_lvl_string[];
extern u64 log_time_s;
extern u64 log_time_ns;

#include "os/log.h"


#define _COMPONENT_ID_ 3
#define _COMPONENT_STR_ ""

#define os_log(level, ...) do {	\
	if ((level) <= log_component_lvl[_COMPONENT_ID_])	\
		_os_log(log_lvl_string[level], __func__, _COMPONENT_STR_, __VA_ARGS__);	\
} while(0)

#endif /* _COMMON_LOG_H_ */

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef WEBSERVICE_JSON_API_H_
#define WEBSERVICE_JSON_API_H_
#include "plug_json.h"

PLUG_JSON_CB_PROTOTYPE(json_motor_api);
PLUG_JSON_CB_PROTOTYPE(json_log_api);
PLUG_JSON_CB_PROTOTYPE(json_session_api);
PLUG_JSON_CB_PROTOTYPE(json_settings_api);
PLUG_JSON_CB_PROTOTYPE(json_time_api);
PLUG_JSON_CB_PROTOTYPE(json_system_api);
PLUG_JSON_CB_PROTOTYPE(json_user_list_api);
PLUG_JSON_CB_PROTOTYPE(json_user_api);
PLUG_JSON_CB_PROTOTYPE(json_motd_api);
PLUG_JSON_CB_PROTOTYPE(json_reset_api);

PLUG_EXTENSION_PROTOTYPE(qmc_logging);
PLUG_EXTENSION_PROTOTYPE(qmc_fw_upload);
PLUG_EXTENSION_PROTOTYPE(qmc_authorize);
PLUG_EXTENSION_PROTOTYPE(qmc_check_session);

#endif

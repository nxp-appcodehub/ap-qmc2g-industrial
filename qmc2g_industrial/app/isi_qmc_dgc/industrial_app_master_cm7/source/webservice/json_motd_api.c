/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <ctype.h>

#include "api_configuration.h"
#include "api_qmc_common.h"
#include "plug_json.h"
#include "json_string.h"

#include "qmc_features_config.h"

PLUG_JSON_CB(json_motd_api);

/**
 * @brief send the MOTD config value as a json string
 */
http_status_t json_motd_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    qmc_status_t status;
    char value[CONFIG_MAX_VALUE_LEN + 1] = {0};

    switch (request->method)
    {
        case HTTP_GET:
            status = CONFIG_GetStrValue((const unsigned char*)"MOTD", (unsigned char*)value);
            if (status == kStatus_QMC_Ok)
            {
                fputs_json_string(value, response);
            }
            else {
                fputs_json_string("QMC2G - MOTD not set", response);
            }
            break;
        default:
            break;
    }
    return HTTP_REPLY_OK;
}

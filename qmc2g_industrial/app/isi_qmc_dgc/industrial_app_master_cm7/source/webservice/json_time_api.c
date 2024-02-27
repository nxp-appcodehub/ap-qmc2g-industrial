/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "api_qmc_common.h"
#include "api_usermanagement.h"
#include "plug_json.h"
#include "core_json.h"
#include "json_api_common.h"

#include "api_rpc.h"
#include "webservice/json_api_common.h"

PLUG_JSON_CB(json_time_api);

/**
 * @brief time api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */

http_status_t json_time_api(plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    http_status_t code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
    qmc_status_t status;
    qmc_timestamp_t timestamp = {0};

    char *time_sec;
    char *time_msec;
    size_t time_len;
    JSONTypes_t type;

    unsigned long long int number;

    JSONStatus_t jstatus;
    usrmgmt_session_t *user_session = request->session;

    switch (request->method)
    {
        case HTTP_GET:
            status = RPC_GetTimeFromRTC(&timestamp);
            if (webservice_check_error(status, response))
            {
                fputs("{\"time\":", response);
                print_json_timestamp(&timestamp, response);
                fputs("}\n", response);
            }

            code = HTTP_REPLY_OK;
            break;
        case HTTP_PUT:
        case HTTP_POST:
            if (!user_session)
            {
                code = HTTP_NOREPLY_UNAUTHORIZED;
                break;
            }
            if (!user_session || user_session->role != kUSRMGMT_RoleMaintenance)
            {
                code = HTTP_NOREPLY_FORBIDDEN;
                break;
            }
            code    = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("time"), &time_sec, &time_len, &type);
            if (jstatus != JSONSuccess)
            {
                break;
            }
            if (type != JSONString)
            {
                break;
            }
            number            = strntoull(time_sec, time_len, (const char**)&time_msec, 10);

            timestamp.seconds = number / 1000;
            timestamp.milliseconds = number % 1000;
            code = HTTP_REPLY_OK;

            status = RPC_SetTimeToRTC(&timestamp);
            if (webservice_check_error(status, response))
            {
                fputs("{\"time\":", response);
                print_json_timestamp(&timestamp, response);
                fputs("}\n", response);
            }
            break;
        default:
            break;
    }
    return code;
}

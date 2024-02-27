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
#include "tcpip.h"
#include "webservice/json_api_common.h"

PLUG_JSON_CB(json_reset_api);

/**
 * @brief reset callback function
 * called by the tcp stack once the webserver is done
 *
 * @param ctx  qmc_reset_cause_id_t value cast to void*
 *
*/
static void system_reset(void *ctx)
{ qmc_reset_cause_id_t cause= (qmc_reset_cause_id_t)ctx;
  RPC_Reset(cause);
}


/**
 * @brief reset api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */


http_status_t json_reset_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    http_status_t code;
    usrmgmt_session_t *user_session = request->session;

    switch (request->method)
    {
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

            tcpip_callback(system_reset,(void*)(intptr_t)kQMC_ResetRequest);
            code = HTTP_NOREPLY_NO_CONTENT;
            break;
        default:
            code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            break;
    }
    return code;
}

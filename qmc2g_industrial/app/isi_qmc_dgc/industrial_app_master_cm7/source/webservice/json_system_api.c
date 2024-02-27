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

#include "api_motorcontrol.h"
#include "api_qmc_common.h"
#include "api_board.h"
#include "api_usermanagement.h"
#include "plug_json.h"
#include "json_string.h"
#include "core_json.h"
#include "json_api_common.h"

#include "api_board.h"
#include "qmc_features_config.h"
#include "webservice/json_api_common.h"

PLUG_JSON_CB(json_system_api);

#define QMC_LIFECYCLE(_)                         \
    _(kQMC_LcCommissioning, "commissioning")     \
    _(kQMC_LcOperational, "operational")         \
    _(kQMC_LcError, "error")                     \
    _(kQMC_LcMaintenance, "maintenance")         \
    _(kQMC_LcDecommissioning, "decommissioning") \
    //

// gererate qmc_lifecycle_to_string
GENERATE_ENUM_TO_STRING(qmc_lifecycle, qmc_lifecycle_t, QMC_LIFECYCLE, static);

// gererate qmc_lifecycle_from_string
GENERATE_ENUM_FROM_STRING(qmc_lifecycle, qmc_lifecycle_t, QMC_LIFECYCLE, static);

/**
 * @brief system api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */

http_status_t json_system_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    int code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
    qmc_status_t status;

    char *lc_str;
    size_t lc_len;
    JSONTypes_t type;

    JSONStatus_t jstatus;
    qmc_fw_version_t version;
    qmc_lifecycle_t lc              = 0;
    qmc_lifecycle_t prev_lc         = 0;

    switch (request->method)
    {
        case HTTP_PUT:
        case HTTP_POST:
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("lifecycle"), &lc_str, &lc_len, &type);
            if (jstatus != JSONSuccess)
            {
                break;
            }
            if (type != JSONString)
            {
                break;
            }
            qmc_lifecycle_from_string(&lc, lc_str, lc_len);

            prev_lc = BOARD_GetLifecycle();
            status  = BOARD_SetLifecycle(lc);

            if (prev_lc == kQMC_LcError && lc == kQMC_LcMaintenance)
            {
                unsigned int i;
                for (i = kMC_Motor1; i < MC_MAX_MOTORS; i++)
                {
                    (void)MC_UnfreezeMotor(i);
                }
            }

            if (!webservice_check_error(status, response))
            {
                code = HTTP_REPLY_OK;
                break;
            }
            /*fallthrou*/
        case HTTP_GET:
            lc      = BOARD_GetLifecycle();
            version = BOARD_GetFwVersion();
            fputs("{\"deviceId\":", response);
            fputs_json_string(BOARD_GetDeviceIdentifier(), response);
            fputs(", \"lifecycle\":", response);
            fputs_json_string(qmc_lifecycle_to_string(lc), response);
            fprintf(response, ", \"fwVersion\":\"%u.%u.%u\"}\n", version.major, version.minor, version.bugfix);
            code = HTTP_REPLY_OK;
            break;
        default:
            break;
    }
    return code;
}

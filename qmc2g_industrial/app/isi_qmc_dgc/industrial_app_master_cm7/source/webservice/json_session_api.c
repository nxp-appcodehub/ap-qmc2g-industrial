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

#include "api_qmc_common.h"
#include "api_usermanagement.h"
#include "mbedtls/platform_util.h"
#include "plug_json.h"
#include "json_string.h"
#include "core_json.h"
#include "json_api_common.h"

static char s_token_temp_buffer[512];

PLUG_JSON_CB(json_session_api);

/**
 * @brief session api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */

http_status_t json_session_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    int code            = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
    qmc_status_t status = kStatus_QMC_Ok;
    JSONTypes_t type;
    JSONStatus_t jstatus;
    usrmgmt_session_id user_sid;
    char *user, *pass;
    size_t user_length;
    size_t pass_length;
    size_t length;
    usrmgmt_session_id key_sid            = -1;
    const usrmgmt_session_t *user_session = request->session;

    if (request->path_match_end && request->path_match_end[0] != 0)
    {
        unsigned long val;
        char *endptr;
        val = strtoul(request->path_match_end, &endptr, 10);
        if (endptr && *endptr == '\0'&& val < USRMGMT_MAX_SESSIONS)
        {
            key_sid = val;
        }
    }

    switch (request->method)
    {
        case HTTP_POST:
            /* create a session */
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("user"), &user, &user_length, &type);
            if (jstatus != JSONSuccess)
            {
                break;
            }
            if (type != JSONString)
            {
                break;
            }

            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("passphrase"), &pass, &pass_length, &type);
            if (jstatus != JSONSuccess)
            {
                break;
            }
            if (type != JSONString)
            {
                break;
            }

            user[user_length] = '\0';
            pass[pass_length] = '\0';
            code              = HTTP_REPLY_OK;
            length            = sizeof(s_token_temp_buffer);
            status            = USRMGMT_CreateSession((unsigned char *)s_token_temp_buffer, &length, &user_sid, NULL,
                                                      (unsigned char *)user, (unsigned char *)pass);
            if (status == kStatus_QMC_Err)
            {
                code = HTTP_NOREPLY_UNAUTHORIZED;
                break;
            }
            status = USRMGMT_ValidateSession((unsigned char *)s_token_temp_buffer, strlen(s_token_temp_buffer),
                                             &user_sid, &user_session);
            if (status == kStatus_QMC_Err)
            {
                code = HTTP_NOREPLY_UNAUTHORIZED;
                break;
            }

            if (webservice_check_error(status, response))
            {
                fprintf(response, "{\"sid\":%i, \"token\":", user_sid);
                fputs_json_string(s_token_temp_buffer, response);
                fprintf(response, ", \"payload\":%s", user_session->token_payload);
                fputs("}\n", response);
            }
            break;
        case HTTP_GET:
            /* list active sessions */
            if (!user_session)
            {
                code = HTTP_NOREPLY_UNAUTHORIZED;
            }
            else
            {
                int count                        = 0;
                int n                            = 0;
                usrmgmt_session_id sid           = 0;
                const usrmgmt_session_t *session = NULL;
                code                             = HTTP_NOREPLY_NOT_FOUND;
                if (key_sid < 0)
                {
                    fputs("[", response);
                }
                do
                {
                    status = USRMGMT_IterateSessions(&count, &sid, &session);
                    if (status != kStatus_QMC_Ok)
                    {
                        break;
                    }
                    if (key_sid < 0 || key_sid == sid)
                    {
                    	if (n++ > 0 && key_sid < 0)
                    	{
                    		fputs(",\n", response);
                    	}
                    	code = HTTP_REPLY_OK;
                        fputs((char *)session->token_payload, response);
                    }
                    if (key_sid == sid)
                    {
                        break;
                    }
                } while (true);
                if (key_sid < 0)
                {
                    fputs("]\n", response);
                }
            }
            break;
        case HTTP_DELETE:
            /* list active sessions */
            do
            {
                code = HTTP_NOREPLY_UNAUTHORIZED;
                if (user_session)
                {
                    code = HTTP_REPLY_NO_CONTENT;
                    if (key_sid < 0) /* no sid provided in path */
                    {
                        /* logout current user */
                        status           = USRMGMT_EndSession(user_session->sid,user_session->sid);
                        request->session = NULL;
                    }
                    else if (user_session->role == kUSRMGMT_RoleMaintenance)
                    {
                        /* logout provided session  */
                        status = USRMGMT_EndSession(user_session->sid,key_sid);
                        if (status != kStatus_QMC_Ok)
                        {
                            code = HTTP_REPLY_NOT_FOUND;
                        }
                    }
                    else {
                    	code = HTTP_NOREPLY_FORBIDDEN;
                    }
                }
            } while (false);
        default:
            break;
    }
    mbedtls_platform_zeroize(s_token_temp_buffer, sizeof(s_token_temp_buffer));
    return code;
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "api_configuration.h"
#include "api_qmc_common.h"
#include "api_usermanagement.h"
#include "json_string.h"
#include "plug.h"
#include "webservice_logging_task.h"

PLUG_EXTENSION_REGISTER_HEADER(s_auth_field, FIELD_HASH_AUTHORIZATION, 1024, 0);

/**
 * @struct authorization state
 */
struct authorization
{
    char *bearer_token; /*!< pointer to the token provided */
    bool did_check; /*!< if the token has been already validated (if a body was uploaded) */
    http_status_t status_code; /*!< reply status code from rule's flags */
};

PLUG_EXTENSION(qmc_authorize, struct authorization, &s_auth_field);

/**
 * @brief try to authorize a user by validating the Authorization Header
 *
 * replies HTTP_ACCEPT (0) if authorization is successful
 * replies the status code from the rules flags field otherwise
 * 
 * if flags is HTTP_ACCEPT, it does not enforce authorization.
 * 
 * request->session is populated only if authorization is successful
 *
 */
http_status_t qmc_authorize_on_reply(plug_state_p state,
                                     const plug_rule_t *rule,
                                     plug_request_t *request,
                                     plug_response_t *response,
                                     http_status_t status_code)
{
    usrmgmt_session_id sid;
    const usrmgmt_session_t *session;
    /* flags is used to enforce authorization */

    /* only if not already checked, might already been called by on_body_data */
    if (!state->did_check)
    {
        state->did_check = true;
        /* load default response from flags */
        state->status_code = rule->flags > 0 ? HTTP_NOREPLY_UNAUTHORIZED : HTTP_ACCEPT;
        if (*s_auth_field.is_set)
        {
            qmc_status_t status;

            if (strncasecmp(s_auth_field.value_ptr, "Bearer ", 7) == 0)
            {
                const char *bearer_token = &s_auth_field.value_ptr[7];
                const size_t size = s_auth_field.value_size - 7;
                const size_t len  = strnlen(bearer_token, size);

                status = USRMGMT_ValidateSession((unsigned char *)bearer_token, len, &sid, &session);
                if (status == kStatus_QMC_Ok)
                {
                    state->status_code = HTTP_ACCEPT;
                    request->session   = (void *)session;
                }
            }
        }
        /* if the client accepts json, pretend it does not accept html to use json encoding by default for the authorized endpoints */
        if (request->accept_json)
        {
            request->accept_html = 0;
        }
    }
    return state->status_code;
}

/**
 * @brief authorize when body data is read.
 *
 * this is called before the on_reply callbacks.
 * other plugs might want the session to be available when receiving a body.
 * so if there is a body authentication is done early.
 *
 */
http_status_t qmc_authorize_on_body_data(plug_state_p state,
                                         const plug_rule_t *rule,
                                         plug_request_t *request,
                                         plug_response_t *response,
                                         FILE *body,
                                         u16_t len)
{
    /* only if not already checked, this function might be called multiple times */
    if (!state->did_check)
    {
        qmc_authorize_on_reply(state, rule, request, response, 0);
    }
    return state->status_code;
}

/** @brief send authenticate headers.
 *
 * realm string from rules 'options' field pointer.
 *
 */
void qmc_authorize_write_headers(plug_state_p state,
                                 const plug_rule_t *rule,
                                 plug_request_t *request,
                                 plug_response_t *response,
                                 http_status_t status_code,
                                 FILE *headers)
{
    fputs("Cache-Control: no-cache\r\n", headers);
    fputs("WWW-Authenticate: Bearer realm=", headers);
    fputs_json_string((const char *)rule->options, headers);
    fputs("\r\n", headers);
}

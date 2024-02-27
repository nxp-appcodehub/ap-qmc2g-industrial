/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "plug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

PLUG_EXTENSION(plug_redirect);


/**
 * @brief  set rule's status code, on_reply callback
 */
http_status_t plug_redirect_on_match(
    plug_state_p state, const plug_rule_t *rule, plug_request_t *request)
{
    http_status_t status_code=HTTP_ACCEPT;
    if (rule->flags)
    {
        status_code = rule->flags;
    }
    return status_code;
}
/**
 * @brief write_headers callback
 * write the Location header
 *
 * @param state
 * @param rule
 * @param request
 * @param response
 * @param status_code
 * @param headers
 */
void plug_redirect_write_headers(plug_state_p state,
                                 const plug_rule_t *rule,
                                 plug_request_t *request,
                                 plug_response_t *response,
                                 http_status_t status_code,
                                 FILE *headers)
{
    static const char *const fmt = "Location: %s\r\n";
    const char *target           = rule->options;
    fprintf(headers, fmt, target);
}


/**
 * @brief  set rule's status code, on_reply callback 
 */
http_status_t plug_redirect_on_reply(
    plug_state_p state, const plug_rule_t *rule, plug_request_t *request, plug_response_t *response, http_status_t status_code)
{

    response->location = rule->options;
    return status_code;
}

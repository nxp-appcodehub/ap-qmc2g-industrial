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

PLUG_EXTENSION(qmc_check_session);



/**
 * @brief  check if the session field is set, or deny access
 */
http_status_t qmc_check_session_on_reply(
    plug_state_p state, const plug_rule_t *rule, plug_request_t *request, plug_response_t *response, http_status_t status_code)
{
    return request->session? HTTP_ACCEPT : HTTP_NOREPLY_UNAUTHORIZED;
}

/**
 * @brief  check if the session field is set, or deny access
 *
 * for body uploads. as they are processed earlyer.
 */
http_status_t qmc_check_session_on_body_data(plug_state_p state,
                               const plug_rule_t *rule,
                               plug_request_t *request,
                               plug_response_t *response,
                               FILE *body,
                               u16_t len)
{
    return request->session? HTTP_ACCEPT : HTTP_NOREPLY_UNAUTHORIZED;
}


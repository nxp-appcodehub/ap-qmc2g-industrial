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
#include "api_usermanagement.h"
#include "plug.h"
#include "webservice_logging_task.h"

PLUG_EXTENSION(qmc_logging);

void qmc_logging_on_cleanup(plug_state_p state,
                              const plug_rule_t *rule,
                              const plug_request_t *request,
                              const plug_response_t *response,
                              http_status_t status_code)
{
    const usrmgmt_session_t *session = request->session;

    Webservice_IncrementErrorCounter(status_code, session ? session->uid : kCONFIG_KeyNone);
}

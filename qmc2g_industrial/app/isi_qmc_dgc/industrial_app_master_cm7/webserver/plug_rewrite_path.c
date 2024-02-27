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
#include <stdio.h>

PLUG_EXTENSION(plug_rewrite_path);
/* this simple plug to change the url, it returns 0 and will therefore be only called on match */
/* optionally rewrite the HTTP method (flags) */

http_status_t plug_rewrite_path_on_match(plug_state_p state, const plug_rule_t *rule, plug_request_t *request)
{
    /* rewrite method */
    if (rule->flags)
    {
        request->method = rule->flags;
    }

    size_t len;
    /* rewrite PATH */
    len=snprintf(request->path, sizeof(request->path), "%s", (const char *)rule->options);
    if(len<1){
      return HTTP_NOREPLY_INTERNAL_SERVER_ERROR;
    }
    request->path_match_end=request->path+len;
    return HTTP_ACCEPT;
}

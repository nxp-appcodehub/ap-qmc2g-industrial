/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "cit_httpd_opt.h"
#include "plug.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "plug_status.h"

struct status_state
{
    enum plug_status_mode mode;
};

PLUG_EXTENSION(plug_status, struct status_state);
/* static u16_t s_response_len; */

static const char s_html_format[] =
    "<!DOCTYPE html>\n"
    "<html style=\"height: 100%%;\">"
    "<head>\n"
    "<title>%s</title>\n"
    "%s"
    "</head>"
    "<body style=\"color: #999; margin: 0; font-family: sans-serif;\">"
    "<div id=\"main\" style=\" display: table; width: 100%%; height: 100vh; text-align: center; \">"
    "<div style=\" display: table-cell; vertical-align: middle; \">"
    "<h1 style=\" font-size: 16; display: inline-block; padding-right: 12px;\">\n"
    "%d : %s\n"
    "</h1>"
    "</div>"
    "</div>"
    "</body>"
    "</html>"
    "\n";

#define META_FORMAT "<meta http-equiv=\"refresh\" content=\"4; URL=%s\">\n"
#define JSON_FORMAT "{\"code\":%d, \"message\":\"%s\"%s}\n"

static char s_plug_status_meta_buffer[sizeof(META_FORMAT) + CIT_HTTPD_MAX_URI_LEN];
static char s_plug_status_buffer[sizeof(s_html_format) + sizeof(s_plug_status_meta_buffer) + 128];

/**
 * @brief  set the rules status code, on_match callback
 */
http_status_t plug_status_on_match(plug_state_p state, const plug_rule_t *rule, plug_request_t *request)
{
    /* use flags from the rule as status code default */
    return rule->flags;
}

/**
 * @brief generate error reply if any status code is set
 */
http_status_t plug_status_on_reply(plug_state_p state,
                                   const plug_rule_t *rule,
                                   plug_request_t *request,
                                   plug_response_t *response,
                                   http_status_t status_code)
{
    const char *msg;
    status_code = REPLY_STATUS_CODE(status_code);
    msg         = status_code_string(status_code);

    if (rule)
    {
        response->location = rule->options;
        /* overwrite if set from the rule */
        if (rule->flags)
        {
            status_code = rule->flags;
        }
        /* use error handler */
        if (status_code == 0)
        {
            return status_code;
        }
    }
    response->content_length = 0;
    if (request->accept_html)
    {
        state->mode            = HTML_STATUS;
        response->content_type = "text/html";
    }
    else if (request->accept_json)
    {
        state->mode            = JSON_STATUS;
        response->content_type = "application/json";
    }
    else
    {
        response->content_type = "text/plain";
        state->mode            = TEXT_STATUS;
    }
    *s_plug_status_meta_buffer = '\0';
    if (status_code >= HTTP_REPLY_MULTIPLE_CHOICES && status_code < HTTP_REPLY_BAD_REQUEST && response->location)
    {
        switch (state->mode)
        {
            case HTML_STATUS:
                snprintf(s_plug_status_meta_buffer, sizeof(s_plug_status_meta_buffer), META_FORMAT, response->location);
                break;
            case JSON_STATUS:
                snprintf(s_plug_status_meta_buffer, sizeof(s_plug_status_meta_buffer), ", \"redirect\"=\"%s\"",
                         response->location);
                break;
            default:
                snprintf(s_plug_status_meta_buffer, sizeof(s_plug_status_meta_buffer), "\"%s\"", response->location);
                break;
        }
    }

    switch (state->mode)
    {
        case HTML_STATUS:
            response->content_length = snprintf(s_plug_status_buffer, sizeof(s_plug_status_buffer), &*s_html_format,
                                                msg, s_plug_status_meta_buffer, status_code, msg);
            break;
        case JSON_STATUS:
            response->content_length = snprintf(s_plug_status_buffer, sizeof(s_plug_status_buffer), JSON_FORMAT,
                                                status_code, msg, s_plug_status_meta_buffer);
            break;
        default:
            response->content_length = snprintf(s_plug_status_buffer, sizeof(s_plug_status_buffer), "%3d %s\n%s\n",
                                                status_code, msg, s_plug_status_meta_buffer);
            break;
    }
    return status_code;
}

/**
 * @brief write the body data for the request, write_body_data callback
 */
void plug_status_write_body_data(plug_state_p state,
                                 const plug_rule_t *rule,
                                 plug_request_t *request,
                                 const plug_response_t *response,
                                 http_status_t status_code,
                                 FILE *body,
                                 u16_t sndbuf)
{
    if (status_code == HTTP_REPLY_NOT_MODIFIED)
    {
        return;
    }
    fputs(s_plug_status_buffer, body);
}

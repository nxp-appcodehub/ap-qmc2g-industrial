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

static char PLUG_TMP_BUFFER_SECTION s_plug_json_buffer[TCP_SND_BUF];

#ifdef PLUG_JSON_INCLUDE_FILE
#include PLUG_JSON_INCLUDE_FILE
#endif

#include "constants.h"
#include <string.h>
#include <stdio.h>
#include <lwip/mem.h>
#include "plug_json.h"

PLUG_EXTENSION_REGISTER_HEADER(s_content_type_field,
                               FIELD_HASH_CONTENT_TYPE,
                               sizeof("application/json"),
                               sizeof("Content-Type"));

/**
 * @brief plug state for json processing
 */
struct plug_json_state
{
    void *request_json;
    void *response_json;
    void *session;
    u16_t json_len;
    u16_t valid_json : 1;
};

PLUG_EXTENSION(plug_json, struct plug_json_state, &s_content_type_field);

/**
 * @brief plug on_body callback
 * validates request headers and validates body data is valid JSON
 */
http_status_t plug_json_on_body_data(plug_state_p state,
                                     const plug_rule_t *rule,
                                     plug_request_t *request,
                                     plug_response_t *response,
                                     FILE *body,
                                     u16_t len)
{
    int code        = 0;
    u16_t mem_limit = rule->flags;
    // if the data fits into the limit, and there is data, and the content type was application/json
    if (request->content_length > mem_limit)
    {
        return HTTP_NOREPLY_PAYLOAD_TOO_LARGE;
    }
    int valid_content_type = *s_content_type_field.is_set;
    if (valid_content_type)
    {
        if (lwip_strnicmp(s_content_type_field.value_ptr, "application/json", s_content_type_field.value_size))
        {
            valid_content_type = 0;
        }

        if (lwip_strnicmp(s_content_type_field.field_ptr, "Content-Type", s_content_type_field.field_size))
        {
            valid_content_type = 0;
        }
    }
    if (!valid_content_type)
    {
        return HTTP_NOREPLY_UNSUPPORTED_MEDIA_TYPE;
    }
    if (len <= mem_limit && request->content_length > 0 && valid_content_type)
    {
        u16_t count;
        if (!state->request_json)
        {
            state->request_json = mem_calloc(1, request->content_length + 1);
        }
        if (!state->request_json)
        {
            return HTTP_NOREPLY_INTERNAL_SERVER_ERROR; // malloc failed
        }

        /*
         * seek to the beginning, we do not incrementally read, and the whole body is currently kept anyway
         */
        state->json_len                      = 0;
        count                                = fread(state->request_json, 1, len, body);
        ((char *)state->request_json)[count] = '\0';

        // check if file is complete otherwise wait
        if (count == request->content_length)
        {
            if (PLUG_JSON_VALIDATE(state->request_json, count))
            {
                state->valid_json = 1;
            }
            else
            {
                mem_free(state->request_json);
                state->request_json = NULL;
            }
        }
        else
        {
            fseek(body, 0, SEEK_SET);
        }
    }
    else
    {
        // skip body content, wrong content type
        fseek(body, 0, SEEK_END);
    }
    return code;
}

/**
 * @brief on cleanup callback
 *
 * cleanup internal buffers
 *
 * @param state plug state struct
 * @param rule  rule belonging to this invocation
 */
void plug_json_on_cleanup(plug_state_p state,
                          const plug_rule_t *rule,
                          const plug_request_t *request,
                          const plug_response_t *response,
                          http_status_t status_code)
{
    if (state->request_json)
    {
        mem_free(state->request_json);
    }
    if (state->response_json)
    {
        mem_free(state->response_json);
    }
    *state = (struct plug_json_state){0};
}

/**
 * @brief on_reply callback
 * generate response if any status code is set
 */
http_status_t plug_json_on_reply(plug_state_p state,
                                 const plug_rule_t *rule,
                                 plug_request_t *request,
                                 plug_response_t *response,
                                 http_status_t status_code)
{
    json_cb callback_fn      = rule->options;
    response->content_length = 0;
    json_response_t resp     = JSON_RESPONSE_T_INITIALIZER;
    // validate the content type header explicitly, to avoid any hash collisions.
    // the content type might be used by an web firewall, so this is extra paranoid,

    http_status_t rc;
    response->content_type = "application/json";

    if (status_code)
    {
        return status_code;
    }

    // ensure the client can accept json
    if (!request->accept_json)
    {
        return HTTP_NOREPLY_NOT_ACCEPTABLE;
    }

    // prefer to send json response from plug json. this influences the error handler
    request->accept_html=0;

    // no json document is defined as valid input
    if (request->content_length == 0)
    {
        state->valid_json = 1;
    }
    // anything that has potentially a body, should have a valid body if any
    if (request->method & HTTP_ANY_RECV)
    {
        if (!state->valid_json)
        {
            return HTTP_NOREPLY_BAD_REQUEST;
        }
        if (request->content_length > rule->flags)
        {
            return HTTP_NOREPLY_PAYLOAD_TOO_LARGE;
        }
    }

    // etag buffer
    response->buffer[0] = '\0';
    // callback
    FILE *json_stream = fmemopen(s_plug_json_buffer, sizeof(s_plug_json_buffer), "w");
    if (!json_stream)
    {
        return HTTP_NOREPLY_SERVICE_UNAVAILABLE;
    }

    rc = callback_fn(request, state->request_json, request->content_length, &resp, json_stream);

    if (state->request_json)
    {
        mem_free(state->request_json);
        state->request_json = NULL;
    }

    if (resp.etag)
    {
        response->etag = resp.etag;
    }
    if (rc < HTTP_ACCEPT)
    {
        // callback wants to generate an error
        fclose(json_stream);
        return rc;
    }

    int pos                  = ftell(json_stream);
    response->content_length = LWIP_MAX(0, pos);
    fclose(json_stream);
    if (response->content_length > 0 && request->method != HTTP_HEAD)
    {
        state->response_json = mem_malloc(response->content_length);
        if (state->response_json)
        {
            memcpy(state->response_json, s_plug_json_buffer, response->content_length);
        }
    }
    return rc ? REPLY_STATUS_CODE(rc) : HTTP_REPLY_OK;
}

/**
 * @brief write_headers callback.
 * set location header if this is a HTTP_REPLY_SEE_OTHER response
 *
 */
void plug_json_write_headers(plug_state_p state,
                             const plug_rule_t *rule,
                             plug_request_t *request,
                             plug_response_t *response,
                             http_status_t status_code,
                             FILE *headers)
{
    // allow a SEE_OTHER to the same resource
    if (REPLY_STATUS_CODE(status_code) == HTTP_REPLY_SEE_OTHER)
    {
        fprintf(headers, "Location: %s\r\n", request->uri);
    }
}

/**
 * @brief write_body callback
 * send response data from json callback
 */
void plug_json_write_body_data(plug_state_p state,
                               const plug_rule_t *rule,
                               plug_request_t *request,
                               const plug_response_t *response,
                               http_status_t status_code,
                               FILE *body,
                               u16_t sndbuf)
{
    if (response->content_length && state->response_json)
    {
        fwrite(state->response_json, response->content_length, 1, body);
    }
    if (state->response_json)
    {
      mem_free(state->response_json);
      state->response_json = NULL;
    }
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CORE_IOT_HTTPD_PLUG_PLUG_JSON_H_
#define CORE_IOT_HTTPD_PLUG_PLUG_JSON_H_

#include "cit_httpd_opt.h"
#include "plug.h"
/**
 *   plug_json provides a simplified callback system for json endpoints 
 */

PLUG_EXTENSION_PROTOTYPE(plug_json);

#define JSON_SCRATCH_BUFFER_SIZE CIT_HTTPD_SCRATCH_BUFFER_SIZE


/**
 * @brief a json response.
 */
typedef struct json_response
{
    char *buffer; /*!< points to the scratch buffer for a json response. */
    char *etag;   /*!< can be used to provide a custom etag */
    char *headers; /*!< point to any extra headers to send for this response */
    const u16_t buffer_size; /*!< the size of the scratch buffer provided */
} json_response_t;

#define JSON_RESPONSE_T_INITIALIZER \
    {.buffer = response->buffer, .buffer_size = sizeof(response->buffer), .etag = NULL, .headers = NULL};

#define _JSON_CB_ARGS() \
    (plug_request_t * request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response);


/**
 * @typedef plug json callback type
 */
typedef http_status_t(*json_cb) _JSON_CB_ARGS();


#define PLUG_JSON_CB_PROTOTYPE(cbname) http_status_t cbname _JSON_CB_ARGS();

#define PLUG_JSON_CB(cbname)                                \
    GCC_PRAGMA(GCC diagnostic error "-Wmissing-prototypes") \
    http_status_t cbname _JSON_CB_ARGS();

#endif

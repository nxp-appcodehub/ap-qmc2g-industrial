/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <lwip/opt.h>
#include <ctype.h>

#ifndef CIT_HTTPD_CONSTANTS_H_
#define CIT_HTTPD_CONSTANTS_H_

/* typecast http_status_t to it's absolute value, turning HTTP_NOREPLY_... to HTTP_REPLY_... */
#define REPLY_STATUS_CODE(code)   ((http_status_t)abs((int)(code)))
#define NOREPLY_STATUS_CODE(code) ((http_status_t)-abs((int)(code)))

/**
 * @enum http status codes
 */
typedef enum http_status
{
    /* internal states for response handling */
    HTTP_ACCEPT = (0),
    HTTP_REJECT = (-1),

    /* status codes generating a reply from the answering plug*/
    HTTP_REPLY_CONTINUE                        = (100),
    HTTP_REPLY_SWITCHING_PROTOCOLS             = (101),
    HTTP_REPLY_PROCESSING                      = (102),
    HTTP_REPLY_OK                              = (200),
    HTTP_REPLY_CREATED                         = (201),
    HTTP_REPLY_ACCEPTED                        = (202),
    HTTP_REPLY_NON_AUTHORITATIVE_INFORMATION   = (203),
    HTTP_REPLY_NO_CONTENT                      = (204),
    HTTP_REPLY_RESET_CONTENT                   = (205),
    HTTP_REPLY_PARTIAL_CONTENT                 = (206),
    HTTP_REPLY_MULTI_STATUS                    = (207),
    HTTP_REPLY_ALREADY_REPORTED                = (208),
    HTTP_REPLY_IM_USED                         = (226),
    HTTP_REPLY_MULTIPLE_CHOICES                = (300),
    HTTP_REPLY_MOVED_PERMANENTLY               = (301),
    HTTP_REPLY_FOUND                           = (302),
    HTTP_REPLY_SEE_OTHER                       = (303),
    HTTP_REPLY_NOT_MODIFIED                    = (304),
    HTTP_REPLY_USE_PROXY                       = (305),
    HTTP_REPLY_SWITCH_PROXY                    = (306),
    HTTP_REPLY_TEMPORARY_REDIRECT              = (307),
    HTTP_REPLY_PERMANENT_REDIRECT              = (308),
    HTTP_REPLY_BAD_REQUEST                     = (400),
    HTTP_REPLY_UNAUTHORIZED                    = (401),
    HTTP_REPLY_PAYMENT_REQUIRED                = (402),
    HTTP_REPLY_FORBIDDEN                       = (403),
    HTTP_REPLY_NOT_FOUND                       = (404),
    HTTP_REPLY_METHOD_NOT_ALLOWED              = (405),
    HTTP_REPLY_NOT_ACCEPTABLE                  = (406),
    HTTP_REPLY_PROXY_AUTHENTICATION_REQUIRED   = (407),
    HTTP_REPLY_REQUEST_TIMEOUT                 = (408),
    HTTP_REPLY_CONFLICT                        = (409),
    HTTP_REPLY_GONE                            = (410),
    HTTP_REPLY_LENGTH_REQUIRED                 = (411),
    HTTP_REPLY_PRECONDITION_FAILED             = (412),
    HTTP_REPLY_PAYLOAD_TOO_LARGE               = (413),
    HTTP_REPLY_URI_TOO_LONG                    = (414),
    HTTP_REPLY_UNSUPPORTED_MEDIA_TYPE          = (415),
    HTTP_REPLY_RANGE_NOT_SATISFIABLE           = (416),
    HTTP_REPLY_EXPECTATION_FAILED              = (417),
    HTTP_REPLY_MISDIRECTED_REQUEST             = (421),
    HTTP_REPLY_UNPROCESSABLE_ENTITY            = (422),
    HTTP_REPLY_LOCKED                          = (423),
    HTTP_REPLY_FAILED_DEPENDENCY               = (424),
    HTTP_REPLY_UPGRADE_REQUIRED                = (426),
    HTTP_REPLY_PRECONDITION_REQUIRED           = (428),
    HTTP_REPLY_TOO_MANY_REQUESTS               = (429),
    HTTP_REPLY_REQUEST_HEADER_FIELDS_TOO_LARGE = (431),
    HTTP_REPLY_UNAVAILABLE_FOR_LEGAL_REASONS   = (451),
    HTTP_REPLY_INTERNAL_SERVER_ERROR           = (500),
    HTTP_REPLY_NOT_IMPLEMENTED                 = (501),
    HTTP_REPLY_BAD_GATEWAY                     = (502),
    HTTP_REPLY_SERVICE_UNAVAILABLE             = (503),
    HTTP_REPLY_GATEWAY_TIMEOUT                 = (504),
    HTTP_REPLY_HTTP_VERSION_NOT_SUPPORTED      = (505),
    HTTP_REPLY_VARIANT_ALSO_NEGOTIATES         = (506),
    HTTP_REPLY_INSUFFICIENT_STORAGE            = (507),
    HTTP_REPLY_LOOP_DETECTED                   = (508),
    HTTP_REPLY_NOT_EXTENDED                    = (510),
    HTTP_REPLY_NETWORK_AUTHENTICATION_REQUIRED = (511),

    /* status codes generating a reply using the error handling response path*/
    /* this does not need to be an error, it states this plug will not generate the response itself*/
    HTTP_NOREPLY_CONTINUE                        = (-100),
    HTTP_NOREPLY_SWITCHING_PROTOCOLS             = (-101),
    HTTP_NOREPLY_PROCESSING                      = (-102),
    HTTP_NOREPLY_OK                              = (-200),
    HTTP_NOREPLY_CREATED                         = (-201),
    HTTP_NOREPLY_ACCEPTED                        = (-202),
    HTTP_NOREPLY_NON_AUTHORITATIVE_INFORMATION   = (-203),
    HTTP_NOREPLY_NO_CONTENT                      = (-204),
    HTTP_NOREPLY_RESET_CONTENT                   = (-205),
    HTTP_NOREPLY_PARTIAL_CONTENT                 = (-206),
    HTTP_NOREPLY_MULTI_STATUS                    = (-207),
    HTTP_NOREPLY_ALREADY_REPORTED                = (-208),
    HTTP_NOREPLY_IM_USED                         = (-226),
    HTTP_NOREPLY_MULTIPLE_CHOICES                = (-300),
    HTTP_NOREPLY_MOVED_PERMANENTLY               = (-301),
    HTTP_NOREPLY_FOUND                           = (-302),
    HTTP_NOREPLY_SEE_OTHER                       = (-303),
    HTTP_NOREPLY_NOT_MODIFIED                    = (-304),
    HTTP_NOREPLY_USE_PROXY                       = (-305),
    HTTP_NOREPLY_SWITCH_PROXY                    = (-306),
    HTTP_NOREPLY_TEMPORARY_REDIRECT              = (-307),
    HTTP_NOREPLY_PERMANENT_REDIRECT              = (-308),
    HTTP_NOREPLY_BAD_REQUEST                     = (-400),
    HTTP_NOREPLY_UNAUTHORIZED                    = (-401),
    HTTP_NOREPLY_PAYMENT_REQUIRED                = (-402),
    HTTP_NOREPLY_FORBIDDEN                       = (-403),
    HTTP_NOREPLY_NOT_FOUND                       = (-404),
    HTTP_NOREPLY_METHOD_NOT_ALLOWED              = (-405),
    HTTP_NOREPLY_NOT_ACCEPTABLE                  = (-406),
    HTTP_NOREPLY_PROXY_AUTHENTICATION_REQUIRED   = (-407),
    HTTP_NOREPLY_REQUEST_TIMEOUT                 = (-408),
    HTTP_NOREPLY_CONFLICT                        = (-409),
    HTTP_NOREPLY_GONE                            = (-410),
    HTTP_NOREPLY_LENGTH_REQUIRED                 = (-411),
    HTTP_NOREPLY_PRECONDITION_FAILED             = (-412),
    HTTP_NOREPLY_PAYLOAD_TOO_LARGE               = (-413),
    HTTP_NOREPLY_URI_TOO_LONG                    = (-414),
    HTTP_NOREPLY_UNSUPPORTED_MEDIA_TYPE          = (-415),
    HTTP_NOREPLY_RANGE_NOT_SATISFIABLE           = (-416),
    HTTP_NOREPLY_EXPECTATION_FAILED              = (-417),
    HTTP_NOREPLY_MISDIRECTED_REQUEST             = (-421),
    HTTP_NOREPLY_UNPROCESSABLE_ENTITY            = (-422),
    HTTP_NOREPLY_LOCKED                          = (-423),
    HTTP_NOREPLY_FAILED_DEPENDENCY               = (-424),
    HTTP_NOREPLY_UPGRADE_REQUIRED                = (-426),
    HTTP_NOREPLY_PRECONDITION_REQUIRED           = (-428),
    HTTP_NOREPLY_TOO_MANY_REQUESTS               = (-429),
    HTTP_NOREPLY_REQUEST_HEADER_FIELDS_TOO_LARGE = (-431),
    HTTP_NOREPLY_UNAVAILABLE_FOR_LEGAL_REASONS   = (-451),
    HTTP_NOREPLY_INTERNAL_SERVER_ERROR           = (-500),
    HTTP_NOREPLY_NOT_IMPLEMENTED                 = (-501),
    HTTP_NOREPLY_BAD_GATEWAY                     = (-502),
    HTTP_NOREPLY_SERVICE_UNAVAILABLE             = (-503),
    HTTP_NOREPLY_GATEWAY_TIMEOUT                 = (-504),
    HTTP_NOREPLY_HTTP_VERSION_NOT_SUPPORTED      = (-505),
    HTTP_NOREPLY_VARIANT_ALSO_NEGOTIATES         = (-506),
    HTTP_NOREPLY_INSUFFICIENT_STORAGE            = (-507),
    HTTP_NOREPLY_LOOP_DETECTED                   = (-508),
    HTTP_NOREPLY_NOT_EXTENDED                    = (-510),
    HTTP_NOREPLY_NETWORK_AUTHENTICATION_REQUIRED = (-511),
} http_status_t;
/**
 * @brief HTTP methods enum
 */
enum http_method
{
    HTTP_NO_METHOD     = 0u,
    HTTP_GET           = (1u << 0 | 1u << 1), /* will imply HEAD on dispatch */
    HTTP_HEAD          = (1u << 1),
    HTTP_POST          = (1u << 2),
    HTTP_PUT           = (1u << 3),
    HTTP_DELETE        = (1u << 4),
    HTTP_CONNECT       = (1u << 5),
    HTTP_OPTIONS       = (1u << 6),
    HTTP_TRACE         = (1u << 7),
    HTTP_PATCH         = (1u << 8),
    HTTP_ERROR_HANDLER = (1u << 15),                                          /* HANDLE_ERRORS */
    HTTP_ANY_RECV      = (HTTP_POST | HTTP_PUT | HTTP_PATCH), /* methods with a body */
    HTTP_ANY           = (0x1Fu),                                             /* all methods */
};

/*
 * headers are identified by the djb2 hash of their ** lower case ** field name.
 *
 * field_hash("Content-Type") -> djb2_hash("content-type") -> 0xfa6585af
 *
 * functions for calculating the hash are provided below (field_hash_init,field_hash_update,field_hash).
 * the parser generates field hashes during input processing.
 *
 * if you add new values, you can use this short Python script (just enter the field names)
 *

#! /usr/bin/env python3
import fileinput
def hash_djb2(s):
    hash = 538hash = 53811
    for c in s:
        hash = (( hash << 5) + hash) + ord(c)
    return hash & 0xFFFFFFFF

for line in fileinput.input():
    field = line.strip().lower();
    name = ''.join(ch if ch.isalnum() else '_' for ch in field.upper())
    print('FIELD_HASH_%s=0x%08x,' % (name, hash_djb2(field))) */

/*
 * functions for calculating the hash on strings.
 *
 * */

/* Header Field Names */
enum FIELD_HASH
{
    FIELD_HASH_HOST                      = 0x7c97fe63u,
    FIELD_HASH_USER_AGENT                = 0x4fdea600u,
    FIELD_HASH_ACCEPT                    = 0xf15ae9b5u,
    FIELD_HASH_AUTHORIZATION             = 0x5f1f78f6u,
    FIELD_HASH_ACCEPT_LANGUAGE           = 0xe8c17a46u,
    FIELD_HASH_ACCEPT_ENCODING           = 0x42889389u,
    FIELD_HASH_CONNECTION                = 0xd2627cd5u,
    FIELD_HASH_UPGRADE_INSECURE_REQUESTS = 0x03441701u,
    FIELD_HASH_CONTENT_TYPE              = 0xfa6585afu,
    FIELD_HASH_CONTENT_LENGTH            = 0x15c97d6fu,
    FIELD_HASH_EXPECT                    = 0xfc32ae0eu,
    FIELD_HASH_IF_MATCH                  = 0x7f4b8dceu,
    FIELD_HASH_IF_NONE_MATCH             = 0x7f9fd72bu,
};

enum MIME_HASH
{
    MIME_HASH_ALL              = 0x0b876bc8u,
    MIME_HASH_TEXT_HTML        = 0x17e9224eu,
    MIME_HASH_APPLICATION_JSON = 0x78166262u,
};

/* for use in loops, init then update */
static inline void field_hash_init(u32_t *hash)
{
    *hash = 5381;
}

/* remember, this is case insensitive ! */
static inline void field_hash_update(u32_t *hash, unsigned char c)
{
    *hash = ((*hash << 5) + *hash) + (u32_t)tolower(c);
}

/* a pure static inline function is marked unused. add this exception to allow it */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

/* case ignoring hash for use on strings */
static inline u32_t field_hash(unsigned char *s)
{
    u32_t hash;
    field_hash_init(&hash);
    unsigned char c;
    /* coverity[overflow_sink] */
    while ((c = *s++))
    {
        field_hash_update(&hash, c);
    }
    return hash;
}

#pragma GCC diagnostic pop

enum http_method lookup_http_method(const char *str, u16_t len);
const char *status_code_string(int code);
const char *lookup_mimetype(const char *ext);

#endif

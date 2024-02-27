/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "constants.h"

/**
 * @brief parse a http method string
 *
 * @param str method name
 * @param len length of method string
 *
 * @return httpd_method enum 
 */
enum http_method lookup_http_method(const char *str, u16_t len)
{
    const char *rest = NULL; /* stop comparison */
    const char *p    = str;
    const char *pe    = str+len;
    unsigned char c;
    enum http_method m = HTTP_NO_METHOD; /* default return */
    if (p<pe && len <= 7 && str != NULL)
    {
        c = (unsigned char)*p++;
        len--;
        switch (toupper((int)c))
        {
            case 'G':
                rest = "ET";
                m    = HTTP_GET;
                break;
            case 'H':
                rest = "EAD";
                m    = HTTP_HEAD;
                break;
            case 'D':
                rest = "ELETE";
                m    = HTTP_DELETE;
                break;
            case 'C':
                rest = "ONNECT";
                m    = HTTP_CONNECT;
                break;
            case 'O':
                rest = "PTIONS";
                m    = HTTP_OPTIONS;
                break;
            case 'T':
                rest = "RACE";
                m    = HTTP_TRACE;
                break;
            case 'P':
                c = *p++;
                len--;
                switch (toupper((int)c))
                {
                    case 'U':
                        rest = "T";
                        m    = HTTP_PUT;
                        break;
                    case 'O':
                        rest = "ST";
                        m    = HTTP_POST;
                        break;
                    case 'A':
                        rest = "TCH";
                        m    = HTTP_PATCH;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        /* match rest, return m if equal. */
        if (rest)
        {
            m = strncasecmp(rest, p, len) ? HTTP_NO_METHOD : m;
        }
    }
    return m;
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define JSON_UNICODE_ESC_LEN 6
/**
 * @brief write a json string representation, including quotes to fd.
 *
 * Writes a quoted string, including quotes.
 * control characters are encoded as unicode escape sequences.
 * writes "null" if src is NULL.
 *
 * @param src source string
 * @param fd  output FILE
 */
void fputs_json_string(const char *restrict src, FILE *fd)
{
    if (*src)
    {
        char c;
        fputc('"', fd);
        while (*src && src < src + 1)
        {
            c = *src++;
            /* get current character, then increment src */
            switch (c)
            {
                /* there is always room for two characters, for the final two bytes */
                /* we detect later if it was used up prematurely */
                case '\\':
                case '"':
                    fputc('\\', fd);
                    fputc(c, fd);
                    break;
                case '\b':
                    fputs("\\b", fd);
                    break;
                case '\t':
                    fputs("\\t", fd);
                    break;
                case '\n':
                    fputs("\\n", fd);
                    break;
                case '\f':
                    fputs("\\f", fd);
                    break;
                case '\r':
                    fputs("\\r", fd);
                    break;
                default:
                    if (c >= ' ') /*no controll character */
                    {
                        fputc(c, fd);
                        continue;
                    }
                    else
                    { /* write only if we have space for a unicode escape */
                        fprintf(fd, "\\u%04x", (unsigned int)c);
                        continue;
                    }
            }
        }
        fputc('"', fd);
    }
    else
    {
        fputs("null", fd);
    }
};

/**
 * @brief write a json string representation of a string to a buffer
 *
 * Writes a quoted string, including quotes.
 * control characters are encoded as unicode escape sequences.
 * writes "null" if src is NULL.
 *
 * @param dst destination buffer
 * @param size size of destination buffer
 * @param src string to quote
 *
 * @return size needed, if it's greater than `size` not all
 * characters could be written.
 */
size_t json_quote_string(char *restrict dst, size_t size, const char *restrict src)
{
    char *p  = dst;
    char *pe = dst + size;
    if (*src)
    {
        if (p < pe)
        {
            *p = '"';
        }
        p++;
        while (*src && p < pe)
        {
            /* get current character, then increment src */
            char c = *src++;
            switch (c)
            {
                /* there is always room for two characters, for the final two bytes */
                /* we detect later if it was used up prematurely */
                case '\\':
                case '"':
                    *p++ = '\\';
                    if (p < pe)
                    {
                        *p = c;
                    }
                    p++;
                    break;
                case '\b':
                    *p++ = '\\';
                    if (p < pe)
                    {
                        *p = 'b';
                    }
                    p++;
                    break;
                case '\t':
                    *p++ = '\\';
                    if (p < pe)
                    {
                        *p = 't';
                    }
                    p++;
                    break;
                case '\n':
                    *p++ = '\\';
                    if (p < pe)
                    {
                        *p = 'n';
                    }
                    p++;
                    break;
                case '\f':
                    *p++ = '\\';
                    if (p < pe)
                    {
                        *p = 'f';
                    }
                    p++;
                    break;
                case '\r':
                    *p++ = '\\';
                    if (p < pe)
                    {
                        *p = 'r';
                    }
                    p++;
                    break;
                default:
                    if (c >= ' ') /*no controll character */
                    {
                        *p++ = c;
                        break;
                    }
                    if (p + 6 < pe)
                    {
                        snprintf(p, 6, "\\u%04x", (unsigned int)c);
                    }
                    p += 6;
                    break;
            }
        }
        if (p < pe)
        {
            *p = '"';
        }
        p++;
    }
    else
    {
        dst = strncpy(p, "null", size);
        p += 4;
    }
    return p - dst;
};

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief url decode a string
 *
 * ensure out points to a buffer that is at least as long as the input string
 * 
 * @param out destination buffer
 * @param in  source buffer
 */
void url_decode(char *out, const char *in)
{
    unsigned char c;
    for (; *in != '\0'; out++)
    {
        /* coverity[overflow] would hit terminating null byte, invalidating one of the isxdigit tests */
        c = (unsigned char)*in++;
        if (c == '%' && 0 != isxdigit((unsigned char)in[0]) && 0 != isxdigit((unsigned char)in[1]))
        {
            /* coverity[overflow_sink] end of string would not pass the condition above */
            unsigned char h = toupper((unsigned char)*in++);
            /* coverity[overflow_sink] end of string would not pass the condition above */
            unsigned char l = toupper((unsigned char)*in++);
            h -= ((h >= 'A') ? 'A' + 10 : '0');
            l -= ((l >= 'A') ? 'A' + 10 : '0');
            c = (h << 4) | l;
        }
        else if (c == '+')
        {
            c = ' ';
        }
        *out = c;
    }
    *out = '\0';
}


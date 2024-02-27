/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CIT_HTTPD_PLUG_URLDECODE_H
#define CIT_HTTPD_PLUG_URLDECODE_H

/**
 * @brief url decode a string
 *
 * ensure out points to a buffer that is at least as long as the input string
 *
 * @param out destination buffer
 * @param in  source buffer
 */
void url_decode(char *out, const char *in);

/**
 * @brief url decode a string in place
 *
 * @param inout buffer
 */
static inline void url_decode_inplace(char *buffer)
{
    url_decode(buffer, buffer);
}

#endif

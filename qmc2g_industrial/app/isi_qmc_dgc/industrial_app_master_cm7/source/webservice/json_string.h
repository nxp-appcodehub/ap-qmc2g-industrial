/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef JSON_STRING_H_
#define JSON_STRING_H_
#include <stddef.h>
#include <stdio.h>

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
void fputs_json_string(const char *src, FILE *fd);

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
size_t json_quote_string(char *restrict dst, size_t size, const char *restrict src);

#endif

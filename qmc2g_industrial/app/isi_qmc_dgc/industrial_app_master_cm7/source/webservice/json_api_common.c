/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "api_qmc_common.h"
#include "json_api_common.h"

/**
 * @brief adapt strntoull to work with strings not ending in a null byte
 *
 * check end pointer to disambiguate 0 return values
 *
 * @param str pointer to string to parse
 * @param len length of string to consider
 * @param end optional end pointer (see strtoll)
 * @param base numeric base
 *
 * @return parsed value or 0
 */
unsigned long long strntoull(const char *str, size_t len, const char **end, int base)
{
    unsigned long long rc      = 0;
    const unsigned char *start = (const unsigned char *)str;

    if (!len || len > sizeof(unsigned long long) * 8 + 1)
    {
        goto fail;
    }
    for (; start && len && isspace((int)(unsigned char)*start) && start < start+1; start++)
    {
        len--;
    }
    if (!len)
    {
        goto fail;
    }
    else
    {
        char buf[sizeof(unsigned long long) * 8 + 2];
        memcpy(buf, start, len);
        buf[len] = '\0';
        rc       = strtoull(buf, (char**)end, base);
        if (end)
        {
            *end = (char *)start + (*end - buf);
        }
    }
    return rc;
fail:
    if (end)
    {
        *end = (char *)str;
    }
    return 0;
}

/**
 * @brief adapt strntof to work with strings not ending in a null byte
 *
 * check end pointer to disambiguate 0 return values
 *
 * @param str pointer to string to parse
 * @param len length of string to consider
 * @param end optional end pointer (see strtoll)
 * @param base numeric base
 *
 * @return parsed value or 0
 */
float strntof(const char *str, size_t len, const char **end)
{
    float rc;
    const unsigned char *start = (const unsigned char *)str;
    char buf[32];

    if (!len || len > (sizeof(buf) - 1))
    {
        goto fail;
    }
    for (; start && len && isspace((int)(unsigned char)*start) && start < start+1; start++)
    {
        len--;
    }
    if (!len)
    {
        goto fail;
    }
    else
    {
        memcpy(buf, start, len);
        buf[len] = '\0';
        rc       = strtof(buf, (char**)end);
        if (end)
        {
            *end = (const char *)start + (*end - buf);
        }
    }
    return rc;
fail:
    if (end)
    {
        *end = (const char *)str;
    }
    return NAN;
}

/**
 * @brief convert an uint64_t to base 10 ascii
 * this is not supported by newlib nano...
 *
 * @param val value to convert
 *
 * @return pointer to temporary buffer with the string representation.
 *
 */
char *uint64toa(uint64_t val)
{
    static char s_number[20 + 1];
    char *p = s_number + sizeof(s_number);
    *(--p)  = '\0';
    do
    {
        const uint32_t digit = val % 10;
        const char c         = '0' + digit;
        *(--p)               = c;
        val                  = val / 10;
    } while (val);
    return p;
}

/**
 * @brief create a hexdump for a uint64_t value
 *
 * @param val input value
 *
 * @return pointer to temporary buffer containing the result
 */

char *uint64tohex(uint64_t val)
{
    static char s_number[16 + 1];
    char *p = s_number + sizeof(s_number)-1;
    *p  = '\0';
    do
    {
        const uint32_t digit = val % 16;
        const char c         = ((digit < 10) ? '0' : 'A' - 10) + digit;
        *(--p)               = c;
        val                  = val / 16;
    } while (val && p >= s_number);
    return p;
}

/**
 * @brief get an description for an qmc_status_t value
 *
 * @param status qmc_status_t value
 *
 * @return error description
 */
const char *webservice_error_string(qmc_status_t status)
{
    const char *str = "invalid error code returned";
    switch (status)
    {
        case kStatus_QMC_Ok:
            str = "success";
            break;
        case kStatus_QMC_Err:
            str = "failed";
            break;
        case kStatus_QMC_ErrRange:
            str = "out of range";
            break;
        case kStatus_QMC_ErrArgInvalid:
            str = "invalid argument";
            break;
        case kStatus_QMC_Timeout:
            str = "timeout";
            break;
        case kStatus_QMC_ErrBusy:
            str = "busy";
            break;
        case kStatus_QMC_ErrMem:
            str = "out of memory";
            break;
        case kStatus_QMC_ErrSync:
            str = "synchronization error";
            break;
        case kStatus_QMC_ErrNoMsg:
            str = "no message";
            break;
        case kStatus_QMC_ErrInterrupted:
            str = "interrupted";
            break;
        case kStatus_QMC_ErrNoBufs:
            str = "no buffer space";
            break;
        case kStatus_QMC_ErrInternal:
            str = "internal error";
            break;
        case kStatus_QMC_ErrSignatureInvalid:
            str = "invalid signature";
            break;
    }
    return str;
}

/**
 * @brief print error json if qmc_status_t is not kStatus_QMC_Ok
 *
 * @param status    qmc_status_t value to check
 * @param response  FILE handle to print the json message to
 *
 * @return true if status==kStatus_QMC_Ok
 */
bool webservice_check_error(qmc_status_t status, FILE *response)
{
    if (status != kStatus_QMC_Ok)
    {
        fprintf(response, "{\"error\":\"%s\"}\n", webservice_error_string(status));
    }
    return status == kStatus_QMC_Ok;
}

/**
 * @brief print a json representation of an qmc_timestamp_t
 *
 * "12345.123"
 *
 * @param ts the timestamp
 * @param response FILE pointer
 */
void print_json_timestamp(qmc_timestamp_t *ts, FILE *response)
{
    char *secs = uint64toa(ts->seconds);
    fprintf(response, "\"%s%03" PRIu16 "\"", secs, ts->milliseconds);
}

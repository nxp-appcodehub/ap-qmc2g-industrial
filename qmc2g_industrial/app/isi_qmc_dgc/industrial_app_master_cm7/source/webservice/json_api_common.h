/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "api_qmc_common.h"
#include "api_motorcontrol.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
 * @brief convert an uint64_t to base 10 ascii
 * this is not supported by newlib nano...
 *
 * @param val value to convert
 *
 * @return pointer to temporary buffer with the string representation.
 *
 */
char *uint64toa(uint64_t val);

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
unsigned long long strntoull(const char *str, size_t len, const char **end, int base);

/**
 * @brief adapt strntof to work with strings not ending in a null byte
 *
 * check end pointer to disambiguate 0 return values
 *
 * @param str pointer to string to parse
 * @param len length of string to consider
 * @param end optional end pointer (see strtoll)
 *
 * @return parsed value or 0
 */
float strntof(const char *str, size_t len, const char **end);

/**
 * @brief create a hexdump for a uint64_t value
 *
 * @param val input value
 *
 * @return pointer to temporary buffer containing the result
 */
char *uint64tohex(uint64_t val);

/**
 * @brief get an description for an qmc_status_t value
 *
 * @param status qmc_status_t value
 *
 * @return error description
 */
const char *webservice_error_string(qmc_status_t status);

/**
 * @brief print error json if qmc_status_t is not kStatus_QMC_Ok
 *
 * @param status    qmc_status_t value to check
 * @param response  FILE handle to print the json message to
 *
 * @return true if status==kStatus_QMC_Ok
 */
bool webservice_check_error(qmc_status_t status, FILE *response);

/**
 * @brief print a json representation of an qmc_timestamp_t
 *
 * "12345.123"
 *
 * @param ts the timestamp
 * @param response FILE pointer
 */
void print_json_timestamp(qmc_timestamp_t *ts, FILE *response);

/**
 * @brief return a string with the motors number
 *
 * @param motor_id enum value
 *
 * @return pointer to motor string
 */
const char *mc_motor_id_to_string(mc_motor_id_t motor_id);

/**
 * @brief insert a literal string followed by its length
 *
 * @param s string literal
 *
 * @return  string literal, length of string literal
 */
#define SIZED(s) "" s, (sizeof(s) - 1)

/**
 * some macros to convert ascii hex digites (isxdigit) to a value 0-16
 * convert a valid UPPERCASE hex char to a decimal value
 */
#define PRIV_UPPER_HEX_VALUE(c) ((c) < 'A' ? (c) - '0' : (c) - 'A' + 0x0Au)

/**
 * convert hex chars to upper case, could use 'a' but uses 'A' to limit the
 * number decisions paths in the expression. applying the mask to uppercase chars is ok
 * just not for the numbers, helps the compiler optimize this to less code.
 */
#define PRIV_HEX_UPPER(c) ((c) < 'A' ? (c) : (c & 0xDFu))

/**
 * convert a hex digit no matter the case
 * #define PRIV_HEX_VALUE(c) (PRIV_UPPER_HEX_VALUE(HEX_UPPER(c)))
 * help the compiler with the expected return values a bit
 */
#define HEX_VALUE(c) ((unsigned char)PRIV_UPPER_HEX_VALUE(PRIV_HEX_UPPER((unsigned char)c)) & (unsigned char)0x0Fu)

/* concatenat two arguments */
#define PRIV_ENUM_CAT(a, ...) a##__VA_ARGS__

/* concatenate three arguments */
#define PRIV_ENUM_CAT3(a, b, ...) a##b##__VA_ARGS__

/* generate a case statement for an enum_to_string function */
#define PRIV_GEN_ENUM_CASE(val, str, ...) \
    case val:                             \
        return str;

/* generate a lookup table entry for an enum_from_string function */
#define PRIV_GEN_ENUM_LUT(value, string, ...) {.str = string, .len = sizeof(string) - 1, .val = value},

/* generate a enum_to_string function from an ENUM macro  the _Pragmas ensure missing enum values generate an error*/
// const char *name(type enum);
#define GENERATE_ENUM_TO_STRING(name, type, ENUM, ...)                                                           \
    _Pragma("GCC diagnostic push")                                                                               \
    _Pragma("GCC diagnostic error \"-Wswitch\"") __VA_ARGS__ const char *PRIV_ENUM_CAT(name, _to_string)(type v) \
    {                                                                                                            \
        switch (v)                                                                                               \
        {                                                                                                        \
            ENUM(PRIV_GEN_ENUM_CASE)                                                                             \
        }                                                                                                        \
        return ""; /* implicit default: should not be reached */                                                 \
    }                                                                                                            \
    _Pragma("GCC diagnostic pop") //

/* type of an enum lookup table entry */
struct enum_lut
{
    const size_t len;   // text length
    const char *str;    // text representation
    const uint32_t val; // enum value
};

/* generate an enum_from_string function */
// bool name(type *val, const char* str, size_t len)
#define GENERATE_ENUM_FROM_STRING(name, type, ENUM, ...)                                        \
    __VA_ARGS__ bool PRIV_ENUM_CAT(name, _from_string)(type * val, const char *str, size_t len) \
    {                                                                                           \
        static const struct enum_lut PRIV_ENUM_CAT(                                             \
            name, _lut)[] = {ENUM(PRIV_GEN_ENUM_LUT){.str = NULL, .len = 0, .val = 0}};         \
        const struct enum_lut *p;                                                               \
        for (p = PRIV_ENUM_CAT(name, _lut); p->str; p++)                                        \
        {                                                                                       \
            if (p->len != len)                                                                  \
                continue;                                                                       \
            if (strncmp(str, p->str, len) == 0)                                                 \
            {                                                                                   \
                *val = p->val;                                                                  \
                return true;                                                                    \
            }                                                                                   \
        }                                                                                       \
        return false;                                                                           \
    }                                                                                           \
    //

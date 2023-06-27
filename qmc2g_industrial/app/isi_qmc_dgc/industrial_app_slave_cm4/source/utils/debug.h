/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file debug.h
 * @brief Helper macros for debugging.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdbool.h>
#include "debug_log_levels.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DEBUG_LOG_ERROR_TAG   "[ERROR] "
#define DEBUG_LOG_WARNING_TAG "[WARNING] "
#define DEBUG_LOG_INFO_TAG    "[INFO] "

#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_ERROR
#endif

#ifndef DEBUG_LOG_PRINT_FUNC
#include <stdio.h>
#define DEBUG_LOG_PRINT_FUNC printf
#endif

#if defined(DEBUG) && (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_ERROR)
#define DEBUG_LOG_E(fmt, args...) DEBUG_LOG_PRINT_FUNC(DEBUG_LOG_ERROR_TAG fmt, ##args)
#else
#define DEBUG_LOG_E(fmt, args...) \
    do                            \
    {                             \
    } while (0)
#endif

#if defined(DEBUG) && (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_WARNING)
#define DEBUG_LOG_W(fmt, args...) DEBUG_LOG_PRINT_FUNC(DEBUG_LOG_WARNING_TAG fmt, ##args)
#else
#define DEBUG_LOG_W(fmt, args...) \
    do                            \
    {                             \
    } while (0)
#endif

#if defined(DEBUG) && (DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_INFO)
#define DEBUG_LOG_I(fmt, args...) DEBUG_LOG_PRINT_FUNC(DEBUG_LOG_INFO_TAG fmt, ##args)
#else
#define DEBUG_LOG_I(fmt, args...) \
    do                            \
    {                             \
    } while (0)
#endif

#define DEBUG_M4_TAG "[M4] "
#define DEBUG_M7_TAG "[M7] "

#define DEBUG_ARM_CM_DEMCR      (*(volatile uint32_t *)0xE000EDFCU)
#define DEBUG_ARM_CM_DWT_CTRL   (*(volatile uint32_t *)0xE0001000U)
#define DEBUG_ARM_CM_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004U)

#define DEBUG_ARM_ENABLE_DWT         \
    DEBUG_ARM_CM_DEMCR |= 1U << 24U; \
    DEBUG_ARM_CM_DWT_CYCCNT = 0U;    \
    DEBUG_ARM_CM_DWT_CTRL |= 1U << 0U

#define DEBUG_PROFILE_BLOCK_START(n)       \
    do                                     \
    {                                      \
        size_t nBuff       = n;            \
        uint64_t sumIntern = 0U;           \
        uint32_t maxIntern = 0U;           \
        uint32_t minIntern = (uint32_t)-1; \
        for (size_t r = 0U; r < n; r++)    \
        {                                  \
            uint32_t start = DEBUG_ARM_CM_DWT_CYCCNT

#define DEBUG_PROFILE_BLOCK_END(avg, min, max)           \
    uint32_t stop  = DEBUG_ARM_CM_DWT_CYCCNT;            \
    uint32_t delta = stop - start;                       \
    sumIntern += delta;                                  \
    maxIntern = (delta > maxIntern) ? delta : maxIntern; \
    minIntern = (delta < minIntern) ? delta : minIntern; \
    }                                                    \
    avg = sumIntern / nBuff;                             \
    max = maxIntern;                                     \
    min = minIntern;                                     \
    }                                                    \
    while (0)

#endif /* _DEBUG_H_ */

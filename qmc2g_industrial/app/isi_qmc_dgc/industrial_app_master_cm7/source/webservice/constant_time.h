/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef WEBSERVICE_CONSTANT_TIME_H_
#define WEBSERVICE_CONSTANT_TIME_H_
#include <stddef.h>
#include <stdint.h>

/**
 * @brief constant time rangecheck
 *
 * @param min range lower limmit
 * @param max range upper limit
 * @param value value to compare with the limits
 *
 * @return returns 0xff if in range, 0x00 otherwise
 */

inline uint8_t uint8_ct_rangecheck(uint8_t min, uint8_t max, uint8_t value)
{
    /* min_mask = min <= value ? 0 : 0xff */
    unsigned min_mask = ((unsigned)value - min) >> 8;
    /* max_mask = value <= max ? 0 : 0xff */
    unsigned max_mask = ((unsigned)max - value) >> 8;
    /* combine masks, negate and mask the result region */
    return (~(min_mask | max_mask) & 0xff);
}

#endif

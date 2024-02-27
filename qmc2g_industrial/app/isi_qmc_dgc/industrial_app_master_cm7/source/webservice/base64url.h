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
#include <stddef.h>
#include <stdint.h>

/*!
 * @brief macro to calculate base64 encoded length
 * @param bytes payload byte count
 * @return bytes in base64 encoding
 */
#define BASE64_ENCODED_LENGTH(bytes) (4 * (((bytes) / 3) + ((bytes) % 3 != 0)))

/*!
 * @brief macro to calculate base64 decoded length
 * @param bytes encoded byte count
 * @returns maximum bytes in transport encoding
 */
#define BASE64_DECODED_LENGTH(bytes) (3 * (((bytes) / 4) + ((bytes) % 4 != 0)))

/**
 * @brief  constant time rangecheck function
 *
 * @param min lower bound
 * @param max upper bound
 * @param value value to check
 *
 * @return 0xFF if in range 0x00 otherwise
 */
uint8_t uint8_ct_rangecheck(uint8_t min, uint8_t max, uint8_t value);

/**
 * @brief base64 encoder using url escaping charactern
 *
 * @param dest   target buffer - optional
 * @param size   target buffer size
 * @param nchars pointer to size_t value for the encoded length - written even if dest is NULL
 * @param src    source bytes pointer
 * @param len    length of bytes to encode
 *
 * @return  0 on success -1 otherise
 */
int base64url_encode(
    unsigned char *restrict dest, size_t size, size_t *nchars, const uint8_t *restrict src, size_t len);

/**
 * @brief base64 decoder using url escaping characters
 *
 * @param dest   target buffer
 * @param size   target buffer size
 * @param nchars pointer to size_t decoded bytes
 * @param src    source of base64url string
 * @param len    length of base64url string
 *
 * @return  0 on success -1 otherise
 */
int base64url_decode(
    uint8_t *restrict dest, size_t size, size_t *nchars, const unsigned char *restrict src, size_t len);

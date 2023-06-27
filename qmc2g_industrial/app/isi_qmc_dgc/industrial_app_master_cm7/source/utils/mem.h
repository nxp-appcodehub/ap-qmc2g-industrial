/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file mem.h
 * @brief Helper inline functions related to memory access.
 *
 * Includes memset and memcpy versions which support volatile pointers.
 * Further, it provides functions to unpack a uint32_t from a byte array 
 * (for both little and big-endian).
 */

#ifndef _MEM_H_
#define _MEM_H_

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief memset version that accepts volatile pointers.
 *
 * @param pDest Pointer to the start of the destination memory region.
 * @param val The value to which the destination memory region should be set.
 * @param len The length of the destination memory region.
 * @return void* The pDest pointer.
 * 
 */
static inline volatile void *vmemset(volatile void *const pDest, uint8_t val, size_t len)
{
    assert(NULL != pDest);

    volatile uint8_t *const pDestByte = pDest;
    for (size_t off = 0U; off < len; off++)
    {
        pDestByte[off] = val;
    }

    return pDest;
}

/*!
 * @brief memcpy version that accepts volatile pointers.
 *
 * @param pDest Pointer to the start of the destination memory region.
 * @param pSrc Pointer to the start of the source memory region.
 * @param len The amount of bytes that should be copied from pSrc to pDest.
 * @return void* The pDest pointer.
 * 
 */
static inline volatile void *vmemcpy(volatile void *const pDest, volatile const void *const pSrc, size_t len)
{
    assert((NULL != pDest) && (NULL != pSrc));

    volatile uint8_t *const pDestByte      = pDest;
    volatile const uint8_t *const pSrcByte = pSrc;
    for (size_t off = 0U; off < len; off++)
    {
        pDestByte[off] = pSrcByte[off];
    }

    return pDest;
}

/*!
 * @brief Unpacks an unsigned 32bit integer using little-endian byte order.
 *
 * @param pSrc Source pointer from which the integer is unpacked.
 * @return uint32_t The unpacked unsigned 32bit integer.
 *
 */
static inline uint32_t unpackU32LittleEndian(const uint8_t *const pSrc)
{
    assert(NULL != pSrc);

    return pSrc[0U] | (pSrc[1U] << 8U) | (pSrc[2U] << 16U) | (pSrc[3U] << 24U);
}

/*!
 * @brief Unpacks an unsigned 32bit integer using big-endian byte order.
 *
 * @param pSrc Source pointer from which the integer is unpacked.
 * @return uint32_t The unpacked unsigned 32bit integer.
 *
 */
static inline uint32_t unpackU32BigEndian(const uint8_t *const pSrc)
{
     assert(NULL != pSrc);

    return (pSrc[0U] << 24U) | (pSrc[1U] << 16U) | (pSrc[2U] << 8U) |  pSrc[3U];
}

#endif /* _MEM_H_ */

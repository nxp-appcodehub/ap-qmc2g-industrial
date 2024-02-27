/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include "genavb/genavb.h"

#if !(defined(__LITTLE_ENDIAN__) || defined(__BIG_ENDIAN__))
#error
#endif


#define load_shift_store(dst, src) \
    val = val2 >> shift;           \
    val2 = *(uint32_t *)(src);     \
    *(uint32_t *)(dst) = val | (val2 << shift2)


/*	memcpy_aligned32
 *
 *  place of execution: ITCM
 *
 *  description: TSN implementation of memcpy_aligned32
 *
 *  params: *dst - destination address
 *          *src - source address
 *          len - lenght of the copied data
 *
 */
static void memcpy_aligned32(void *dst, const void *src, unsigned int len)
{
	assert(len);

	unsigned int len32;

    if (((unsigned long)src & 0x4) && ((unsigned long)dst & 0x4)) {
        __asm("ldr r5, [%1], #4\n\t"
              "str r5, [%0], #4\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
        len -= 4;
    }

    for (len32 = len >> 3; len32 > 0; len32--) {
        __asm("ldm %1!, {r5-r6}\n\t"
              "stm %0!, {r5-r6}\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "r6", "memory");
    }

    if (len & 0x4) {
        __asm("ldr r5, [%1], #4\n\t"
              "str r5, [%0], #4\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
    }

    if (len & 0x2) {
        __asm("ldrh r5, [%1], #2\n\t"
              "strh r5, [%0], #2\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
    }

    if (len & 0x1)
        *(uint8_t *)dst = *(uint8_t *)src;
}


/*	memcpy_aligned
 *
 *  place of execution: ITCM
 *
 *  description: TSN implementation of memcpy_aligned
 *
 *  params: *dst - destination address
 *          *src - source address
 *          len - lenght of the copied data
 *
 */
static void memcpy_aligned(void *dst, const void *src, unsigned int len)
{
	assert(len);

    if ((unsigned long)src & 0x1) {
        __asm("ldrb r5, [%1], #1\n\t"
              "strb r5, [%0], #1\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
        len--;
    }

    if ((unsigned long)src & 0x2) {
        __asm("ldrh r5, [%1], #2\n\t"
              "strh r5, [%0], #2\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
        len -= 2;
    }

    memcpy_aligned32(dst, src, len);
}


/*	memcpy_unaligned
 *
 *  place of execution: ITCM
 *
 *  description: TSN implementation of memcpy_unaligned
 *
 *  params: *dst - destination address
 *          *src - source address
 *          len - lenght of the copied data
 *
 */
static void memcpy_unaligned(void *dst, const void *src, unsigned int len)
{
    uint32_t val, val2, tmp, shift, shift2;

    assert(len);

    /* 32bit align destination */
    switch ((unsigned long)dst) {
    case 1:
        __asm("ldrb r5, [%1], #1\n\t"
              "strb r5, [%0], #1\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
        len--;
        break;
    case 2:
        __asm("ldrh r5, [%1], #2\n\t"
              "strh r5, [%0], #2\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
        len -= 2;
        break;
    case 3:
        __asm("ldrb r5, [%1], #1\n\t"
              "strb r5, [%0], #1\n\t"
              "ldrh r5, [%1], #2\n\t"
              "strh r5, [%0], #2\n\t"
              : "=r"(dst), "=r"(src)
              : "0"(dst), "1"(src)
              : "r5", "memory");
        len -= 3;
        break;
    }

    /* 32bit align source */
    switch ((unsigned long)src & 0x3) {
    case 2:
#ifdef __LITTLE_ENDIAN__
        val2 = (*(uint16_t *)src) << 16;
#else
        val2 = *(uint16_t *)src;
#endif
        src += 2;
        len -= 2;

        shift = 16;

        break;

    default:
#ifdef __LITTLE_ENDIAN__
        val2 = (*(uint8_t *)src) << 24;
#else
        val2 = *(uint8_t *)src;
#endif
        src++;
        len--;

        shift = 24;

        break;

    case 1:
#ifdef __LITTLE_ENDIAN__
        val2 = (*(uint8_t *)src) << 8;
        src++;
        val2 |= (*(uint16_t *)src) << 16;
#else
        val2 = (*(uint8_t *)src) << 16;
        src++;
        val2 |= *(uint16_t *)src;
#endif
        len -= 3;
        src += 2;

        shift = 8;

        break;
    }

    shift2 = 32 - shift;

    /* 32bit read, shift, write */
    for (tmp = len >> 4; tmp > 0; tmp--, src += 16, dst += 16) {
        load_shift_store(dst, src);
        load_shift_store(dst + 4, src + 4);
        load_shift_store(dst + 8, src + 8);
        load_shift_store(dst + 12, src + 12);
    }

    if (len & 0x8) {
        load_shift_store(dst, src);
        load_shift_store(dst + 4, src + 4);

        src += 8;
        dst += 8;
    }

    if (len & 0x4) {
        load_shift_store(dst, src);

        src += 4;
        dst += 4;
    }

    /* Shift, store last bytes */
#ifdef __LITTLE_ENDIAN__
    val = val2 >> shift;
#else
    val = val2 << shift;
#endif

    /* remaining bytes to read */
    len &= 0x3;

    /* read last bytes */
    switch (len) {
    default:
        val2 = 0;
        break;

    case 1:
#ifdef __LITTLE_ENDIAN__
        val2 = *(uint8_t *)src;
#else
        val2 = (*(uint8_t *)src) << 24;
#endif
        break;

    case 2:
#ifdef __LITTLE_ENDIAN__
        val2 = *(uint16_t *)src;
#else
        val2 = (*(uint16_t *)src) << 16;
#endif
        break;

    case 3:
#ifdef __LITTLE_ENDIAN__
        val2 = (*(uint16_t *)src) | ((*(uint8_t *)(src + 2)) << 16);
#else
        val2 = ((*(uint16_t *)src) << 16) | ((*(uint8_t *)(src + 2)) << 8);
#endif
        break;
    }

    /* remaining bytes to write, <= 6 */
    len += shift2 >> 3;

#ifdef __LITTLE_ENDIAN__
    val |= (val2 << shift2);
#else
    val |= (val2 >> shift2);
#endif

    if (len & 0x4) {
        *(uint32_t *)dst = val;
#ifdef __LITTLE_ENDIAN__
        val = val2 >> shift;
#else
        val = val2 << shift;
#endif
        dst += 4;
    }

    if (len & 0x2) {
#ifdef __LITTLE_ENDIAN__
        *(uint16_t *)dst = val;
        val >>= 16;
#else
        *(uint16_t *)dst = val >> 16;
        val <<= 16;
#endif
        dst += 2;
    }

    if (len & 0x1)
#ifdef __LITTLE_ENDIAN__
        *(uint8_t *)dst = val;
#else
        *(uint8_t *)dst = val >> 24;
#endif
}


/*	memcpy
 *
 *  place of execution: ITCM
 *
 *  description: TSN implementation of mempcy
 *
 *  params: *dst - destination address
 *          *src - source address
 *          len - lenght of the copied data
 *
 */
void *memcpy(void *dst, const void *src, size_t len)
{
    switch (len) {
    case 0:
        break;
    case 1:
        *(uint8_t *)dst = *(uint8_t *)src;
        break;
    case 2:
        *(uint16_t *)dst = *(uint16_t *)src;
        break;
    case 3:
        *(uint16_t *)dst = *(uint16_t *)src;
        *(uint8_t *)(dst + 2) = *(uint8_t *)(src + 2);
        break;
    case 4:
        *(uint32_t *)dst = *(uint32_t *)src;
        break;
    case 5:
        *(uint32_t *)dst = *(uint32_t *)src;
        *(uint8_t *)(dst + 4) = *(uint8_t *)(src + 4);
        break;
    case 6:
        *(uint32_t *)dst = *(uint32_t *)src;
        *(uint16_t *)(dst + 4) = *(uint16_t *)(src + 4);
        break;
    case 7:
        *(uint32_t *)dst = *(uint32_t *)src;
        *(uint16_t *)(dst + 4) = *(uint16_t *)(src + 4);
        *(uint8_t *)(dst + 6) = *(uint8_t *)(src + 6);
        break;
    case 8:
        *(uint32_t *)dst = *(uint32_t *)src;
        *(uint32_t *)(dst + 4) = *(uint32_t *)(src + 4);
        break;
    default:
        if (((unsigned long)dst & 0x3) == ((unsigned long)src & 0x3))
            memcpy_aligned(dst, src, len);
        else
            memcpy_unaligned(dst, src, len);
        break;
    }

    return dst;
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "base64url.h"
#include "constant_time.h"
/*
 * Encode a buffer into base64url format
 * *dest destination buffer
 * size sizeof(*dest)
 * *nchars count of encoded characters, or needed buffer length
 * *src byte buffer to encode
 * len number of bytes from src
 *
 * will pad to a multiple of four bytes if size allows, padding is done mith '.'
 * and is not reported in *nchars, as it's optional for this encoding.
 *
 * input buffer operations are done with constant time behavior to mask invalid data positions
 */

/**
 * @brief constant time base64 url char encoder map
 *
 * @param value to encode
 *
 * @return character for encoding the value
 */

static inline char base64url_char_for_value(register const uint8_t value)
{
    /* note: only one of the rangechecks will ever be non-zero */
    /* the expression right of the & will therefore be assigned to digit */
    uint8_t digit = 0;
    digit |= uint8_ct_rangecheck(0, 25, value) & ('A' + value);
    digit |= uint8_ct_rangecheck(26, 51, value) & ('a' + value - 26);
    digit |= uint8_ct_rangecheck(52, 61, value) & ('0' + value - 52);
    digit |= uint8_ct_rangecheck(62, 62, value) & '-';
    digit |= uint8_ct_rangecheck(63, 63, value) & '_';
    return (digit);
}

/**
 * @brief constant time base64url char decoder map
 *
 * @param c character to decode
 *
 * note: only one of the rangechecks will ever be non-zero
 * the expression right of the & will therefore be assigned to the result.
 * the result is one more than the actual value, returning zero if there
 * is no matching rangecheck.
 *
 * subtracting one returns the correct value and -1 in case of an invalid char.
 *
 * @return returns the character's table position / value or -1 if invalid
 */

static inline int8_t base64url_value_for_char(register const char c)
{
    unsigned result = 0;
    /*                             range         c_offset + value +1*/
    result |= uint8_ct_rangecheck('A', 'Z', c) & (c - 'A' + 0 + 1);
    result |= uint8_ct_rangecheck('a', 'z', c) & (c - 'a' + 26 + 1);
    result |= uint8_ct_rangecheck('0', '9', c) & (c - '0' + 52 + 1);
    result |= uint8_ct_rangecheck('-', '-', c) & (c - '-' + 62 + 1);
    result |= uint8_ct_rangecheck('_', '_', c) & (c - '_' + 63 + 1);
    return ((int8_t)result) - 1; /* -1 if no match is found*/
}
#if !defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/**
 * @brief base64url_encoder
 *
 * @param dest   destination buffer
 * @param size   destination buffer size
 * @param nchars output character counter
 * @param src    source string
 * @param len    length of the source string
 *
 * @return  0 on success, -1 otherwise
 */
int base64url_encode(unsigned char *restrict dest, size_t size, size_t *nchars, const uint8_t *restrict src, size_t len)
{
    size_t i, n;
    unsigned char *p;
    int result = -1;

    if (len == 0)
    {
        *nchars = 0;
        *dest   = '\0';
    }

    n = len / 3 + (len % 3 != 0);
    {
        if (n * 4 < n)
        {
            return -1;
        }
        n       = n * 4;
        *nchars = n + 1;
        if ((size >= n + 1) && (NULL != dest))
        {
            n = (len / 3) * 3;
            /* iterate full triplets*/
            for (i = 0, p = dest; i < n; i += 3)
            {
                unsigned int b1, b2, b3;
                b1 = *src++;
                b2 = *src++;
                b3 = *src++;

                *p++ = base64url_char_for_value((uint8_t)(b1 >> 2) & 0x3F);
                *p++ = base64url_char_for_value((uint8_t)(((b1 & 3) << 4) + (b2 >> 4)) & 0x3F);
                *p++ = base64url_char_for_value((uint8_t)(((b2 & 15) << 2) + (b3 >> 6)) & 0x3F);
                *p++ = base64url_char_for_value((uint8_t)b3 & 0x3F);
            }
            /* process 1-2 remaining bytes if any*/
            if (i < len)
            {
                unsigned int b1, b2;
                b1 = *src++;
                b2 = ((i + 1) < len) ? *src++ : 0u;

                *p++ = base64url_char_for_value((uint8_t)(b1 >> 2) & 0x3F);
                *p++ = base64url_char_for_value((uint8_t)(((b1 & 3) << 4) + (b2 >> 4)) & 0x3F);

                if ((i + 1) < len)
                {
                    *p++ = base64url_char_for_value((uint8_t)((b2 & 15) << 2) & 0x3F);
                }
                /* add padding if there is space, will not be reported in nchars*/
                for (i = 0; i < ((size_t)(p - dest) % 4) && size < ((size_t)(p - dest) + i); i++)
                {
                    p[i] = '.';
                }
            }
            result  = 0;
            *nchars = p - dest;
        }
    }
    return result;
}

/*
 *  Decode an base64url encoded char buffer
 *
 *  *dest destination buffer
 *  size  sizeof(*dest)
 *  *nchars number of output bytes in dest
 *  *src character buffer to decode
 *  len number of characters to decode
 *
 *  - will ignore 1-3 bytes of padding with '.' or '='
 *  - will not ignore any whtespace or additional padding (returns -1)

 *  src buffer operations ore selected for constant time behavior to
 *  mask invalid data positions.
 */
int base64url_decode(uint8_t *restrict dest, size_t size, size_t *nchars, const unsigned char *restrict src, size_t len)
{
    size_t i;
    size_t valid_count = 0;
    int result         = -1;
    /* constant time validity check*/
    /* walk the buffer and count valid caracters*/
    for (i = 0; i < len; i++)
    {
        int in_range = 0;
        in_range |= (uint8_ct_rangecheck('A', 'Z', src[i]) & 1);
        in_range |= (uint8_ct_rangecheck('a', 'z', src[i]) & 1);
        in_range |= (uint8_ct_rangecheck('0', '9', src[i]) & 1);
        in_range |= (uint8_ct_rangecheck('-', '-', src[i]) & 1);
        in_range |= (uint8_ct_rangecheck('_', '_', src[i]) & 1);
        valid_count += in_range; /* increment if true*/
    }

    /* check the last 3 bytes in the buffer and count valid padding*/
    /* padding_bits bit 1-3 are set to one if there is padding*/
    /* at the corresponding byte position from the end of the src buffer.*/
    /**/
    /* padding_valid is then only valid as long as all bits positions*/
    /* occur in consecutive order from the end of the buffer.*/
    /* 1: 0b0001 3: 0b0011 7: 9b0111*/
    /* (padding has to be at the end of the buffer, if it exists)*/
    /**/
    /* for each padding byte at a valid position padding_count is then incremented*/
    size_t padding_count = 0;
    uint8_t padding_bits = 0;

    /* constant time for len >= 3*/
    const size_t max_padding = MIN(len, 3);
    for (i = 0; i < max_padding; i++)
    {
        int pos           = (len - 1) - i;
        int padding_valid = 0;
        /* check padding pattern, accept '=' and '.' set padding location bit*/
        padding_bits |= uint8_ct_rangecheck('=', '=', src[pos]) & (1 << i); /* set bit for =*/
        padding_bits |= uint8_ct_rangecheck('.', '.', src[pos]) & (1 << i); /* or for .*/
        /* one of the padding_bits checks must always apply to set padding_valid to 1*/
        /* compares with all valid values.*/
        padding_valid |= uint8_ct_rangecheck(1, 1, padding_bits) & 1;
        padding_valid |= uint8_ct_rangecheck(3, 3, padding_bits) & 1;
        padding_valid |= uint8_ct_rangecheck(7, 7, padding_bits) & 1;
        /* add (one or zero)*/
        padding_count += padding_valid; /* count all bytes where the padding is valid*/
    };

    /* erase output count*/
    *nchars = 0;

    /* valid chars + valid padding must be 'len' or there are invalid characters or invalid padding*/
    /* invalid input is rejected now, this is not masked by constant time computation.§*/
    /* but we only want to protect character locations leading to a validity decision, not the validity decision
     * itself.*/

    if (len == valid_count + padding_count)
    {
        /* calculate the needed output buffer size*/
        /* outlen = ( ( valid_count * 6 ) + 7 ) >> 3; but this could overflow*/
        /* so we split the value and multiply twice and add the results*/

        size_t outlen = (6 * (valid_count >> 3)) + ((6 * (valid_count & 0x7) + 7) >> 3) - 1;

        if (dest == NULL || size < outlen)
        {
            /* output buffer to small*/
            *nchars = outlen;
            result  = -1;
        }
        else
        {
            uint32_t value          = 0;
            uint8_t *p              = dest;
            const uint8_t *s        = src;
            const uint8_t *const pe = dest + outlen;
            const uint8_t *const se = src + valid_count;
            /* we are going to return success now, the rest must always succeed*/
            result = 0;
            /* input is valid, input length is known, output fits into dest buffer*/
            /* decoding done in constant time only if outlen is a multiple of three*/
            i           = 0;
            int padding = 0;
            while (p < pe)
            {
                value <<= 6; /* next 6 bits*/
                if (s < se)
                {
                    value |= base64url_value_for_char(*s++);
                }
                else
                {
                    padding++;
                }
                if (++i % 4 == 0) /* every 4th char*/
                {
                    *p++ = (uint8_t)((value >> 16) & 0xff);
                    if (padding < 2)
                    {
                        *p++ = (uint8_t)((value >> 8) & 0xff);
                    }
                    if (padding < 1)
                    {
                        *p++ = (uint8_t)((value)&0xff);
                    }
                }
            }
            *nchars = (p - dest);
            result  = 0;
        }
    }
    return result;
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file awdg_utils_int.c
 * @brief Implements utility functions needed by the AWDG client.
 *
 * Do not include outside the AWDG client module!
 */
#include "awdg_utils_int.h"
#include "mbedtls/ssl.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Checks if the given character is a whitespace (SPACE or TAB).
 *
 * @param[in] character The character to examine.
 * @retval true The given character is a whitespace.
 * @retval false The given character is not a whitespace.
 */
static bool IsWhitespace(char character)
{
    bool ret = false;

    if ((' ' == character) || ('\t' == character))
    {
        ret = true;
    }

    return ret;
}

/*!
 * @brief Converts a ASCII digit to its numerical value.
 *
 * @param[in] character The character containing the ASCII digit.
 * @return The numerical value of the ASCII digit in character, 
 *         or UINT8_MAX if no digit in character.
 */
static uint8_t DigitToValue(char character)
{
    uint8_t ret = UINT8_MAX;

    /* valid digit (ASCII) */
    if ((character >= '0') && (character <= '9'))
    {
        ret = character - '0';
    }

    return ret;
}

/*!
 * @brief Convert a string to a number (base 10).
 *
 * @param[in] pStr Pointer to the string representing the number.
 * @param[in] strLen Length of the string.
 * @param[out] pNumber Pointer to an uint32_t in which the result is stored.
 * @return A bool representing the result of the operation.
 * @retval true Operation was successful, result is in pNumber.
 * @retval false Operation failed, pNumber stayed unmodified.
 */
bool AWDG_UTILS_ParseUint32(const char *pStr, size_t strLen, uint32_t *pNumber)
{
    const char *pTrimmedStr = pStr;
    uint32_t result         = 0U;
    uint32_t power          = 1U;
    uint8_t val             = 0U;
    uint32_t tmp            = 0U;
    size_t trimmedStrLen    = strLen;

    assert((NULL != pStr) && (NULL != pNumber));

    /* remove leading whitespace */
    for (; (trimmedStrLen > 0U); trimmedStrLen--)
    {
        if(IsWhitespace(pTrimmedStr[0]))
        {
            /* remove whitespace */
            pTrimmedStr++;
        }
        else
        {
            break;
        }
    }
    /* remove trailing whitespace */
    for (; (trimmedStrLen > 0U); trimmedStrLen--)
    {
        if(!IsWhitespace(pTrimmedStr[trimmedStrLen - 1U]))
        {
            break;
        }
    }

    /* empty string is not a valid number */
    if (0U == trimmedStrLen)
    {
        return false;
    }
    /* an uint32 can not have more than 10 digits */
    if (trimmedStrLen > UINT32_MAX_DIGITS)
    {
        return false;
    }

    /* convert to number
     * overflow can only happen at most significant digit, processed later (length was checked before) */
    for (size_t pos = trimmedStrLen - 1U; pos > 0U; pos--)
    {
        val = DigitToValue(pTrimmedStr[pos]);
        /* invalid character */
        if (UINT8_MAX == val)
        {
            return false;
        }
        result += val * power;
        power *= 10U;
    }

    /* for most significant digit include overflow check */
    val = DigitToValue(pTrimmedStr[0U]);
    /* invalid character */
    if (UINT8_MAX == val)
    {
        return false;
    }
    /* overflow check 1 */
    if (val > (UINT32_MAX / power))
    {
        return false;
    }
    tmp = val * power;
    /* overflow check 2 */
    if (result > (UINT32_MAX - tmp))
    {
        return false;
    }
    result += tmp;

    /* write back result */
    *pNumber = result;
    return true;
}

/*!
 * @brief Secure element network recv function.
 *
 * See www.freertos.org/network-interface.html.
 *
 * @param[in, out] pNetworkContext Implementation-defined network context.
 * @param[in, out] pBuffer Buffer to receive the data into.
 * @param[in] bytesToRecv Number of bytes requested from the network.
 * @return The number of bytes received or a negative value to indicate error.
 */
int32_t AWDG_UTILS_SERecv(NetworkContext_t *pNetworkContext, void *pBuffer, size_t bytesToRecv)
{
    int32_t ret = 0;

    assert((NULL != pNetworkContext) && (NULL != pNetworkContext->pParams));

    /* possible return values:
     * SOCKETS_SOCKET_ERROR, SOCKETS_EINVAL, MBEDTLS_ERR_SSL_INTERNAL_ERROR, errors from mbedtls_ssl_read
     */
    ret = SecureSocketsTransport_Recv(pNetworkContext, pBuffer, bytesToRecv);
    if ((MBEDTLS_ERR_SSL_TIMEOUT == ret) || (MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        /* for these errors user should retry */
        ret = 0;
    }

    return ret;
}

/*!
 * @brief Secure element network send function.
 *
 * See www.freertos.org/network-interface.html.
 *
 * @param[in, out] pNetworkContext Implementation-defined network context.
 * @param[in] pBuffer Buffer containing the bytes to send over the network.
 * @param[in] bytesToSend Number of bytes to send over the network.
 * @return The number of bytes sent or a negative value to indicate error.
 */
int32_t AWDG_UTILS_SESend(NetworkContext_t *pNetworkContext, const void *pBuffer, size_t bytesToSend)
{
    int32_t ret = 0;

    assert((NULL != pNetworkContext) && (NULL != pNetworkContext->pParams));

    /* possible return values:
     * SOCKETS_SOCKET_ERROR, SOCKETS_EINVAL, MBEDTLS_ERR_SSL_INTERNAL_ERROR, errors from mbedtls_ssl_write
     */
    ret = SecureSocketsTransport_Send(pNetworkContext, pBuffer, bytesToSend);
    if ((MBEDTLS_ERR_SSL_TIMEOUT == ret) || (MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        /* for these errors user should retry */
        ret = 0;
    }

    return ret;
}

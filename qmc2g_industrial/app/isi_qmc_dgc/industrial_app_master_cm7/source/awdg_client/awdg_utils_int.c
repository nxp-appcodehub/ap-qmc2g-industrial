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
 * Definitions
 ******************************************************************************/
#if SIZE_MAX < INT32_MAX
#error size_t implementation must be able to represent INT32_MAX!
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Checks if the given character is a whitespace (SPACE or TAB).
 *
 * @startuml
 * start
 * if (character) then (space || tab)
 *   :return true;
 *   stop
 * endif
 * :return false;
 * stop
 * @enduml
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
 * @startuml
 * start
 * if (character in {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}) then (true)
 *   :return character - '0';
 *   stop
 * endif
 * :return UINT8_MAX;
 * stop
 * @enduml
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
        ret = (uint8_t)character - (uint8_t)'0';
    }

    return ret;
}

/*!
 * @brief Convert a string to a number (base 10).
 *
 * @startuml
 * start
 * :Remove leading and trailing whitespaces (see IsWhitespace) from string pStr 
 * Resulting string is defined by pTrimmedStr and trimmedStrLen;
 * if (trimmedStrLen) then (0 || > UINT32_MAX_DIGITS)
 *   :return false;
 *   stop
 * endif
 * :result = 0
 * power = 1;
 * while(c in reversed pTrimmedStr)
 *   :val = DigitToValue(c);
 *   if () then (val == UINT8_MAX)
 *     :return false;
 *     stop
 *   endif
 *   :result = result + val * power
 *   power = power * 10;
 *   if (overflow) then (true)
 *     :return false;
 *     stop
 *   endif 
 * endwhile (else)
 * :~*pNumber = result;
 * stop
 * @enduml
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
    uint32_t result         = 0U;
    uint32_t power          = 1U;
    uint8_t val             = 0U;
    int32_t trimmedStartPos  = 0;
    int32_t trimmedEndPos    = 0;

    assert((NULL != pStr) && (NULL != pNumber));

    /* string length not supported - either 0 or too long */
    if ((0U == strLen) || (strLen > (size_t)INT32_MAX))
    {
    	return false;
    }
    /* last character */
    trimmedEndPos = strLen - 1U;

    /* remove leading whitespace */
    for (; (trimmedStartPos <= trimmedEndPos); trimmedStartPos++)
    {
        if(!IsWhitespace(pStr[trimmedStartPos]))
        {
            break;
        }
    }
    /* remove trailing whitespace */
    for (; (trimmedStartPos <= trimmedEndPos); trimmedEndPos--)
    {
        if(!IsWhitespace(pStr[trimmedEndPos]))
        {
            break;
        }
    }

    /* empty string is not a valid number */
    if (trimmedStartPos > trimmedEndPos)
    {
        return false;
    }
    /* an uint32 can not have more than 10 digits */
    if ((trimmedEndPos - trimmedStartPos + 1U) > UINT32_MAX_DIGITS)
    {
        return false;
    }

    /* convert to number
     * overflow can only happen at most-significant digit, processed later (length was checked before) */
    for (size_t pos = trimmedEndPos; pos > trimmedStartPos; pos--)
    {
        val = DigitToValue(pStr[pos]);
        /* invalid character */
        if (UINT8_MAX == val)
        {
            return false;
        }
        result += val * power;
        power *= 10U;
    }

    /* for most significant digit include overflow check */
    val = DigitToValue(pStr[trimmedStartPos]);
    /* invalid character */
    if (UINT8_MAX == val)
    {
        return false;
    }
    
    /* overflow check */
    if (val > ((UINT32_MAX - result) / power))
    {
        return false;
    }
    result += val * power;

    /* write back result */
    *pNumber = result;
    return true;
}

/*!
 * @brief Secure element network recv function.
 *
 * See www.freertos.org/network-interface.html.
 *
 * @startuml
 * start
 * :ret = SecureSocketsTransport_Recv(pNetworkContext, pBuffer, bytesToRecv);
 * if (ret) then (one of {MBEDTLS_ERR_SSL_TIMEOUT, MBEDTLS_ERR_SSL_WANT_READ, MBEDTLS_ERR_SSL_WANT_WRITE})
 *   :ret = 0 (retry);
 * endif
 * :return ret;
 * stop
 * @enduml
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
 * @startuml
 * start
 * :ret = SecureSocketsTransport_Send(pNetworkContext, pBuffer, bytesToSend);
 * if (ret) then (one of {MBEDTLS_ERR_SSL_TIMEOUT, MBEDTLS_ERR_SSL_WANT_READ, MBEDTLS_ERR_SSL_WANT_WRITE})
 *   :ret = 0 (retry);
 * endif
 * :return ret;
 * stop
 * @enduml
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

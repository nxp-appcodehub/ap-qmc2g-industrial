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
 * @file awdg_utils_int.h
 * @brief Implements utility functions needed by the AWDG client.
 *
 * Do not include outside the AWDG client module!
 */
#ifndef AWDG_UTILS_INT_H
#define AWDG_UTILS_INT_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "se_secure_sockets.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define UINT32_MAX_DIGITS (10U) /*!< floor(log10(UINT32_MAX)) + 1 */

/*!
 * @brief Macro returning the length of an static array or string (including NULL terminator).
 *
 * @param[in] Statically allocated array or string.
 * @return The number of elements in the static array or string (including NULL terminator).
 */
#define LEN(a) (sizeof(a) / sizeof(a[0]))

/*!
 * @brief Macro returning the max. length of the Base64 encoding of n bytes.
 *
 * @param[in] n The number of bytes that should be Base64 encoded.
 * @return size_t The max. length of the Base64 encoding of the bytes.
 */
#define BASE64_LEN(n) (4U * (((size_t)n + 3U - 1U) / 3U))

/*!
 * @brief Macro returning the maximum value of a and b.
 *
 * @param[in] a A number.
 * @param[in] b A number.
 * @return max(a,b)
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*!
 * @brief Network Context structure for transmission using the secure element.
 */
struct NetworkContext
{
    SecureSocketsTransportParams_t *pParams;
};

/*******************************************************************************
 * API
 ******************************************************************************/

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
bool AWDG_UTILS_ParseUint32(const char *pStr, size_t strLen, uint32_t *pNumber);

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
int32_t AWDG_UTILS_SERecv(NetworkContext_t *pNetworkContext, void *pBuffer, size_t bytesToRecv);

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
int32_t AWDG_UTILS_SESend(NetworkContext_t *pNetworkContext, const void *pBuffer, size_t bytesToSend);

#endif

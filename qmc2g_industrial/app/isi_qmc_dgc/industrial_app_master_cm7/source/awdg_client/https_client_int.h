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
 * @file https_client_int.h
 * @brief Implements a HTTPS client on top of the SE transport functions.
 *
 * Do not include outside the AWDG client module!
 */
#ifndef HTTPS_CLIENT_INT_H
#define HTTPS_CLIENT_INT_H

#include <stdint.h>

#include "transport_interface.h"

/*!
 * @brief HTTPS Client Status enum
 *
 * Values less than 0 are specific error codes.
 * Value of 0 is a generic success response.
 */
typedef enum
{
    kStatus_HTTPS_CLIENT_Ok                     = 0,  /*!> Success, no error ocurred */
    kStatus_HTTPS_CLIENT_ErrHeaders             = -1, /*!> HTTP headers error */
    kStatus_HTTPS_CLIENT_ErrSend                = -2, /*!> HTTP send error */
    kStatus_HTTPS_CLIENT_ErrResponseContentType = -3, /*!> HTTP response has the wrong content type */
    kStatus_HTTPS_CLIENT_ErrResponseStatus      = -4  /*!> HTTP response is not 200 OK */

} https_client_status_t;

/*!
 * @brief Sends a POST request with the given configuration to a server.
 *
 * @startuml
 * start
 * :Prepare POST request to server pHostname and path pPath
 * pBuffer is used as buffer for headers and response;
 * :Initialize request headers;
 * if () then (fail)
 *  :return kStatus_HTTPS_CLIENT_ErrHeaders;
 *  stop
 * endif
 * :Add "Content-Type: application/json" header;
 * if () then (fail)
 *  :return kStatus_HTTPS_CLIENT_ErrHeaders;
 *  stop
 * endif
 * :Send prepared request with body pJson to server using secure sockets;
 * if () then (fail)
 *  :return kStatus_HTTPS_CLIENT_ErrSend;
 *  stop
 * endif
 * :Check response content type;
 * if () then (not JSON)
 *  :return kStatus_HTTPS_CLIENT_ErrResponseContentType;
 *  stop
 * endif
 * :Return response using pResponse and its length using pResponseLen;
 * if (response status code) then (not "200 OK")
 *  :return kStatus_HTTPS_CLIENT_ErrResponseStatus;
 *  stop
 * endif
 * :return kStatus_HTTPS_CLIENT_Ok;
 * stop
 * @enduml
 * 
 * @param[in] pHostname Pointer to the hostname string to which to connect to.
 * @param[in] hostnameLen Length (without null terminator) of the hostname string.
 * @param[in] pPath Pointer to the path string to which to send the request to.
 * @param[in] pathLen Length (without null terminator) of the path string.
 * @param[in] pJson Pointer to the JSON body, which will be sent to the server.
 * @param[in] jsonLen Length (without null terminator) of the JSON string.
 * @param[in,out] pBuffer Pointer to a data buffer which the function can manipulate.
 * @param[in] bufferLen Size of the buffer (used for HTTP request headers and response).
 * @param[out] pResponse Pointer which will point to the start of the response body in the buffer.
 * @param[out] pResponseLen Points to a size_t to which the length of the response is written to.
 * @param[in] pNetworkContext Pointer to a valid pNetworkContext object on which the request will be handled.
 * @return A https_client_status_t status code.
 * @retval kStatus_HTTPS_CLIENT_Ok Success
 * @retval kStatus_HTTPS_CLIENT_ErrHeaders Preparing the request headers failed.
 * @retval kStatus_HTTPS_CLIENT_ErrSend Sending the request failed.
 * @retval kStatus_HTTPS_CLIENT_ErrResponseContentType The content type of the response does not match the expectation.
 * @retval kStatus_HTTPS_CLIENT_ErrResponseStatus The response of the server was not "200 OK".
 */
https_client_status_t HTTPS_CLIENT_PostJson(const char *pHostname,
                                            const size_t hostnameLen,
                                            const char *pPath,
                                            const size_t pathLen,
                                            const char *pJson,
                                            const size_t jsonLen,
                                            uint8_t *pBuffer,
                                            const size_t bufferLen,
                                            const uint8_t **pResponse,
                                            size_t *pResponseLen,
                                            NetworkContext_t *pNetworkContext);

#endif

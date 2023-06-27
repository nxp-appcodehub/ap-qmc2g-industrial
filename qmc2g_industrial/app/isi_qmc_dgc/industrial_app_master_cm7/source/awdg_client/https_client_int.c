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
 * @file https_client_int.c
 * @brief Implements a HTTPS client on top of the SE transport functions.
 *
 * Do not include outside the AWDG client module!
 */
#include "https_client_int.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include MBEDTLS_CONFIG_FILE
#include "core_http_client.h"
#include "awdg_utils_int.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define HTTP_CODE_SUCCESS        (200U)         /*!> HTTP "200 OK" status code */
#define HTTP_CONTENT_TYPE_HEADER "Content-Type" /*!> HTTP "Content-Type" header string */
#define HTTP_CONTENT_TYPE_HEADER_LEN \
    (LEN(HTTP_CONTENT_TYPE_HEADER) - 1U)          /*!> Length of the HTTP "Content-Type" header string */
#define HTTP_CONTENT_TYPE_JSON "application/json" /*!> HTTP "application/json" content type string */
#define HTTP_CONTENT_TYPE_JSON_LEN \
    (LEN(HTTP_CONTENT_TYPE_JSON) - 1U)                    /*!> Length of the "application/json" content type string */
#define HTTP_METHOD_POST     "POST"                       /*!> HTTP POST method */
#define HTTP_METHOD_POST_LEN (LEN(HTTP_METHOD_POST) - 1U) /*!> HTTP POST method len */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Gets the current FreeRTOS scheduler uptime in ms.
 *
 * @return The current scheduler uptime in ms.
 */
static uint32_t getTimeMs(void)
{
    /* needed for HTTP client timeouts */
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/*!
 * @brief Sends a POST request with the given configuration to a server.
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
                                            NetworkContext_t *pNetworkContext)
{
    HTTPStatus_t httpLibraryStatus          = HTTPInvalidParameter;
    HTTPRequestInfo_t requestInfo           = {0};
    HTTPRequestHeaders_t requestHeaders     = {0};
    TransportInterface_t transportInterface = {0};
    HTTPResponse_t response                 = {0};
    const char *field                       = NULL;
    size_t fieldLen                         = 0U;

    assert((NULL != pHostname) && (NULL != pPath) && (NULL != pJson) && (NULL != pBuffer) && (NULL != pNetworkContext));
    assert((NULL == pResponse) || (NULL != pResponseLen));

    /* configure request */
    requestInfo.pMethod   = HTTP_METHOD_POST;
    requestInfo.methodLen = HTTP_METHOD_POST_LEN;
    requestInfo.pPath     = pPath;
    requestInfo.pathLen   = pathLen;
    requestInfo.pHost     = pHostname;
    requestInfo.hostLen   = hostnameLen;
    requestInfo.reqFlags  = 0U;
    /* configure request headers */
    requestHeaders.pBuffer   = pBuffer;
    requestHeaders.bufferLen = bufferLen;

    /* init headers */
    httpLibraryStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders, &requestInfo);
    if (HTTPSuccess != httpLibraryStatus)
    {
        return kStatus_HTTPS_CLIENT_ErrHeaders;
    }

    /* add content-type header */
    httpLibraryStatus = HTTPClient_AddHeader(&requestHeaders, HTTP_CONTENT_TYPE_HEADER, HTTP_CONTENT_TYPE_HEADER_LEN,
                                             HTTP_CONTENT_TYPE_JSON, HTTP_CONTENT_TYPE_JSON_LEN);
    if (HTTPSuccess != httpLibraryStatus)
    {
        return kStatus_HTTPS_CLIENT_ErrHeaders;
    }

    /* prepare transport interface */
    transportInterface.recv            = AWDG_UTILS_SERecv;
    transportInterface.send            = AWDG_UTILS_SESend;
    transportInterface.pNetworkContext = pNetworkContext;

    /* reuse request buffer for response */
    response.pBuffer   = pBuffer;
    response.bufferLen = bufferLen;
    response.getTime   = getTimeMs;

    /* try to transmit request */
    httpLibraryStatus =
        HTTPClient_Send(&transportInterface, &requestHeaders, (const uint8_t *)pJson, jsonLen, &response, 0);
    if (HTTPSuccess != httpLibraryStatus)
    {
        return kStatus_HTTPS_CLIENT_ErrSend;
    }

    /* check response content type */
    httpLibraryStatus =
        HTTPClient_ReadHeader(&response, HTTP_CONTENT_TYPE_HEADER, HTTP_CONTENT_TYPE_HEADER_LEN, &field, &fieldLen);
    if ((HTTPSuccess != httpLibraryStatus) || (NULL == field) ||
        (0 != strncmp(field, HTTP_CONTENT_TYPE_JSON, fieldLen)))
    {
        return kStatus_HTTPS_CLIENT_ErrResponseContentType;
    }

    /* return body pointer, data stored in user supplied buffer */
    if (NULL != pResponse)
    {
        *pResponse    = response.pBody;
        *pResponseLen = response.bodyLen;
    }

    /* fail if status by server was not "200 OK" */
    if (HTTP_CODE_SUCCESS != response.statusCode)
    {
        return kStatus_HTTPS_CLIENT_ErrResponseStatus;
    }

    return kStatus_HTTPS_CLIENT_Ok;
}

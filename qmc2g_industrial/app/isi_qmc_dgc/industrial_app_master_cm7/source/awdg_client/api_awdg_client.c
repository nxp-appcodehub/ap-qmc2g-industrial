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
 * @file awdg_client_api.c
 * @brief Implements functions for communicating with the AWDG ticket server from a FreeRTOS environment.
 *
 */
#include "api_awdg_client.h"

#include <string.h>
#include <utils/debug.h>
#include <assert.h>

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/base64.h"
#include "se_secure_sockets.h"
#include "awdg_utils_int.h"
#include "https_client_int.h"
#include "awdg_ticket_int.h"
#include "core_http_client.h"
#include "se_session.h"
#include "qmc_features_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SERVER_PORT     (443U)                  /*!< Port of the AWDG server (TLS) */
#define SERVER_PATH     "/device/watchdog"      /*!< Watchdog resource path */
#define SERVER_PATH_LEN (LEN(SERVER_PATH) - 1U) /*!< Length of the watchdog resource path */

#define TICKET_REQUEST_BODY_BEGIN     "{\"nonce\":\""                       /*!< Ticket body begin */
#define TICKET_REQUEST_BODY_BEGIN_LEN (LEN(TICKET_REQUEST_BODY_BEGIN) - 1U) /*!< Ticket body begin length */
#define TICKET_REQUEST_BODY_END       "\"}"                                 /*!< Ticket body end */
#define TICKET_REQUEST_BODY_END_LEN   (LEN(TICKET_REQUEST_BODY_END) - 1U)   /*!< Ticket body end length */
#define TICKET_REQUEST_NONCE_BASE64_MAX_LEN                                   \
    (BASE64_LEN(AWDG_CLIENT_TICKET_NONCE_SIZE)) /*!< Max. length Base64 nonce \
                                                 */

#define HTTP_REQUEST_BODY_BUFFER_LEN                                       \
    (TICKET_REQUEST_BODY_BEGIN_LEN + TICKET_REQUEST_NONCE_BASE64_MAX_LEN + \
     TICKET_REQUEST_BODY_END_LEN) /*!< Buffer size for the HTTP request body. */
/* Request header and response buffer is used for two purposes:
 * A) Headers
 * Example (131bytes):
 *  POST /device/watchdog HTTP/1.1\r\n
 *  User-Agent: QMC2G\r\n
 *  Host: api.awdt.local\r\n
 *  Connection: keep-alive\r\n
 *  Content-Type: application/json\r\n\r\n
 * - Round up to 256 bytes for longer (here unknown) host names
 *  - Path + User-Agent length can be added
 * B) Response (JSON ticket)
 * Example (with 512bit curve 237bytes):
 *  {
 *    "ticket":
 *    {
 *      "timeout": <U32 (10 bytes max)>,
 *      "signature": "<B64 encoded signature ()>"
 *    }
 *  }
 * - A pretty-printed JSON without the timeout and the signature needs 66bytes:
 *   - Reserve 512 bytes for server headers
 *   - Round up to 100 to allow different formatting
 *   - The timeout is U32 and therefore needs max. 10 bytes
 *   - The length of the signature value depends on the used curve
 */
#define HTTP_REQUEST_HEADERS_RESPONSE_BUFFER_LEN                  \
    MAX(256U + SERVER_PATH_LEN + LEN(HTTP_USER_AGENT_VALUE) - 1U, \
        512U + 100U + UINT32_MAX_DIGITS +                         \
            BASE64_LEN(                                           \
                AWDG_CLIENT_MAX_RAW_SIGNATURE_SIZE)) /*!< Buffer size for the HTTP request headers and response. */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Requests a new ticket for the given nonce from the server.
 *
 * @param[in] pServerName Pointer to the null-terminated host name string to which to connect to
 * @param[in] pNonce Pointer to the nonce fetched from the AWDG
 * @param[out] pTicketBuffer Output pointer to the buffer in which the ticket will be placed (timeout:signature format)
 * @param[in,out] pTicketBufferLen Pointer to variable which contains the size of the ticket buffer.
 *                                 After a successful call this variable contains the received ticket's size.
 * @retval kStatus_AWDG_CLIENT_Ok Success, no error occurred 
 * @retval kStatus_AWDG_CLIENT_ErrArgInvalid Argument invalid 
 * @retval kStatus_AWDG_CLIENT_ErrConnection Connection error
 * @retval kStatus_AWDG_CLIENT_ErrRequest Request error 
 * @retval kStatus_AWDG_CLIENT_ErrJsonParsing JSON parsing error
 */
awdg_client_status_code_t AWDG_CLIENT_RequestTicket(const char *pServerName,
                                                    const uint8_t *pNonce,
                                                    const size_t nonceLen,
                                                    uint8_t *pTicketBuffer,
                                                    size_t *pTicketBufferLen)
{
    TransportSocketStatus_t socketReturn    = TRANSPORT_SOCKET_STATUS_CONNECT_FAILURE;
    https_client_status_t httpsClientReturn = kStatus_HTTPS_CLIENT_ErrSend;
    JSONStatus_t jsonReturn                 = JSONIllegalDocument;
    /* +1 for string 0 terminator */
    uint8_t nonceBase64[TICKET_REQUEST_NONCE_BASE64_MAX_LEN + 1U];
    size_t nonceBase64Len = 0U;
    char bodyBuffer[HTTP_REQUEST_BODY_BUFFER_LEN + 1U];
    uint8_t headersResponseBuffer[HTTP_REQUEST_HEADERS_RESPONSE_BUFFER_LEN];
    ServerInfo_t serverInfo                                     = {0};
    SecureSocketsTransportParams_t secureSocketsTransportParams = {0};
    NetworkContext_t netCtx                                     = {0};
    const uint8_t *pResponse                                    = NULL;
    size_t responseLen                                          = 0U;
    /* TLS SE indices */
    const se_client_tls_ctx_t kTlsCtx = {.server_root_cert_index = idCustomerCaCert,
                                         .client_keyPair_index   = idAwdtDevIdKeyPair,
                                         .client_cert_index      = idAwdtDevIdCert};
    /* secure socket configuration */
    const SocketsConfig_t kSocketConfig = {.enableTls     = true,
                                           .sendTimeoutMs = SECURE_WATCHDOG_SOCKET_TIMEOUT_MS,
                                           .recvTimeoutMs = SECURE_WATCHDOG_SOCKET_TIMEOUT_MS,
                                           /* no ALPN extension */
                                           .pAlpnProtos = NULL,
                                           /* allow SNI extension */
                                           .disableSni = false,
                                           /* no TLS may frequent length negotiation */
                                           .maxFragmentLength = 0U,
                                           /* server root CA check handled by secure element configuration */
                                           .pRootCa    = NULL,
                                           .rootCaSize = 0U};

    /* check input arguments */
    if (NULL == pServerName)
    {
        return kStatus_AWDG_CLIENT_ErrArgInvalid;
    }
    if ((NULL == pNonce) || (AWDG_CLIENT_TICKET_NONCE_SIZE != nonceLen))
    {
        return kStatus_AWDG_CLIENT_ErrArgInvalid;
    }
    if ((NULL == pTicketBuffer) || (NULL == pTicketBufferLen) || (*pTicketBufferLen < AWDG_CLIENT_MAX_RAW_TICKET_SIZE))
    {
        return kStatus_AWDG_CLIENT_ErrArgInvalid;
    }

    /* convert nonce to base64
     * can only fail if buffer is too small, which only happens if the input is too large */
    if (0 !=
        mbedtls_base64_encode(nonceBase64, TICKET_REQUEST_NONCE_BASE64_MAX_LEN + 1U, &nonceBase64Len, pNonce, nonceLen))
    {
        return kStatus_AWDG_CLIENT_ErrArgInvalid;
    }

    /* build body */
    const size_t bodyLen = TICKET_REQUEST_BODY_BEGIN_LEN + nonceBase64Len + TICKET_REQUEST_BODY_END_LEN;
    assert(bodyLen <= HTTP_REQUEST_BODY_BUFFER_LEN);
    memcpy(bodyBuffer, TICKET_REQUEST_BODY_BEGIN, TICKET_REQUEST_BODY_BEGIN_LEN);
    memcpy(bodyBuffer + TICKET_REQUEST_BODY_BEGIN_LEN, (char *) nonceBase64, nonceBase64Len);
    memcpy(bodyBuffer + TICKET_REQUEST_BODY_BEGIN_LEN + nonceBase64Len, TICKET_REQUEST_BODY_END,
           TICKET_REQUEST_BODY_END_LEN);
    bodyBuffer[bodyLen] = 0;

    /* set up TLS connection through secure element */

    /* AWDG ticket server connection details */
    serverInfo.pHostName      = pServerName;
    serverInfo.hostNameLength = strlen(pServerName);
    serverInfo.port           = SERVER_PORT;

    /* network context */
    netCtx.pParams = &secureSocketsTransportParams;

    /* establish connection (closes socket on error) */
    socketReturn = SE_SecureSocketsTransport_Connect(&netCtx, &serverInfo, &kSocketConfig, &kTlsCtx);
    if (TRANSPORT_SOCKET_STATUS_SUCCESS != socketReturn)
    {
        return kStatus_AWDG_CLIENT_ErrConnection;
    }

    /* post json */
    httpsClientReturn = HTTPS_CLIENT_PostJson(
        serverInfo.pHostName, serverInfo.hostNameLength, SERVER_PATH, SERVER_PATH_LEN, bodyBuffer, bodyLen,
        headersResponseBuffer, HTTP_REQUEST_HEADERS_RESPONSE_BUFFER_LEN, &pResponse, &responseLen, &netCtx);
    if (kStatus_HTTPS_CLIENT_Ok != httpsClientReturn)
    {
        /* SOCKET_Close() is always called and only fails if socket is invalid anyhow
         *  -> hence just ignore return value, we can not do anything anyhow */
        (void)SecureSocketsTransport_Disconnect(&netCtx);
        return kStatus_AWDG_CLIENT_ErrRequest;
    }

    /* transform to binary ticket */
    jsonReturn = AWDG_TICKET_JsonToBinaryTicket((const char *)pResponse, responseLen, pTicketBuffer, pTicketBufferLen);
    if (JSONSuccess != jsonReturn)
    {
        /* SOCKET_Close() is always called and only fails if socket is invalid anyhow
         *  -> hence just ignore return value, we can not do anything anyhow */
        (void)SecureSocketsTransport_Disconnect(&netCtx);
        return kStatus_AWDG_CLIENT_ErrJsonParsing;
    }

    /* SOCKET_Close() is always called and only fails if socket is invalid anyhow
     *  -> hence just ignore return value, we can not do anything anyhow */
    (void)SecureSocketsTransport_Disconnect(&netCtx);
    return kStatus_AWDG_CLIENT_Ok;
}

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef SECURE_ELEMENT_SE_SECURE_SOCKETS_H_
#define SECURE_ELEMENT_SE_SECURE_SOCKETS_H_

#include "api_qmc_common.h"
#include "transport_secure_sockets.h"

/*!
 * @brief Initialize secure element enabled secure sockets.
 *
 * Must be called only once!
 *
 * @return True on success;
 */
qmc_status_t SE_InitSecureSockets(void);

/*
 * Represents the object IDs on the secure element of the objects
 * that the TLS client might need.
 */
typedef struct
{
    uint32_t server_root_cert_index;
    uint32_t client_keyPair_index;
    uint32_t client_cert_index;
} se_client_tls_ctx_t;



/*!
 * @brief Sets up a TCP only connection or a TLS session on top of a TCP connection with Secure Sockets API.
 *
 * Use this function instead of SecureSocketsTransport_Connect(...)
 * to pass extra information related to the provisioned material
 * on the secure element.
 * For the rest, the normal SecureSocketsTransport_*() functions
 * should be used.
 * If mTLS is not need and no client material exist on the secure
 * element, pass 0 to the client_keyPair_index and client_cert_index
 * members of the pSeClientTlsContext structure.
 * If the server root certificate residing in RAM is to be used,
 * pass 0 to the server_root_cert_index member of the
 * se_client_tls_ctx_t structure and instead use the usual members
 * pRootCa and rootCaSize of the pSocketsConfig structure.
 *
 * @param[out] pNetworkContext The output parameter to return the created network context.
 * @param[in] pServerInfo Server connection info.
 * @param[in] pSocketsConfig socket configs for the connection.
 * @param[in] pSeClientTlsContext secure object IDs on the secure element.
 *
 * @return #TRANSPORT_SOCKET_STATUS_SUCCESS on success;
 *         #TRANSPORT_SOCKET_STATUS_INVALID_PARAMETER, #TRANSPORT_SOCKET_STATUS_INSUFFICIENT_MEMORY,
 *         #TRANSPORT_SOCKET_STATUS_CREDENTIALS_INVALID, #TRANSPORT_SOCKET_STATUS_INTERNAL_ERROR,
 *         #TRANSPORT_SOCKET_STATUS_DNS_FAILURE, #TRANSPORT_SOCKET_STATUS_CONNECT_FAILURE on failure.
 */
TransportSocketStatus_t SE_SecureSocketsTransport_Connect( NetworkContext_t * pNetworkContext,
                                                        const ServerInfo_t * pServerInfo,
                                                        const SocketsConfig_t * pSocketsConfig,
														const se_client_tls_ctx_t * pSeClientTlsContext);


#endif /* SECURE_ELEMENT_SE_SECURE_SOCKETS_H_ */

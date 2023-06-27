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
 * @file api_awdg_client.h
 * @brief Implements functions for communicating with the AWDG ticket server in an FreeRTOS environment.
 *
 */
#ifndef API_AWDG_CLIENT_H
#define API_AWDG_CLIENT_H

#include <stdlib.h>
#include <stdint.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define AWDG_CLIENT_MAX_RAW_SIGNATURE_SIZE (140U) /*!< from qmc2g_industrial_M4SLAVE\source\awdg\awdg_api.h */
#define AWDG_CLIENT_MAX_RAW_TICKET_SIZE    (AWDG_CLIENT_MAX_RAW_SIGNATURE_SIZE + 4U)
#define AWDG_CLIENT_TICKET_NONCE_SIZE      (32U) /*!< from qmc2g_industrial_M4SLAVE\source\awdg\awdg_api.h */

/*! \public
 * @brief TLS Client Error enum
 *
 * Values less than 0 are specific error codes
 * Value of 0 is a generic success response
 */
typedef enum
{
    /** Success return value - no error occurred */
    kStatus_AWDG_CLIENT_Ok             = 0,  /*!< Success, no error occurred */
    kStatus_AWDG_CLIENT_ErrArgInvalid  = -1, /*!< Argument invalid */
    kStatus_AWDG_CLIENT_ErrConnection  = -2, /*!< Connection error */
    kStatus_AWDG_CLIENT_ErrRequest     = -3, /*!< Request error */
    kStatus_AWDG_CLIENT_ErrJsonParsing = -4  /*!< JSON parsing error */
} awdg_client_status_code_t;

/*******************************************************************************
 * API
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
                                                    size_t *pTicketBufferLen);

#endif

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
 * @file awdg_ticket_int.c
 * @brief Implements functions for converting an JSON AWDG ticket to a binary AWDG ticket.
 *
 * Do not include outside the AWDG client module!
 */
#include "awdg_ticket_int.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/base64.h"
#include "core_json.h"
#include "awdg_utils_int.h"
#include "api_awdg_client.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define AWDG_TICKET_JSON_SIGNATURE_FIELD "ticket.signature" /*!< Ticket JSON signature field */
#define AWDG_TICKET_JSON_SIGNATURE_FIELD_LEN \
    (LEN(AWDG_TICKET_JSON_SIGNATURE_FIELD) - 1U)        /*!< Ticket JSON signature field len */
#define AWDG_TICKET_JSON_TIMEOUT_FIELD "ticket.timeout" /*!< Ticket JSON timeout field */
#define AWDG_TICKET_JSON_TIMEOUT_FIELD_LEN \
    (LEN(AWDG_TICKET_JSON_TIMEOUT_FIELD) - 1U) /*!< Ticket JSON timeout field length */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Converts a JSON ticket from the server to a binary ticket.
 *
 * @param[in] pJson Pointer to the JSON string to convert.
 * @param[in] jsonLen Length of the JSON string.
 * @param[out] pTicketBuffer Pointer to the buffer where the converted ticket is written.
 * @param[in,out] pTicketBufferSize Pointer to a variable which should contain the length of the input buffer.
 *                                  Should have a size of at least AWDG_TICKET_MAX_RAW_TICKET_SIZE.
 *                                  If the function was successful, the actual ticket size is written into this
 *                                  variable.
 * @return JSONStatus_t status code
 * @retval JSONSuccess The function was successful.
 * @retval !JSONSuccess An error occurred.
 */
JSONStatus_t AWDG_TICKET_JsonToBinaryTicket(const char *pJson,
                                            const size_t jsonLen,
                                            uint8_t *pTicketBuffer,
                                            size_t *pTicketBufferSize)
{
    JSONStatus_t jsonRet      = JSONIllegalDocument;
    int intRet                = -1;
    const char *pSignatureStr = NULL;
    size_t signatureStrLength = 0U;
    const char *pTimeoutStr   = NULL;
    size_t timeoutStrLength   = 0U;
    uint32_t timeout          = 0U;
    size_t ticketSize         = 0U;

    assert((NULL != pJson) && (NULL != pTicketBuffer) && (NULL != pTicketBufferSize));
    assert(*pTicketBufferSize >= AWDG_CLIENT_MAX_RAW_TICKET_SIZE);

    /* {
     *    "ticket":
     *    {
     *      "timeout":1751477360,
     *      "signature":"MEQCIDwoLFgSvNooj4N1yswaWcdQ371r2948/MdEUmraHCFLAiBt+s0w4VcQLGl9Z/XaoiLr78YKJaUhGxeZt6iSnTgbjQ=="
     *    }
     * }
     */

    /* parse json */
    jsonRet = JSON_Validate(pJson, jsonLen);
    if (JSONSuccess != jsonRet)
    {
        return JSONIllegalDocument;
    }

    /* get signature field */
    jsonRet = JSON_SearchConst(pJson, jsonLen, AWDG_TICKET_JSON_SIGNATURE_FIELD, AWDG_TICKET_JSON_SIGNATURE_FIELD_LEN,
                               &pSignatureStr, &signatureStrLength, NULL);
    if (JSONSuccess != jsonRet)
    {
        return JSONIllegalDocument;
    }

    /* get timeout field */
    jsonRet = JSON_SearchConst(pJson, jsonLen, AWDG_TICKET_JSON_TIMEOUT_FIELD, AWDG_TICKET_JSON_TIMEOUT_FIELD_LEN,
                               &pTimeoutStr, &timeoutStrLength, NULL);
    if (JSONSuccess != jsonRet)
    {
        return JSONIllegalDocument;
    }

    /* parse uint32 */
    if (!AWDG_UTILS_ParseUint32(pTimeoutStr, timeoutStrLength, &timeout))
    {
        return JSONIllegalDocument;
    }
    /* write timeout in LE format into binary ticket */
    pTicketBuffer[0U] = (uint8_t)(timeout & 0xFFU);
    pTicketBuffer[1U] = (uint8_t)((timeout >> 8U) & 0xFFU);
    pTicketBuffer[2U] = (uint8_t)((timeout >> 16U) & 0xFFU);
    pTicketBuffer[3U] = (uint8_t)((timeout >> 24U) & 0xFFU);

    /* decode signature and write into ticket buffer */
    intRet = mbedtls_base64_decode(pTicketBuffer + sizeof(uint32_t), *pTicketBufferSize - sizeof(uint32_t), &ticketSize,
                                   (const unsigned char *)pSignatureStr, signatureStrLength);
    if (0 != intRet)
    {
        return JSONIllegalDocument;
    }
    ticketSize += sizeof(uint32_t);

    *pTicketBufferSize = ticketSize;
    return JSONSuccess;
}

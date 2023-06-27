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
 * @file awdg_ticket_int.h
 * @brief Implements functions for converting an JSON AWDG ticket to a binary AWDG ticket.
 *
 * Do not include outside the AWDG client module!
 */
#ifndef AWDG_TICKET_INT_H
#define AWDG_TICKET_INT_H

#include <stdint.h>

#include "core_json.h"

/*******************************************************************************
 * API
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
                                            size_t *pTicketBufferSize);

#endif
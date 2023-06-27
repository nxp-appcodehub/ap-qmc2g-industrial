/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_RPC_INTERNAL_H_
#define _API_RPC_INTERNAL_H_

#include "api_rpc.h"


/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Set the four output pins / two SPI select pins on the SNVS domain to the desired state.
 *
 * May return kStatus_QMC_Timeout, if the CM4 core cannot handle the request in time.
 *
 * @param[in] gpioState Bit 0 - 3 represent DIG.INPUT4 - 7 states (1=high, 0=low);
 *                      Bit 4 - 7 are a mask (1=apply state, 0=do not apply state).
 *                      Bit 8 - 9 represent SPI_SEL0 - 1 states;
 *                      Bit 10 - 11 are the corresponding mask.
 * @return A qmc_status_t status code.
 * @retval kStatus_QMC_Timeout
 * Acquiring a mutex or command execution on the CM4 timed out.
 * @retval kStatus_QMC_ErrSync
 * Trying to recover the remote call from a previous timeout failed.
 * @retval kStatus_QMC_Ok
 * The remote call finished successfully.
 */
qmc_status_t RPC_SetSnvsOutput(uint16_t gpioState);

/*!
* @brief Registers IRQ handlers and Initializes event groups, mutexes and data structures used by the RemoteProcedureCallAPI.
*
* This function is meant to be called by the main function / startup task.
*/
void RPC_Init();

/*!
 * @brief Processes incoming events from the CM4.
 *
 * This function must be called from the GPR_IRQ SW interrupt.
 *
 */
void RPC_HandleISR(void);

#endif /* _API_RPC_INTERNAL_H_ */

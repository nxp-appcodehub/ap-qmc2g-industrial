/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef FAULT_DETECTION_TASKS_H
#define FAULT_DETECTION_TASKS_H

#include "api_fault.h"

/*!
 * @brief Initiates the fault handling task, then starts an infinite loop that
 * reads incoming faults and handles and logs them.
 *
 * @param[in] pvParameters Unused.
 */
void FaultHandlingTask(void *pvParameters);

/*!
 * @brief A generic fault handler.
 *
 * @param[in] fault The fault that should be written into the buffer.
 *
 * @detail  Generic fault handler. Not used by the QMC2G demo application.
 * 			All interrupts calling the FAULT_RaiseFaultEvent_fromISR must
 * 			have the same priority as the fast motor control loop interrupts
 * 			or issues with the fault buffer may occur. Other solutions include
 *			using a critical section, mutexes etc. However, it is recommended
 * 			to always use polling instead of interrupts because causing any
 * 			delays to the fast motor control interrupt handling will result
 * 			in undefined behavior.
 */
void faulthandler_x_callback(fault_source_t src);

#endif /* FAULT_DETECTION_TASKS_H */

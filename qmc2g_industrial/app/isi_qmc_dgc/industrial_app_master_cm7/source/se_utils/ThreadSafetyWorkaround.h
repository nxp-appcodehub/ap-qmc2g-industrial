/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef SE_UTILS_THREADSAFETYWORKAROUND_H_
#define SE_UTILS_THREADSAFETYWORKAROUND_H_
#include "fsl_sss_api.h"

sss_status_t ThreadSafetyWorkaround_Initialize(void);
void ThreadSafetyWorkaround_Deinitialize(void);
void ThreadSafetyWorkaround_Lock(void);
void ThreadSafetyWorkaround_Unlock(void);

#endif /* SE_UTILS_THREADSAFETYWORKAROUND_H_ */

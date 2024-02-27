/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file soft_mpu_api.c
 * @brief Implements a simple software-based MPU.
 */
#include "soft_mpu_api.h"

#include <stdbool.h>

/*******************************************************************************
 * Code
 ******************************************************************************/

bool SOFT_MPU_IsAccessAllowed(const soft_mpu_entry_t *const pSoftMpu,
                              const size_t softMpuEntries,
                              const uintptr_t accessBase,
                              size_t accessSize)
{
    /* by default accesses are forbidden */
    bool allowed = false;
    size_t accessLastOffset = 0U;
    uintptr_t accessLast = 0U;

    /* an access with size 0 does nothing, allow it */
    if (0U == accessSize)
    {
        return true;
    }

    /* check if access region overflows; accessSize is > 0 */
    accessLastOffset = accessSize - 1U;
    if (accessBase > (UINTPTR_MAX - accessLastOffset))
    {
        return false;
    }
    accessLast = accessBase + accessLastOffset;

    /* entry with the highest ID wins */
    for (size_t i = 0U; i < softMpuEntries; i++)
    {
        if (kSOFT_MPU_AccessAllow == pSoftMpu[i].policy)
        {
            /* an entry with ALLOW policy only matches if the whole access region lies within */
            if ((accessBase >= pSoftMpu[i].base) && (accessLast <= pSoftMpu[i].last))
            {
                allowed = true;
            }
        }
        else
        {
            /* an entry with DENY policy matches if any part of the access region lies within */
            if ((accessLast >= pSoftMpu[i].base) && (accessBase <= pSoftMpu[i].last))
            {
                allowed = false;
            }
        }
    }

    return allowed;
}

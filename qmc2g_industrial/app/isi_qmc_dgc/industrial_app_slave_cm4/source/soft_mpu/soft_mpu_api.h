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
 * @file soft_mpu_api.h
 * @brief Implements a simple software-based MPU.
 */
#ifndef _SOFT_MPU_API_H_
#define _SOFT_MPU_API_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*!
 * @brief Helper macro for creating a static MPU entry.
 * @param[in] rBase Base address of region.
 * @param[in] rSize Size of region (must not be 0!).
 * @param[in] rPolicy Access policy of region.
 * @return A static MPU entry.
 */
#define SOFT_MPU_STATIC_ENTRY_BASE_SIZE(rBase, rSize, rPolicy)                                   \
    {                                                                                            \
        .base = ((uintptr_t)rBase), .last = ((uintptr_t)rBase + rSize - 1U), .policy = (rPolicy) \
    }
/*!
 * @brief Helper macro for getting the size of a statically-allocated soft MPU.
 * @param[in] mpu Statically allocated soft MPU.
 * @return Number of MPU entries in MPU.
 */
#define SOFT_MPU_STATIC_ENTRIES(mpu) (sizeof(mpu) / sizeof(soft_mpu_entry_t))

/*! @brief Enumeration describing if access to a region is denied or allowed. */
typedef enum
{
    kSOFT_MPU_AccessDeny = 0U,
    kSOFT_MPU_AccessAllow
} soft_mpu_access_policy_t;

/*! @brief Structure describing a soft MPU entry. */
typedef struct
{
    uintptr_t base;                  /*!< Base address of the region */
    uintptr_t last;                  /*!< Last address within the region */
    soft_mpu_access_policy_t policy; /*!< Access policy (denied, allowed) */
} soft_mpu_entry_t;

/*******************************************************************************
 * API
 *******************************************************************************/

/*!
 * @brief Checks if the given soft MPU instance allows the specified access.
 *
 * The access is verified against the soft MPU entries according to a few simple rules:
 *      - By default (no matching soft MPU entry) all accesses are denied.
 *      - If a soft MPU entry matches depends on its policy:
 *          - kSOFT_MPU_AccessDeny: If any part of the access region lies within the region of the MPU entry,
 *                                  then it is a match.
 *          - kSOFT_MPU_AccessAllow: If the full access region lies within the region of the MPU entry,
 *                                   then it is a match.
 *      - Accesses spanning multiple consecutive MPU regions are always denied
 *        (i.e., MPU regions must be aligned accordingly).
 *      - The policy of the matching entry with the highest ID decides if an access is allowed or denied.
 *
 * @param[in] pSoftMpu Pointer to an array of soft_mpu_entry_t entries defining the MPU.
 * @param[in] softMpuEntries Number of entries in the MPU.
 * @param[in] accessBase Base address of the access to check.
 * @param[in] accessSize Size of the access to check.
 * @return True if access is allowed, false if denied.
 */
bool SOFT_MPU_IsAccessAllowed(const soft_mpu_entry_t *const pSoftMpu,
                              const size_t softMpuEntries,
                              const uintptr_t accessBase,
                              size_t accessSize);

#endif /* _SOFT_MPU_API_H_ */

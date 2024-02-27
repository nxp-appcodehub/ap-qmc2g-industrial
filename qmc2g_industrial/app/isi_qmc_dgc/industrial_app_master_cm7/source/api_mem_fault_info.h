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
 * @file api_mem_fault_info.h
 * @brief Module including helper functions to get more information about memory faults.
 *
 */
#ifndef _API_MEM_FAULT_INFO_H_
#define _API_MEM_FAULT_INFO_H_

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MEM_FAULT_INFO_MAX_DATA_WORDS (16U)

/*! @brief Structure describing the fault stack frame pushed by hardware and a compliant handler */
typedef struct __attribute__((packed, aligned(4)))
{
    /* pushed by custom exception handler */
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t reserved;
    /* pushed by hardware before entering exception */
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} mem_fault_info_stack_frame_t;

/*! @brief Structure describing a memory access fault. */
typedef struct
{
    uint32_t ins;        /*!< The raw instruction which caused the fault. */
    uint8_t insWidth;    /*!< The width of the instruction which caused the fault (2 or 4 bytes). */
    uintptr_t address;   /*!< The target address of the access which caused the fault. */
    uint8_t accessSize;  /*!< The size of the access (1, 2, 4 bytes). */
    uint32_t data[MEM_FAULT_INFO_MAX_DATA_WORDS]; /*!< The source words in case of a write access (max. 16 -> store
                                                     multiple).*/
    uint8_t dataWords;                            /*!< The number of valid source words. */
    uint32_t *pWritebackGpr; /*!< Pointer to the GPR in the fault stack frame to where a writeback should happen in case
                                of an successful access. */
    uint32_t writebackValue; /*!< The writeback value. */
} mem_fault_info_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Attempts to get more information about a write access fault using the fault stack frame and access address.
 *
 * The results are written to the mem_fault_info_t object pMemAccessFaultInfo points to.
 * Note that this functions only supports a limited number of instructions which can cause a write access fault.
 * For more information, refer to kStoreInstructionsW2 and kStoreInstructionsW4 in mem_fault_info.c.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame.
 *                             The fault stack frame must have the format specified by mem_fault_info_stack_frame_t.
 * @param[in] faultAccessAddress The target address of the write which caused the fault.
 * @param[out] pMemAccessFaultInfo Pointer to a mem_fault_info_t object where more information about the
 *                                 fault is written.
 * @return A boolean.
 * @retval true
 * Getting more information about the fault succeeded, refer to pMemAccessFaultInfo.
 * @retval false
 * Otherwise
 */
bool MEM_FAULT_INFO_GetWriteAccessFaultInformation(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                                   const uintptr_t faultAccessAddress,
                                                   mem_fault_info_t *const pMemAccessFaultInfo);

#endif /* _API_MEM_FAULT_INFO_H_ */

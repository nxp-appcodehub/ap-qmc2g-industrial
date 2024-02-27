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
 * @file mem_fault_info.c
 * @brief Module including helper functions to get more information about memory faults.
 *
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "api_mem_fault_info.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* defines to check if a instruction is 32- or 16-bit wide */
#define INS_W32_MASK   (0xF800U)
#define INS_W32_MATCH1 (0xE800U)
#define INS_W32_MATCH2 (0xF000U)
#define INS_W32_MATCH3 (0xF800U)

/*! @brief Enum assigning identifiers to the different field formats belonging to different instruction encodings. */
typedef enum
{
    kInsFieldFormatStrSingle = 0U,
    kInsFieldFormatStrSingleExt32,
    kInsFieldFormatStrMult16,
    kInsFieldFormatStrMult32,
    kInsFieldFormatStrDouble
} ins_field_format_id_t;

/*! @brief Structure describing the field format for 16-bit and 32-bit store single instructions. */
typedef struct
{
    uint32_t rtMask;
    uint8_t rtPos;
} ins_field_format_str_single_t;

/*! @brief Structure describing the extended field format for 32-bit store single instructions. */
typedef struct
{
    uint32_t rtMask;
    uint8_t rtPos;
    uint32_t rnMask;
    uint8_t rnPos;
    uint32_t imm8Mask;
    uint8_t imm8Pos;
    uint32_t indexMask;
    uint32_t addMask;
    uint32_t wbackMask;
} ins_field_format_str_single_ext32_t;

/*! @brief Structure describing the field format for 16-bit store multiple instructions. */
typedef struct
{
    uint32_t registersMask;
    uint8_t registersPos;
    uint32_t rnMask;
    uint8_t rnPos;
} ins_field_format_str_mult16_t;

/*! @brief Structure describing the field format for 32-bit store multiple instructions. */
typedef struct
{
    uint32_t registersMask;
    uint8_t registersPos;
    uint32_t rnMask;
    uint8_t rnPos;
    uint32_t wbackMask;
    bool decrementBefore;
} ins_field_format_str_mult32_t;

/*! @brief Structure describing the field format for 32-bit STRD (immediate) Encoding T1 instruction. */
typedef struct
{
    uint32_t rtMask;
    uint8_t rtPos;
    uint32_t rt2Mask;
    uint8_t rt2Pos;
    uint32_t rnMask;
    uint8_t rnPos;
    uint32_t imm8Mask;
    uint8_t imm8Pos;
    uint32_t indexMask;
    uint32_t addMask;
    uint32_t wbackMask;
} ins_field_format_strd_t;

typedef struct _ins_info ins_info_t;
typedef bool (*ins_match_fn_t)(const ins_info_t *const pInsInfo, const uint32_t rawIns);

/*! @brief Structure describing a store instruction .*/
typedef struct _ins_info
{
    uint32_t mask;
    uint32_t match;
    ins_match_fn_t matchFn;
    uint8_t width;
    uint8_t accessSize;
    uint8_t fieldsFormat;
    union
    {
        ins_field_format_str_single_t strSingle;
        ins_field_format_str_single_ext32_t strSingleExt32;
        ins_field_format_str_mult16_t strMult16;
        ins_field_format_str_mult32_t strMult32;
        ins_field_format_strd_t strDouble;
    } fields;
} ins_info_t;

/*!
 * @brief Performs additional checks if a word is a STR{B,H} (immediate) instruction.
 *
 * @param[in] pInsInfo Pointer to an ins_info_t structure describing the candidate instruction.
 * @param[in] rawIns Any word.
 * @return A boolean.
 * @retval true
 * rawIns disassembles to the instruction given by pInsInfo.
 * @retval false
 * Otherwise
 */
static bool strSingleExt32Match(const ins_info_t *const pInsInfo, const uint32_t rawIns);

/*!
 * @brief Performs additional checks if a word is a STR (immediate) instruction.
 *
 * @param[in] pInsInfo Pointer to an ins_info_t structure describing the candidate instruction.
 * @param[in] rawIns Any word.
 * @return A boolean.
 * @retval true
 * rawIns disassembles to the instruction given by pInsInfo.
 * @retval false
 * Otherwise
 */
static bool strSingleWordExt32Match(const ins_info_t *const pInsInfo, const uint32_t rawIns);

/*!
 * @brief Performs additional checks if a word is a STMDB instruction.
 *
 * @param[in] pInsInfo Pointer to an ins_info_t structure describing the candidate instruction.
 * @param[in] rawIns Any word.
 * @return A boolean.
 * @retval true
 * rawIns disassembles to the instruction given by pInsInfo.
 * @retval false
 * Otherwise
 */
static bool stmdbMatch(const ins_info_t *const pInsInfo, const uint32_t rawIns);

/*!
 * @brief Performs additional checks if a word is a STRD (immediate) instruction.
 *
 * @param[in] pInsInfo Pointer to an ins_info_t structure describing the candidate instruction.
 * @param[in] rawIns Any word.
 * @return A boolean.
 * @retval true
 * rawIns disassembles to the instruction given by pInsInfo.
 * @retval false
 * Otherwise
 */
static bool strDoubleMatch(const ins_info_t *const pInsInfo, const uint32_t rawIns);

/*******************************************************************************
 * Variables
 *******************************************************************************/

/*! @brief Information about relevant 16-bit store instructions (needed to disassemble).
 *
 * See armv7m architecture reference manual A5.2 for more details.
 * NOTE: The following kind of store instructions were skipped as not relevant for our usecase:
 * 		- Store exclusive instructions as they should not be used to access device memory
 * 		- Push instructions as the device memory is not used as stack
 * 		- Unprivileged store instructions as only privileged accesses are used currently and even if not,
 * 		  then accesses to device memory should only be done by privileged accesses
 */
static const ins_info_t kStoreInstructionsW2[] = {
    /* STRB (immediate) Encoding T1 */
    {.mask             = 0xF800U,
     .match            = 0x7000U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 1U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x7U, .rtPos = 0U}},
    /* STRH (immediate) Encoding T1 */
    {.mask             = 0xF800U,
     .match            = 0x8000U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 2U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x7U, .rtPos = 0U}},
    /* STR (immediate) Encoding T1 */
    {.mask             = 0xF800U,
     .match            = 0x6000U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x7U, .rtPos = 0U}},
    /* STR (immediate) Encoding T2 */
    {.mask             = 0xF800U,
     .match            = 0x9000U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x700U, .rtPos = 8U}},

    /* STRB (register) Encoding T1 */
    {.mask             = 0xFE00U,
     .match            = 0x5400U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 1U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x7U, .rtPos = 0U}},
    /* STRH (register) Encoding T1 */
    {.mask             = 0xFE00U,
     .match            = 0x5200U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 2U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x7U, .rtPos = 0U}},
    /* STR (register) Encoding T1 */
    {.mask             = 0xFE00U,
     .match            = 0x5000U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0x7U, .rtPos = 0U}},

    /* STM, STMIA, STMEA Encoding T1 */
    {.mask             = 0xF800U,
     .match            = 0xC000U,
     .matchFn          = NULL,
     .width            = 2U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrMult16,
     .fields.strMult16 = {
         .registersMask = 0xFFU,
         .registersPos  = 0U,
         .rnMask        = 0x700U,
         .rnPos         = 8U,
     }}};

/*! @brief Information about relevant 32-bit store instructions (needed to disassemble)
 *
 * See armv7m architecture reference manual A5.3 for more details
 * NOTE: The following kind of store instructions were skipped as not relevant for our usecase:
 * 		- Store exclusive instructions as they should not be used to access device memory
 * 		- Push instructions as the device memory is not used as stack
 * 		- Unprivileged store instructions as only privileged accesses are used currently and even if not,
 * 		  then accesses to device memory should only be done by privileged accesses
 */
static const ins_info_t kStoreInstructionsW4[] = {
    /* STRB (immediate) Encoding T2 */
    {.mask             = 0xFFF00000U,
     .match            = 0xF8800000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 1U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0xF000U, .rtPos = 12U}},
    /* STRB (immediate) Encoding T3 */
    {.mask         = 0xFFF00800U,
     .match        = 0xF8000800U,
     .matchFn      = strSingleExt32Match,
     .width        = 4U,
     .accessSize   = 1U,
     .fieldsFormat = kInsFieldFormatStrSingleExt32,
     .fields.strSingleExt32 =
         {
             .rtMask    = 0xF000U,
             .rtPos     = 12U,
             .rnMask    = 0x000F0000U,
             .rnPos     = 16U,
             .imm8Mask  = 0xFFU,
             .imm8Pos   = 0U,
             .indexMask = 0x400U,
             .addMask   = 0x200U,
             .wbackMask = 0x100U,
         }},
    /* STRH (immediate) Encoding T2 */
    {.mask             = 0xFFF00000U,
     .match            = 0xF8A00000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 2U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0xF000U, .rtPos = 12U}},
    /* STRH (immediate) Encoding T3 */
    {.mask         = 0xFFF00800U,
     .match        = 0xF8200800U,
     .matchFn      = strSingleExt32Match,
     .width        = 4U,
     .accessSize   = 2U,
     .fieldsFormat = kInsFieldFormatStrSingleExt32,
     .fields.strSingleExt32 =
         {
             .rtMask    = 0xF000U,
             .rtPos     = 12U,
             .rnMask    = 0x000F0000U,
             .rnPos     = 16U,
             .imm8Mask  = 0xFFU,
             .imm8Pos   = 0U,
             .indexMask = 0x400U,
             .addMask   = 0x200U,
             .wbackMask = 0x100U,
         }},
    /* STR (immediate) Encoding T3 */
    {.mask             = 0xFFF00000U,
     .match            = 0xF8C00000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0xF000U, .rtPos = 12U}},
    /* STR (immediate) Encoding T4 */
    {.mask         = 0xFFF00800U,
     .match        = 0xF8400800U,
     .matchFn      = strSingleWordExt32Match,
     .width        = 4U,
     .accessSize   = 4U,
     .fieldsFormat = kInsFieldFormatStrSingleExt32,
     .fields.strSingleExt32 =
         {
             .rtMask    = 0xF000U,
             .rtPos     = 12U,
             .rnMask    = 0xF0000U,
             .rnPos     = 16U,
             .imm8Mask  = 0xFFU,
             .imm8Pos   = 0U,
             .indexMask = 0x400U,
             .addMask   = 0x200U,
             .wbackMask = 0x100U,
         }},

    /* STRB (register) Encoding T2 */
    {.mask             = 0xFFF00FC0U,
     .match            = 0xF8000000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 1U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0xF000U, .rtPos = 12U}},
    /* STRH (register) Encoding T2 */
    {.mask             = 0xFFF00FC0U,
     .match            = 0xF8200000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 2U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0xF000U, .rtPos = 12U}},
    /* STR (register) Encoding T2 */
    {.mask             = 0xFFF00FC0U,
     .match            = 0xF8400000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrSingle,
     .fields.strSingle = {.rtMask = 0xF000U, .rtPos = 12U}},

    /* STM, STMIA, STMEA Encoding T2 */
    {.mask             = 0xFFD0A000U,
     .match            = 0xE8800000U,
     .matchFn          = NULL,
     .width            = 4U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrMult32,
     .fields.strMult32 = {.registersMask   = 0xFFFFU,
                          .registersPos    = 0U,
                          .rnMask          = 0xF0000U,
                          .rnPos           = 16U,
                          .wbackMask       = 0x200000U,
                          .decrementBefore = false}},
    /* STMDB, STMFD Encoding T2 */
    {.mask             = 0xFFD0A000U,
     .match            = 0xE9000000U,
     .matchFn          = stmdbMatch,
     .width            = 4U,
     .accessSize       = 4U,
     .fieldsFormat     = kInsFieldFormatStrMult32,
     .fields.strMult32 = {.registersMask   = 0xFFFFU,
                          .registersPos    = 0U,
                          .rnMask          = 0xF0000U,
                          .rnPos           = 16U,
                          .wbackMask       = 0x200000U,
                          .decrementBefore = true}},

    /* STRD (immediate) Encoding T1 */
    {.mask         = 0xFE500000U,
     .match        = 0xE8400000U,
     .matchFn      = strDoubleMatch,
     .width        = 4U,
     .accessSize   = 4U,
     .fieldsFormat = kInsFieldFormatStrDouble,
     .fields.strDouble =
         {
             .rtMask    = 0xF000U,
             .rtPos     = 12U,
             .rt2Mask   = 0x0F00U,
             .rt2Pos    = 8U,
             .rnMask    = 0xF0000U,
             .rnPos     = 16U,
             .imm8Mask  = 0xFFU,
             .imm8Pos   = 0U,
             .indexMask = 0x01000000U,
             .addMask   = 0x00800000U,
             .wbackMask = 0x00200000U,
         }},
};

/*******************************************************************************
 * Code
 ******************************************************************************/

static bool strSingleExt32Match(const ins_info_t *const pInsInfo, const uint32_t rawIns)
{
    /* if P == 1 && U == 1 && W == 0, then it is a STR{B,H}T instruction (not supported) */
    if ((rawIns & pInsInfo->fields.strSingleExt32.indexMask) && (rawIns & pInsInfo->fields.strSingleExt32.addMask) &&
        !(rawIns & pInsInfo->fields.strSingleExt32.wbackMask))
    {
        return false;
    }

    return true;
}

static bool strSingleWordExt32Match(const ins_info_t *const pInsInfo, const uint32_t rawIns)
{
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pInsInfo->fields.strSingleExt32.rnPos < 32U);
    uint8_t rn = (uint8_t)((rawIns & pInsInfo->fields.strSingleExt32.rnMask) >> pInsInfo->fields.strSingleExt32.rnPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pInsInfo->fields.strSingleExt32.imm8Pos < 32U);
    uint8_t imm8 =
        (uint8_t)((rawIns & pInsInfo->fields.strSingleExt32.imm8Mask) >> pInsInfo->fields.strSingleExt32.imm8Pos);

    /* if P == 1 && U == 1 && W == 0, then it is a STRT instruction (not supported) */
    if ((rawIns & pInsInfo->fields.strSingleExt32.indexMask) && (rawIns & pInsInfo->fields.strSingleExt32.addMask) &&
        !(rawIns & pInsInfo->fields.strSingleExt32.wbackMask))
    {
        return false;
    }

    /* if Rn == '1101' && P == 1 && U == 0 && W == 1 && imm8 == '00000100',
     * then it is a PUSH instruction (not supported) */
    if ((0xDU == rn) && (rawIns & pInsInfo->fields.strSingleExt32.indexMask) &&
        !(rawIns & pInsInfo->fields.strSingleExt32.addMask) && (rawIns & pInsInfo->fields.strSingleExt32.wbackMask) &&
        (0x4U == imm8))
    {
        return false;
    }

    return true;
}

static bool stmdbMatch(const ins_info_t *const pInsInfo, const uint32_t rawIns)
{
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pInsInfo->fields.strMult32.rnPos < 32U);
    uint8_t rn = (uint8_t)((rawIns & pInsInfo->fields.strMult32.rnMask) >> pInsInfo->fields.strMult32.rnPos);

    /* if W == 1 && Rn == '1101', then it is a PUSH instruction (not supported) */
    if ((rawIns & pInsInfo->fields.strMult32.wbackMask) && (0xDU == rn))
    {
        return false;
    }

    return true;
}

static bool strDoubleMatch(const ins_info_t *const pInsInfo, const uint32_t rawIns)
{
    /* if P == 0 && W == 0, then it is another instruction (not supported) */
    if (!(rawIns & pInsInfo->fields.strDouble.indexMask) && !(rawIns & pInsInfo->fields.strDouble.wbackMask))
    {
        return false;
    }

    return true;
}

/*!
 * @brief Returns an access pointer to a GPR in the fault stack frame.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame pushed by the hardware and custom handler.
 * @param[in] gpr Number identifying a GPR in the fault stack frame for which an access pointer should be returned.
 * @return Pointer to the specified GPRs data within the fault stack frame if it exists, NULL otherwise.
 */
static uint32_t *getFaultStackFrameGprPtr(mem_fault_info_stack_frame_t *const pFaultStackFrame, uint8_t gpr)
{
    assert(NULL != pFaultStackFrame);

    switch (gpr)
    {
        case 0U:
            return &pFaultStackFrame->r0;
        case 1U:
            return &pFaultStackFrame->r1;
        case 2U:
            return &pFaultStackFrame->r2;
        case 3U:
            return &pFaultStackFrame->r3;
        case 4U:
            return &pFaultStackFrame->r4;
        case 5U:
            return &pFaultStackFrame->r5;
        case 6U:
            return &pFaultStackFrame->r6;
        case 7U:
            return &pFaultStackFrame->r7;
        case 8U:
            return &pFaultStackFrame->r8;
        /* SB, TR */
        case 9U:
            return &pFaultStackFrame->r9;
        case 10U:
            return &pFaultStackFrame->r10;
        /* FP */
        case 11U:
            return &pFaultStackFrame->r11;
        /* IP */
        case 12U:
            return &pFaultStackFrame->r12;
        /* LR */
        case 14U:
            return &pFaultStackFrame->lr;
        /* PC */
        case 15U:
            return &pFaultStackFrame->pc;
        default:
            /* GPR does not exist in fault stack frame (SP) */
            return NULL;
    }

    /* GPR does not exist */
    return NULL;
}

/*!
 * @brief Checks if the word given by rawIns disassembles to the instruction given by pInsInfo.
 *
 * @param[in] pInsInfo Pointer to an ins_info_t structure describing the candidate instruction.
 * @param[in] rawIns Any word.
 * @return A boolean.
 * @retval true
 * rawIns disassembles to the instruction given by pInsInfo.
 * @retval false
 * Otherwise
 */
static bool isInstruction(const ins_info_t *const pInsInfo, uint32_t rawIns)
{
    assert(NULL != pInsInfo);
    /* first check match and mask */
    if (pInsInfo->match == (rawIns & pInsInfo->mask))
    {
        /* no additional match function, then it is a match*/
        if (NULL == pInsInfo->matchFn)
        {
            return true;
        }
        /* check optional match function if exists */
        else if (pInsInfo->matchFn(pInsInfo, rawIns))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

/*!
 * @brief Attempts to disassemble the data located at pPc into a supported store instruction.
 *
 * For a list of supported store instructions see kStoreInstructionsW2, kStoreInstructionsW4.
 *
 * @param[in] pPc Pointer to the location which should be disassembled.
 * @param[out] pMatchedInsInfo Pointer to an ins_info_t structure where details about the disassembled
 *                             instruction will be written.
 * @param[out] pRawIns Pointer to a uint32_t where the raw instruction located at pPc will be written.
 * @return A boolean.
 * @retval true
 * rawIns disassembled to a supported store instruction. More details can be found in pMatchedInsInfo and pRawIns.
 * @retval false
 * Otherwise
 */
static bool disassembleStore(const uint16_t *const pPc,
                             const ins_info_t **const pMatchedInsInfo,
                             uint32_t *const pRawIns)
{
    assert((NULL != pPc) && (NULL != pMatchedInsInfo) && (NULL != pRawIns));

    /* is it a 32 bit instruction? */
    uint16_t rawHwMasked = pPc[0] & INS_W32_MASK;
    if ((INS_W32_MATCH1 == rawHwMasked) || (INS_W32_MATCH2 == rawHwMasked) || (INS_W32_MATCH3 == rawHwMasked))
    {
        /* try to disassemble to a 32-bit instruction */
        for (size_t i = 0U; i < (sizeof(kStoreInstructionsW4) / sizeof(ins_info_t)); i++)
        {
            /* same layout as in the armv7m architecture reference manual */
            uint32_t raw = ((uint32_t)pPc[0] << (sizeof(uint16_t) * 8U)) | pPc[1];
            if (isInstruction(&kStoreInstructionsW4[i], raw))
            {
                *pMatchedInsInfo = &kStoreInstructionsW4[i];
                *pRawIns         = raw;
                return true;
            }
        }
    }
    else
    {
        /* try to disassemble to a 16-bit instruction */
        for (size_t i = 0U; i < (sizeof(kStoreInstructionsW2) / sizeof(ins_info_t)); i++)
        {
            uint32_t raw = pPc[0];
            if (isInstruction(&kStoreInstructionsW2[i], raw))
            {
                *pMatchedInsInfo = &kStoreInstructionsW2[i];
                *pRawIns         = raw;
                return true;
            }
        }
    }

    return false;
}

/*!
 * @brief Attempts to get more information about a write access fault caused by a store single instruction.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame.
 *                             The fault stack frame must have the format specified by mem_fault_info_stack_frame_t.
 * @param[in] faultAccessAddress The target address of the write which caused the fault.
 * @param[in] rawIns The instruction that caused the access fault.
 * @param[in] pMatchedInsInfo Pointer to an ins_info_t structure providing more information about the
 *                            instruction that caused the access fault.
 * @param[out] pMemAccessFaultInfo Pointer to a mem_fault_info_t object where more information about the
 *                                 fault is written.
 * @return A boolean.
 * @retval true
 * Getting more information about the fault succeeded, refer to pMemAccessFaultInfo.
 * @retval false
 * Otherwise
 */
static bool getWriteAccessFaultInfoStrSingle(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                             const uintptr_t faultAccessAddress,
                                             const uint32_t rawIns,
                                             const ins_info_t *const pMatchedInsInfo,
                                             mem_fault_info_t *const pMemAccessFaultInfo)
{
    (void)faultAccessAddress;

    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strSingle.rtPos < 32U);
    uint8_t rt =
        (uint8_t)((rawIns & pMatchedInsInfo->fields.strSingle.rtMask) >> pMatchedInsInfo->fields.strSingle.rtPos);
    uint32_t *pRt = NULL;

    /* fetch store data value */
    pRt = getFaultStackFrameGprPtr(pFaultStackFrame, rt);
    assert(NULL != pRt);
    if (NULL == pRt)
    {
        /* unsupported GPR -> invalid */
        return false;
    }

    /* save details about faulted store */
    pMemAccessFaultInfo->data[0U]  = *pRt;
    pMemAccessFaultInfo->dataWords = 1U;

    /* store single instructions with this field format never perform writeback */

    return true;
}

/*!
 * @brief Attempts to get more information about a write access fault caused by a store single instruction.
 *
 * Applies to store single instruction with extended 32-bit field format.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame.
 *                             The fault stack frame must have the format specified by mem_fault_info_stack_frame_t.
 * @param[in] faultAccessAddress The target address of the write which caused the fault.
 * @param[in] rawIns The instruction that caused the access fault.
 * @param[in] pMatchedInsInfo Pointer to an ins_info_t structure providing more information about the
 *                            instruction that caused the access fault.
 * @param[out] pMemAccessFaultInfo Pointer to a mem_fault_info_t object where more information about the
 *                                 fault is written.
 * @return A boolean.
 * @retval true
 * Getting more information about the fault succeeded, refer to pMemAccessFaultInfo.
 * @retval false
 * Otherwise
 */
static bool getWriteAccessFaultInfoStrSingleExt32(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                                  const uintptr_t faultAccessAddress,
                                                  const uint32_t rawIns,
                                                  const ins_info_t *const pMatchedInsInfo,
                                                  mem_fault_info_t *const pMemAccessFaultInfo)
{
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strSingleExt32.rtPos < 32U);
    uint8_t rt = (uint8_t)((rawIns & pMatchedInsInfo->fields.strSingleExt32.rtMask) >>
                           pMatchedInsInfo->fields.strSingleExt32.rtPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strSingleExt32.rnPos < 32U);
    uint8_t rn    = (uint8_t)((rawIns & pMatchedInsInfo->fields.strSingleExt32.rnMask) >>
                           pMatchedInsInfo->fields.strSingleExt32.rnPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strSingleExt32.imm8Pos < 32U);                           
    uint8_t imm8 = (uint8_t)((rawIns & pMatchedInsInfo->fields.strSingleExt32.imm8Mask) >>
                           pMatchedInsInfo->fields.strSingleExt32.imm8Pos);
    uint32_t *pRt = NULL;
    uint32_t writebackValue = (uint32_t)faultAccessAddress;

    /* fetch store data value */
    pRt = getFaultStackFrameGprPtr(pFaultStackFrame, rt);
    assert(NULL != pRt);
    if (NULL == pRt)
    {
        /* unsupported GPR -> invalid */
        return false;
    }

    /* save details about faulted store */
    pMemAccessFaultInfo->data[0U]  = *pRt;
    pMemAccessFaultInfo->dataWords = 1U;

    /* writeback? -> store details */
    if (rawIns & pMatchedInsInfo->fields.strSingleExt32.wbackMask)
    {
        pMemAccessFaultInfo->pWritebackGpr = getFaultStackFrameGprPtr(pFaultStackFrame, rn);
        assert(NULL != pMemAccessFaultInfo->pWritebackGpr);
        if (NULL == pMemAccessFaultInfo->pWritebackGpr)
        {
            /* unsupported GPR -> invalid */
            return false;
        }

        /* special handling for post-indexed accesses as fault address does not correspond with writeback value
         * in this case */
        if(!(rawIns & pMatchedInsInfo->fields.strSingleExt32.indexMask))
        {
            if(rawIns & pMatchedInsInfo->fields.strSingleExt32.addMask)
            {
                /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
                writebackValue += (uint32_t)imm8;
            }
            else 
            {
                /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
                writebackValue -= (uint32_t)imm8;
            }
        }

        pMemAccessFaultInfo->writebackValue = writebackValue;
    }

    return true;
}

/*!
 * @brief Count the bits which are 1 in val.
 *
 * @param[in] val Value to process.
 * @return The number of bits which are 1 in val.
 */
static uint8_t popcountHalfword(uint16_t val)
{
    uint8_t count = 0U;
    for (uint8_t i = 0U; i < (sizeof(val) * 8U); i++)
    {
        count += (uint8_t)(val & 0x1U);
        val >>= 1U;
    }

    return count;
}

/*!
 * @brief Attempts to get more information about a write access fault caused by a store multiple instruction.
 *
 * Applies to 16-bit store multiple instructions.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame.
 *                             The fault stack frame must have the format specified by mem_fault_info_stack_frame_t.
 * @param[in] faultAccessAddress The target address of the write which caused the fault.
 * @param[in] rawIns The instruction that caused the access fault.
 * @param[in] pMatchedInsInfo Pointer to an ins_info_t structure providing more information about the
 *                            instruction that caused the access fault.
 * @param[out] pMemAccessFaultInfo Pointer to a mem_fault_info_t object where more information about the
 *                                 fault is written.
 * @return A boolean.
 * @retval true
 * Getting more information about the fault succeeded, refer to pMemAccessFaultInfo.
 * @retval false
 * Otherwise
 */
static bool getWriteAccessFaultInfoStrMult16(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                             const uintptr_t faultAccessAddress,
                                             const uint32_t rawIns,
                                             const ins_info_t *const pMatchedInsInfo,
                                             mem_fault_info_t *const pMemAccessFaultInfo)
{
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strMult16.registersPos < 32U);
    uint8_t registerList = (uint8_t)((rawIns & pMatchedInsInfo->fields.strMult16.registersMask) >>
                                     pMatchedInsInfo->fields.strMult16.registersPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strMult16.rnPos < 32U);
    uint8_t rn =
        (uint8_t)((rawIns & pMatchedInsInfo->fields.strMult16.rnMask) >> pMatchedInsInfo->fields.strMult16.rnPos);
    uint32_t *pRn     = NULL;
    uint8_t faultWord = 0U;
    uint8_t bitCount  = 0U;

    /* fetch base register */
    pRn = getFaultStackFrameGprPtr(pFaultStackFrame, rn);
    assert(NULL != pRn);
    if (NULL == pRn)
    {
        /* unsupported GPR -> invalid */
        return false;
    }

    /* calculate word where the fault occurred */
    if (faultAccessAddress >= *pRn)
    {
        assert(((faultAccessAddress - *pRn) / sizeof(uint32_t)) < 8U);
        /* the result will always fit into an uint8_t as the fault address and base address can be at most 7 words apart
         * (instruction can store 8 words at most) */
        faultWord = (uint8_t)((faultAccessAddress - *pRn) / sizeof(uint32_t));
    }
    else 
    {
        /* should not occur, unsupported */
        return false;
    }

    /* extract access data */
    pMemAccessFaultInfo->dataWords = 0U;
    /* the fault may not have occurred at writing the first word to memory, handle such situations
     * only start at the word which caused the fault */
    for (uint8_t i = 0U; i < (sizeof(registerList) * 8U); i++)
    {
        if (registerList & (1U << i))
        {
            uint32_t *pGpr = getFaultStackFrameGprPtr(pFaultStackFrame, i);
            assert(NULL != pGpr);
            if (NULL == pGpr)
            {
                /* unsupported GPR -> invalid */
                return false;
            }

            if (bitCount >= faultWord)
            {
                /* save details about faulted store */
                pMemAccessFaultInfo->data[pMemAccessFaultInfo->dataWords] = *pGpr;
                pMemAccessFaultInfo->dataWords++;
            }

            bitCount++;
        }
    }

    pMemAccessFaultInfo->pWritebackGpr = pRn;
    /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
    pMemAccessFaultInfo->writebackValue = *pRn + (((uint32_t)bitCount) * 4U);

    return true;
}

/*!
 * @brief Attempts to get more information about a write access fault caused by a store multiple instruction.
 *
 * Applies to 32-bit store multiple instructions.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame.
 *                             The fault stack frame must have the format specified by mem_fault_info_stack_frame_t.
 * @param[in] faultAccessAddress The target address of the write which caused the fault.
 * @param[in] rawIns The instruction that caused the access fault.
 * @param[in] pMatchedInsInfo Pointer to an ins_info_t structure providing more information about the
 *                            instruction that caused the access fault.
 * @param[out] pMemAccessFaultInfo Pointer to a mem_fault_info_t object where more information about the
 *                                 fault is written.
 * @return A boolean.
 * @retval true
 * Getting more information about the fault succeeded, refer to pMemAccessFaultInfo.
 * @retval false
 * Otherwise
 */
static bool getWriteAccessFaultInfoStrMult32(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                             const uintptr_t faultAccessAddress,
                                             const uint32_t rawIns,
                                             const ins_info_t *const pMatchedInsInfo,
                                             mem_fault_info_t *const pMemAccessFaultInfo)
{
    (void)faultAccessAddress;

    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strMult32.registersPos < 32U);
    uint16_t registerList = (uint16_t)((rawIns & pMatchedInsInfo->fields.strMult32.registersMask) >>
                                       pMatchedInsInfo->fields.strMult32.registersPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strMult32.rnPos < 32U);
    uint8_t rn =
        (uint8_t)((rawIns & pMatchedInsInfo->fields.strMult32.rnMask) >> pMatchedInsInfo->fields.strMult32.rnPos);
    uint32_t *pRn                      = NULL;
    uintptr_t instructionAccessAddress = 0U;
    uint32_t offset                    = 0U;
    uint32_t writebackValue            = 0U;
    uint8_t faultWord                  = 0U;

    /* fetch base register */
    pRn = getFaultStackFrameGprPtr(pFaultStackFrame, rn);
    assert(NULL != pRn);
    if (NULL == pRn)
    {
        /* unsupported GPR -> invalid */
        return false;
    }

    offset = ((uint32_t)popcountHalfword(registerList)) * 4U;
    /* calculate access address and writeback value according to instruction */  
    if (pMatchedInsInfo->fields.strMult32.decrementBefore)
    {
        /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
        writebackValue = *pRn - offset;
        instructionAccessAddress = writebackValue;
    }
    else
    {
        instructionAccessAddress = *pRn;
        /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
        writebackValue = *pRn + offset;
    }

    /* calculate word where the fault occurred */
    if (faultAccessAddress >= instructionAccessAddress)
    {
        assert(((faultAccessAddress - instructionAccessAddress) / sizeof(uint32_t)) < 16U);
        /* the result will always fit into an uint8_t as the fault address and base address can be at most 15 words apart
         * (instruction can store 16 words at most) */
        faultWord = (uint8_t)((faultAccessAddress - instructionAccessAddress) / sizeof(uint32_t));
    }
    else 
    {
        /* should not occur, unsupported */
        return false;
    }

    /* extract access data */
    pMemAccessFaultInfo->dataWords = 0U;
    /* the fault may not have occurred at writing the first word to memory, handle such situations 
     * only start at the word which caused the fault */
    for (uint8_t i = 0U, bitCount = 0U; i < (sizeof(registerList) * 8U); i++)
    {
        if (registerList & (1U << i))
        {
            uint32_t *pGpr = getFaultStackFrameGprPtr(pFaultStackFrame, i);
            assert(NULL != pGpr);
            if (NULL == pGpr)
            {
                /* unsupported GPR -> invalid */
                return false;
            }

            if (bitCount >= faultWord)
            {
                /* save details about faulted store */
                pMemAccessFaultInfo->data[pMemAccessFaultInfo->dataWords] = *pGpr;
                pMemAccessFaultInfo->dataWords++;
            }

            bitCount++;
        }
    }

    /* writeback? -> store details */
    if (rawIns & pMatchedInsInfo->fields.strMult32.wbackMask)
    {
        pMemAccessFaultInfo->pWritebackGpr  = pRn;
        pMemAccessFaultInfo->writebackValue = writebackValue;
    }

    return true;
}

/*!
 * @brief Attempts to get more information about a write access fault caused by a STRD instruction.
 *
 * @param[in] pFaultStackFrame Pointer to the fault stack frame.
 *                             The fault stack frame must have the format specified by mem_fault_info_stack_frame_t.
 * @param[in] faultAccessAddress The target address of the write which caused the fault.
 * @param[in] rawIns The instruction that caused the access fault.
 * @param[in] pMatchedInsInfo Pointer to an ins_info_t structure providing more information about the
 *                            instruction that caused the access fault.
 * @param[out] pMemAccessFaultInfo Pointer to a mem_fault_info_t object where more information about the
 *                                 fault is written.
 * @return A boolean.
 * @retval true
 * Getting more information about the fault succeeded, refer to pMemAccessFaultInfo.
 * @retval false
 * Otherwise
 */
static bool getWriteAccessFaultInfoStrDouble(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                             const uintptr_t faultAccessAddress,
                                             const uint32_t rawIns,
                                             const ins_info_t *const pMatchedInsInfo,
                                             mem_fault_info_t *const pMemAccessFaultInfo)
{
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strDouble.rtPos < 32U);
    uint8_t rt =
        (uint8_t)((rawIns & pMatchedInsInfo->fields.strDouble.rtMask) >> pMatchedInsInfo->fields.strDouble.rtPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strDouble.rt2Pos < 32U);
    uint8_t rt2 =
        (uint8_t)((rawIns & pMatchedInsInfo->fields.strDouble.rt2Mask) >> pMatchedInsInfo->fields.strDouble.rt2Pos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strDouble.rnPos < 32U);
    uint8_t rn =
        (uint8_t)((rawIns & pMatchedInsInfo->fields.strDouble.rnMask) >> pMatchedInsInfo->fields.strDouble.rnPos);
    /* if the position of any field is greater or equal 32, then this is a configuration error */
    assert(pMatchedInsInfo->fields.strDouble.imm8Pos < 32U);                           
    uint8_t imm8 = (uint8_t)((rawIns & pMatchedInsInfo->fields.strDouble.imm8Mask) >>
                           pMatchedInsInfo->fields.strDouble.imm8Pos);
    uint32_t *pRn = NULL;
    uint32_t *pRt = NULL;    
    uint32_t *pRt2 = NULL;
    uintptr_t instructionAccessAddress = 0U;   
    uint32_t writebackValue = 0U;

    /* fetch base register */
    pRn = getFaultStackFrameGprPtr(pFaultStackFrame, rn);
    assert(NULL != pRn);
    if (NULL == pRn)
    {
        /* unsupported GPR -> invalid */
        return false;
    }
    /* fetch store data value */
    pRt = getFaultStackFrameGprPtr(pFaultStackFrame, rt);
    assert(NULL != pRt);
    if (NULL == pRt)
    {
        /* unsupported GPR -> invalid */
        return false;
    }
    pRt2 = getFaultStackFrameGprPtr(pFaultStackFrame, rt2);
    assert(NULL != pRt2);
    if (NULL == pRt2)
    {
        /* unsupported GPR -> invalid */
        return false;
    }

    /* calculate access address and writeback value according to instruction */
    if (rawIns & pMatchedInsInfo->fields.strDouble.addMask)
    {
        /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
        writebackValue = *pRn + (((uint32_t)imm8) << 2U);
    }
    else
    {
        /* no overflow or underflow checks as we want to behave like the corresponding ARM instruction */
        writebackValue = *pRn - (((uint32_t)imm8) << 2U);
    }
    instructionAccessAddress =
        (rawIns & pMatchedInsInfo->fields.strDouble.indexMask) ? writebackValue : *pRn;

    /* the fault may not have occurred at writing the first word to memory, handle such situations
     * fault occurred already on first store */
    if(faultAccessAddress == instructionAccessAddress)
    {
        /* save details about faulted store */
        pMemAccessFaultInfo->data[0U]  = *pRt;
        pMemAccessFaultInfo->data[1U]  = *pRt2;
        pMemAccessFaultInfo->dataWords = 2U;
    }
    /* fault only occurred at second store */
    else if (faultAccessAddress == (instructionAccessAddress + sizeof(uint32_t)))
    {
        /* save details about faulted store */
        pMemAccessFaultInfo->data[0U]  = *pRt2;
        pMemAccessFaultInfo->dataWords = 1U;        
    }
    else 
    {
        /* should not occur, unsupported */
        return false;
    }

    /* writeback? -> store details */
    if (rawIns & pMatchedInsInfo->fields.strDouble.wbackMask)
    {
        pMemAccessFaultInfo->pWritebackGpr  = pRn;
        pMemAccessFaultInfo->writebackValue = writebackValue;
    }

    return true;
}

bool MEM_FAULT_INFO_GetWriteAccessFaultInformation(mem_fault_info_stack_frame_t *const pFaultStackFrame,
                                                   const uintptr_t faultAccessAddress,
                                                   mem_fault_info_t *const pMemAccessFaultInfo)
{
    assert((NULL != pFaultStackFrame) && (NULL != pMemAccessFaultInfo));

    const ins_info_t *pMatchedInsInfo = NULL;
    uint32_t rawIns                   = 0U;
    bool ret                          = false;

    /* is it a store instruction? */
    if (!disassembleStore((uint16_t *)pFaultStackFrame->pc, &pMatchedInsInfo, &rawIns))
    {
        return false;
    }

    /* prepare information about memory access fault */
    /* convert to representation as in memory (internally we use a different representation) */
    pMemAccessFaultInfo->ins            = (4U == pMatchedInsInfo->width) ? (((rawIns & 0xFFFFU) << 16U) | (rawIns >> 16U)) : rawIns;
    pMemAccessFaultInfo->insWidth       = pMatchedInsInfo->width;
    pMemAccessFaultInfo->address        = faultAccessAddress;
    pMemAccessFaultInfo->accessSize     = pMatchedInsInfo->accessSize;
    pMemAccessFaultInfo->dataWords      = 0U;
    pMemAccessFaultInfo->pWritebackGpr  = NULL;
    pMemAccessFaultInfo->writebackValue = 0U;

    switch (pMatchedInsInfo->fieldsFormat)
    {
        case kInsFieldFormatStrSingle:
            ret = getWriteAccessFaultInfoStrSingle(pFaultStackFrame, faultAccessAddress, rawIns, pMatchedInsInfo,
                                                   pMemAccessFaultInfo);
            break;

        case kInsFieldFormatStrSingleExt32:
            ret = getWriteAccessFaultInfoStrSingleExt32(pFaultStackFrame, faultAccessAddress, rawIns, pMatchedInsInfo,
                                                        pMemAccessFaultInfo);
            break;

        case kInsFieldFormatStrMult16:
            ret = getWriteAccessFaultInfoStrMult16(pFaultStackFrame, faultAccessAddress, rawIns, pMatchedInsInfo,
                                                   pMemAccessFaultInfo);
            break;

        case kInsFieldFormatStrMult32:
            ret = getWriteAccessFaultInfoStrMult32(pFaultStackFrame, faultAccessAddress, rawIns, pMatchedInsInfo,
                                                   pMemAccessFaultInfo);
            break;

        case kInsFieldFormatStrDouble:
            ret = getWriteAccessFaultInfoStrDouble(pFaultStackFrame, faultAccessAddress, rawIns, pMatchedInsInfo,
                                                   pMemAccessFaultInfo);
            break;

        default:
            ret = false;
    }

    return ret;
}

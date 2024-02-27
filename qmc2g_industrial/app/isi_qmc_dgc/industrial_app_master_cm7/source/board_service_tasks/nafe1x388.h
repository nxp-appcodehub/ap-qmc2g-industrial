/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _NAFE1X388_H_
#define _NAFE1X388_H_

#include <assert.h>
#include <stdint.h>
#include "fsl_flexio_spi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Instruction Commands */
#define NAFE_CMD_CH0 0x0000
#define NAFE_CMD_CH1 0x0001
#define NAFE_CMD_CH2 0x0002
#define NAFE_CMD_CH3 0x0003
#define NAFE_CMD_CH4 0x0004
#define NAFE_CMD_CH5 0x0005
#define NAFE_CMD_CH6 0x0006
#define NAFE_CMD_CH7 0x0007
#define NAFE_CMD_CH8 0x0008
#define NAFE_CMD_CH9 0x0009
#define NAFE_CMD_CH10 0x000A
#define NAFE_CMD_CH11 0x000B
#define NAFE_CMD_CH12 0x000C
#define NAFE_CMD_CH13 0x000D
#define NAFE_CMD_CH14 0x000E
#define NAFE_CMD_CH15 0x000F
#define NAFE_CMD_ABORT 0x0010
#define NAFE_CMD_END 0x0011
#define NAFE_CMD_CLEAR_ALRM 0x0012
#define NAFE_CMD_CLEAR_DATA 0x0013
#define NAFE_CMD_RESET 0x0014
#define NAFE_CMD_CLEAR_REG 0x0015
#define NAFE_CMD_RELOAD 0x0016

#define NAFE_CMD_SS 0x2000
#define NAFE_CMD_SC 0x2001
#define NAFE_CMD_MM 0x2002
#define NAFE_CMD_MC 0x2003
#define NAFE_CMD_MS 0x2004
#define NAFE_CMD_BURST_DATA 0x2005
#define NAFE_CMD_CALC_CRC_CONFIG 0x2006
#define NAFE_CMD_CALC_CRC_COEF 0x2007
#define NAFE_CMD_CALC_CRC_FAC 0x2008

/* 16-bit User Registers */

#define NAFE_REG_CH_CONFIG0 0x20
#define NAFE_REG_CH_CONFIG1 0x21
#define NAFE_REG_CH_CONFIG2 0x22
#define NAFE_REG_CH_CONFIG3 0x23
#define NAFE_REG_CH_CONFIG4 0x24
#define NAFE_REG_CRC_CONF_REGS 0x25
#define NAFE_REG_CRC_COEF_REGS 0x26
#define NAFE_REG_CRC_TRIM_REGS 0x27
#define NAFE_REG_GPI_DATA 0x29
#define NAFE_REG_GPIO_CONFIG0 0x2A
#define NAFE_REG_GPIO_CONFIG1 0x2B
#define NAFE_REG_GPIO_CONFIG2 0x2C
#define NAFE_REG_GPI_EDGE_POS 0x2D
#define NAFE_REG_GPI_EDGE_NEG 0x2E
#define NAFE_REG_GPO_DATA 0x2F

#define NAFE_REG_SYS_CONFIG0 0x30
#define NAFE_REG_SYS_STATUS0 0x31
#define NAFE_REG_GLOBAL_ALAM_ENABLE 0x32
#define NAFE_REG_GLOBAL_ALARM_INTERRUPT 0x33
#define NAFE_REG_DIE_TEMP 0x34
#define NAFE_REG_CH_STATUS0 0x35
#define NAFE_REG_CH_STATUS1 0x36
#define NAFE_REG_THRS_TEMP 0x37

#define NAFE_REG_PN2 0x7C
#define NAFE_REG_PN1 0x7D
#define NAFE_REG_PN0 0x7E
#define NAFE_REG_SERIAL1 0xAE
#define NAFE_REG_SERIAL0 0xAF

#define NAFE_REG_CRC_TRIM_INT 0x7F

/* 24-bit User Registers */

#define NAFE_REG_CH_DATA0 0x40
#define NAFE_REG_CH_DATA1 0x41
#define NAFE_REG_CH_DATA2 0x42
#define NAFE_REG_CH_DATA3 0x43
#define NAFE_REG_CH_DATA4 0x44
#define NAFE_REG_CH_DATA5 0x45
#define NAFE_REG_CH_DATA6 0x46
#define NAFE_REG_CH_DATA7 0x47
#define NAFE_REG_CH_DATA8 0x49
#define NAFE_REG_CH_DATA9 0x49
#define NAFE_REG_CH_DATA10 0x4A
#define NAFE_REG_CH_DATA11 0x4B
#define NAFE_REG_CH_DATA12 0x4C
#define NAFE_REG_CH_DATA13 0x4D
#define NAFE_REG_CH_DATA14 0x4E
#define NAFE_REG_CH_DATA15 0x4F

#define NAFE_REG_CH_CONFIG5_0 0x50
#define NAFE_REG_CH_CONFIG5_1 0x51
#define NAFE_REG_CH_CONFIG5_2 0x52
#define NAFE_REG_CH_CONFIG5_3 0x53
#define NAFE_REG_CH_CONFIG5_4 0x54
#define NAFE_REG_CH_CONFIG5_5 0x55
#define NAFE_REG_CH_CONFIG5_6 0x56
#define NAFE_REG_CH_CONFIG5_7 0x57
#define NAFE_REG_CH_CONFIG5_8 0x59
#define NAFE_REG_CH_CONFIG5_9 0x59
#define NAFE_REG_CH_CONFIG5_10 0x5A
#define NAFE_REG_CH_CONFIG5_11 0x5B
#define NAFE_REG_CH_CONFIG5_12 0x5C
#define NAFE_REG_CH_CONFIG5_13 0x5D
#define NAFE_REG_CH_CONFIG5_14 0x5E
#define NAFE_REG_CH_CONFIG5_15 0x5F

#define NAFE_REG_CH_CONFIG6_0 0x60
#define NAFE_REG_CH_CONFIG6_1 0x61
#define NAFE_REG_CH_CONFIG6_2 0x62
#define NAFE_REG_CH_CONFIG6_3 0x63
#define NAFE_REG_CH_CONFIG6_4 0x64
#define NAFE_REG_CH_CONFIG6_5 0x65
#define NAFE_REG_CH_CONFIG6_6 0x66
#define NAFE_REG_CH_CONFIG6_7 0x67
#define NAFE_REG_CH_CONFIG6_8 0x69
#define NAFE_REG_CH_CONFIG6_9 0x69
#define NAFE_REG_CH_CONFIG6_10 0x6A
#define NAFE_REG_CH_CONFIG6_11 0x6B
#define NAFE_REG_CH_CONFIG6_12 0x6C
#define NAFE_REG_CH_CONFIG6_13 0x6D
#define NAFE_REG_CH_CONFIG6_14 0x6E
#define NAFE_REG_CH_CONFIG6_15 0x6F

#define NAFE_REG_GAIN_COEF0 0x80
#define NAFE_REG_GAIN_COEF1 0x81
#define NAFE_REG_GAIN_COEF2 0x82
#define NAFE_REG_GAIN_COEF3 0x83
#define NAFE_REG_GAIN_COEF4 0x84
#define NAFE_REG_GAIN_COEF5 0x85
#define NAFE_REG_GAIN_COEF6 0x86
#define NAFE_REG_GAIN_COEF7 0x87
#define NAFE_REG_GAIN_COEF8 0x89
#define NAFE_REG_GAIN_COEF9 0x89
#define NAFE_REG_GAIN_COEF10 0x8A
#define NAFE_REG_GAIN_COEF11 0x8B
#define NAFE_REG_GAIN_COEF12 0x8C
#define NAFE_REG_GAIN_COEF13 0x8D
#define NAFE_REG_GAIN_COEF14 0x8E
#define NAFE_REG_GAIN_COEF15 0x8F

#define NAFE_REG_OFFSET_COEF0 0x90
#define NAFE_REG_OFFSET_COEF1 0x91
#define NAFE_REG_OFFSET_COEF2 0x92
#define NAFE_REG_OFFSET_COEF3 0x93
#define NAFE_REG_OFFSET_COEF4 0x94
#define NAFE_REG_OFFSET_COEF5 0x95
#define NAFE_REG_OFFSET_COEF6 0x96
#define NAFE_REG_OFFSET_COEF7 0x97
#define NAFE_REG_OFFSET_COEF8 0x99
#define NAFE_REG_OFFSET_COEF9 0x99
#define NAFE_REG_OFFSET_COEF10 0x9A
#define NAFE_REG_OFFSET_COEF11 0x9B
#define NAFE_REG_OFFSET_COEF12 0x9C
#define NAFE_REG_OFFSET_COEF13 0x9D
#define NAFE_REG_OFFSET_COEF14 0x9E
#define NAFE_REG_OFFSET_COEF15 0x9F

#define NAFE_REG_OPT_COEF0 0xA0
#define NAFE_REG_OPT_COEF1 0xA1
#define NAFE_REG_OPT_COEF2 0xA2
#define NAFE_REG_OPT_COEF3 0xA3
#define NAFE_REG_OPT_COEF4 0xA4
#define NAFE_REG_OPT_COEF5 0xA5
#define NAFE_REG_OPT_COEF6 0xA6
#define NAFE_REG_OPT_COEF7 0xA7
#define NAFE_REG_OPT_COEF8 0xA9
#define NAFE_REG_OPT_COEF9 0xA9
#define NAFE_REG_OPT_COEF10 0xAA
#define NAFE_REG_OPT_COEF11 0xAB
#define NAFE_REG_OPT_COEF12 0xAC
#define NAFE_REG_OPT_COEF13 0xAD
#define NAFE_REG_OPT_COEF14 0xAE
#define NAFE_REG_OPT_COEF15 0xAF

#define MAX_LOGICAL_CHANNELS 16u

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

typedef enum
{
    kNafeSampleMode_none = 0u,
    kNafeSampleMode_scsrBlock,
    kNafeSampleMode_sccrBlock,
    kNafeSampleMode_mcmrBlock,
    kNafeSampleMode_mccrBlock,
	kNafeSampleMode_mcsrBlock
} NAFE_sampleMode_t;

typedef enum
{
    kNafeRegAccess_write = 0u,
    kNafeRegAccess_read = 1u
} NAFE_regAccessType_t;

typedef enum
{
    kNafeRegDataSize_16bits = 16u,
    kNafeRegDataSize_24bits = 24u
} NAFE_regDataSize_t;

typedef enum
{
    kNafeTrigger_spiCmd = 0u,
    kNafeTrigger_syncPin
} NAFE_triggerMode_t;

typedef enum
{
    kNafeAdcResolution_16bits = 1u,
    kNafeAdcResolution_24bits = 0u
} NAFE_adcResolution_t;

typedef enum
{
    kNafeReadyPinSeqMode_onConversion = 0u,
    kNafeReadyPinSeqMode_onSequencer
} NAFE_readyPinSeqMode_t;

typedef enum
{
    kNafeHvInputPos_gnd = 0u,
    kNafeHvInputPos_ai1p,
    kNafeHvInputPos_ai2p,
    kNafeHvInputPos_ai3p,
    kNafeHvInputPos_ai4p,
    kNafeHvInputPos_refcalH,
    kNafeHvInputPos_refcalL,
    kNafeHvInputPos_aicom,
    kNafeHvInputPos_vexc
} NAFE_hvInputPos_t;

typedef enum
{
    kNafeHvInputNeg_gnd = 0u,
    kNafeHvInputNeg_ai1n,
    kNafeHvInputNeg_ai2n,
    kNafeHvInputNeg_ai3n,
    kNafeHvInputNeg_ai4n,
    kNafeHvInputNeg_refcalH,
    kNafeHvInputNeg_refcalL,
    kNafeHvInputNeg_aicom,
    kNafeHvInputNeg_vexc
} NAFE_hvInputNeg_t;

typedef enum
{
    kNafeLvInput_halfRef_halfRef = 0u,
    kNafeLvInput_gpio0_gpio1,
    kNafeLvInput_halfRefCoarse_halfRef,
    kNafeLvInput_vadd_halfRef,
    kNafeLvInput_vhdd_halfRef,
    kNafeLvInput_halfRef_vhss
} NAFE_lvInput_t;

typedef enum
{
    kNafeInputSel_lvsig = 0u,
    kNafeInputSel_hvsig
} NAFE_inputSel_t;

typedef enum
{
    kNafeChnGain_0p2x = 0u,
    kNafeChnGain_0p4x,
    kNafeChnGain_0p8x,
    kNafeChnGain_1x,
    kNafeChnGain_2x,
    kNafeChnGain_4x,
    kNafeChnGain_8x,
    kNafeChnGain_16x,
} NAFE_chnGain_t;

typedef enum
{
    kNafeAdcSinc_sinc4 = 0u,
    kNafeAdcSinc_sinc4_sinc1,
    kNafeAdcSinc_sinc4_sinc2,
    kNafeAdcSinc_sinc4_sinc3,
    kNafeAdcSinc_sinc4_sinc4,
    kNafeAdcSinc_sinc4_val_5,
    kNafeAdcSinc_sinc4_val_6,
    kNafeAdcSinc_sinc4_val_7,
} NAFE_adcSinc_t;

typedef enum
{
    kNafeAdcSettling_singleCycle = 0u,
    kNafeAdcSettling_normal,
} NAFE_adcSettling_t;

typedef struct
{
	NAFE_triggerMode_t triggerMode;
    NAFE_adcResolution_t adcResolutionCode;
    NAFE_readyPinSeqMode_t readyPinSeqMode;
    uint16_t enabledChnMask;
} NAFE_sysConfig_t;

typedef struct
{
    uint16_t chnIndex;
    NAFE_inputSel_t inputSel;
    NAFE_hvInputPos_t hvAip;
    NAFE_hvInputNeg_t hvAin;
    NAFE_lvInput_t lv;
    NAFE_chnGain_t gain;
    uint32_t dataRateCode;
    NAFE_adcSinc_t adcSinc;
    uint32_t chDelayCode;
    NAFE_adcSettling_t adcSettling;
} NAFE_chnConfig_t;

typedef struct
{
    uint16_t devAddr;
    uint16_t currentChnIndex;
    NAFE_sysConfig_t *sysConfig;
    NAFE_chnConfig_t *chConfig;
    NAFE_sampleMode_t currentSampleMode;
    void *halHdl;
} NAFE_devHdl_t;

typedef struct
{
    NAFE_sampleMode_t sampleMode;
    void *pResult;                                  /* Result pointer. */
    uint32_t chnAmt;                                /* Channel sample amount.For MCMR and MCCR non-Block. */
    uint32_t chnSampleCnt;                          /* Channel sample counter.For MCMR and MCCR non-Block. */
    uint32_t contSampleAmt;                         /* Continuous sample amount. For SCCR and MCCR modes. */
    uint32_t contSampleCnt;                         /* Continuous sample counter. For SCCR and MCCR modes. */
    uint16_t requestedChn;                          /* Channel to read from. For SCCR and SCSR modes. */
    NAFE_regDataSize_t adcResolutionBits;
} NAFE_xferHdl_t;

/*******************************************************************************
 * APIs
 ******************************************************************************/

/*!
 * @brief Initializes the AFE.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 * @retval Returns status success/fail.
 */
status_t NAFE_init(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Starts the requested mode of operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
status_t NAFE_startSample(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

#endif /* #ifndef _NAFE1X388_H_ */

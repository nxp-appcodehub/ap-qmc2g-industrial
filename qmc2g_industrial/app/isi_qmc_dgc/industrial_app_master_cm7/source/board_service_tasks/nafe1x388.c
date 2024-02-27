/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "nafe1x388.h"
#include "nafe_hal_flexio.h"
#include "fsl_debug_console.h"

#define SYS_STATUS0_SINGLE_CH_ACTIVE_MASK 0x8000
#define SYS_STATUS0_MULTI_CHANNEL_ACTIVE_MASK 0x4000
#define SYS_STATUS0_CHIP_READY_MASK 0x2000
#define SYS_STATUS0_ADC_CONV_CH_ACTIVE_MASK 0x00F0
#define NAFE_READING_DELAY 40u
#define ATTEMPTS_BEFORE_TIMEOUT 20u

/*!
 * @brief Resets the AFE.
 *
 * @param[in, out] devHdl Handle for the device structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_resetDevice(NAFE_devHdl_t *devHdl);

/*!
 * @brief Generates a combined command for accessing registers, including the device address, W/R access type, and register address.
 *
 * @param[in] devAddr Device address.
 * @param[in] regAddr Register address.
 * @param[in] accessType Access type (Write or Read).
 * @param[out] combCmd Pointer to the memory where the combined command should be stored.
 *
 */
static void NAFE_combCmdForAccessRegs(uint16_t devAddr, uint16_t regAddr,
                            NAFE_regAccessType_t accessType, uint32_t *combCmd);

/*!
 * @brief Generates combined command for instruction commands, including the device address and instruction commands.
 *
 * @param[in] devAddr Device address.
 * @param[in] cmd Command for the AFE.
 * @param[out] combCmd Pointer to the memory where the combined command should be stored.
 *
 */
static void NAFE_combCmdForInstructionCmds(uint16_t devAddr, uint32_t cmd,
                            uint32_t *combCmd);

/*!
 * @brief Writes into a register.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] regAddr Address of the destination register.
 * @param[in] data Data to write.
 * @param[in] regSize Size of the destination register (16/24 bits).
 *
 * @retval Returns status.
 */
static inline status_t NAFE_writeRegBlock(NAFE_devHdl_t *devHdl, uint16_t regAddr,
                                      uint32_t data, NAFE_regDataSize_t regSize);

/*!
 * @brief Reads from a register.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] regAddr Address of the destination register.
 * @param[out] data Pointer to where the data should be stored.
 * @param[in] regSize Size of the destination register (16/24 bits).
 *
 * @retval Returns status.
 */
static inline status_t NAFE_readRegBlock(NAFE_devHdl_t *devHdl, uint16_t regAddr,
                                     uint32_t *data, NAFE_regDataSize_t regSize);

/*!
 * @brief Sends a command to the AFE.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] cmd The command to send.
 *
 * @retval Returns status.
 */
static inline status_t NAFE_sendCmdBlock(NAFE_devHdl_t *devHdl, uint32_t cmd);

/*!
 * @brief Triggers the sample gathering.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_triggerSample(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Terminates the continuous sample sequence.
 *
 * @param[in, out] devHdl Handle for the device structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_terminateContiSample(NAFE_devHdl_t *devHdl);

/*!
 * @briefConverts the result array from 16/24-bit signed int to double.
 *
 * @param[in, out] pData Pointer to the result array.
 * @param[in] chGain Channel gain.
 * @param[in] dataSizeInBits Length of frame in btis.
 * @param[in] len Amount of frames.
 *
 */
static void NAFE_formatResultArray(void *pData, NAFE_chnGain_t chGain, NAFE_regDataSize_t dataSizeInBits, uint32_t len);

/*!
 * @brief Activates the specified channel.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] chnIndex Index of the channel to activate.
 *
 * @retval Returns status.
 */
static status_t NAFE_setChannel(NAFE_devHdl_t *devHdl, uint16_t chnIndex);

/*!
 * @brief Starts the blocking SCSR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_scsrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Starts the blocking SCCR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_sccrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Starts the blocking MCSR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_mcsrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Starts the blocking MCMR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_mcmrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Starts the blocking MCCR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_mccrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl);

/*!
 * @brief Initializes the AFE.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status success/fail.
 */
status_t NAFE_init(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
    uint32_t u32Tmp = 0;
    uint32_t chnMask, chnMaskCount;
    status_t status = kStatus_Fail;
    uint32_t sys_status0_reg = 0x0000;

    if(devHdl->sysConfig->adcResolutionCode == kNafeAdcResolution_24bits)
    {
        xferHdl->adcResolutionBits = kNafeRegDataSize_24bits;
    }
    else if(devHdl->sysConfig->adcResolutionCode == kNafeAdcResolution_16bits)
    {
        xferHdl->adcResolutionBits = kNafeRegDataSize_16bits;
    }

    /* Reset the device. */
    status = NAFE_resetDevice(devHdl);
    if (status != kStatus_Success)
    {
    	return status;
    }

	status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, &sys_status0_reg, kNafeRegDataSize_16bits); /* Read the SYS_STATUS0 register */
	if ((status == kStatus_Fail) || (!(sys_status0_reg & SYS_STATUS0_CHIP_READY_MASK)))  /* Check if the CHIP is ready (powered on) */
    {
    	return kStatus_Fail;
    }

	if (xferHdl->chnAmt > MAX_LOGICAL_CHANNELS)
	{
		return kStatus_Fail;
	}

    /* Check enabled channel mask. */
    chnMask = devHdl->sysConfig->enabledChnMask;
    chnMaskCount = 0u;
    while (chnMask && chnMaskCount < 16u)
    {
        if (chnMask & 0x01)
        {
            chnMaskCount++;
        }
        chnMask >>= 1u;
    }
    if (chnMaskCount != xferHdl->chnAmt)
    {
        return kStatus_Fail;
    }

    /* Configure channel registers. */
    for (int32_t i = 0; i < xferHdl->chnAmt; i++)
    {
        status = NAFE_setChannel(devHdl, devHdl->chConfig[i].chnIndex);
        if (status != kStatus_Success)
        {
        	return status;
        }

        status = NAFE_writeRegBlock(devHdl, NAFE_REG_CH_CONFIG0, \
				  0x0000\
				| (devHdl->chConfig[i].hvAip << 12u) \
				| (devHdl->chConfig[i].hvAin << 8u) \
				| (devHdl->chConfig[i].gain << 5u) \
				| (devHdl->chConfig[i].inputSel << 4u), \
				  kNafeRegDataSize_16bits);
        if (status != kStatus_Success)
        {
        	return status;
        }

 	 	status = NAFE_writeRegBlock(devHdl, NAFE_REG_CH_CONFIG1, \
 	 			  0x0000\
                | devHdl->chConfig[i].dataRateCode << 3u \
                | devHdl->chConfig[i].adcSinc, \
                  kNafeRegDataSize_16bits);
 	    if (status != kStatus_Success)
 	    {
 	    	return status;
 	    }

        status = NAFE_writeRegBlock(devHdl, NAFE_REG_CH_CONFIG2, \
        		  0x0000\
                | devHdl->chConfig[i].chDelayCode << 10u \
                | devHdl->chConfig[i].adcSettling << 9u, \
                  kNafeRegDataSize_16bits);
        if (status != kStatus_Success)
        {
        	return status;
        }

    }

    /* Configure the system config register. */
    status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_CONFIG0, &u32Tmp, kNafeRegDataSize_16bits);
    if (status != kStatus_Success)
    {
    	return status;
    }


    u32Tmp &= ~(1u << 4u);
    u32Tmp |= devHdl->sysConfig->readyPinSeqMode << 4u;
    u32Tmp &= ~(1u << 5u);
    u32Tmp |= devHdl->sysConfig->triggerMode << 5u;
    u32Tmp &= ~(1u << 14u);
    u32Tmp |= devHdl->sysConfig->adcResolutionCode << 14;
    status = NAFE_writeRegBlock(devHdl, NAFE_REG_SYS_CONFIG0, u32Tmp, kNafeRegDataSize_16bits);
    if (status != kStatus_Success)
    {
    	return status;
    }


    /* Configure the channel config 4 register. */
    status = NAFE_writeRegBlock(devHdl, NAFE_REG_CH_CONFIG4, \
                       devHdl->sysConfig->enabledChnMask, kNafeRegDataSize_16bits);
    if (status != kStatus_Success)
    {
    	return status;
    }


    return kStatus_Success;
}

/*!
 * @brief Starts the requested mode of operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
status_t NAFE_startSample(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
	status_t status = kStatus_Fail;
    uint32_t sys_status0_reg = 0x0000;
    xferHdl->contSampleCnt= 0u;                                    /* For continuous sample modes. */
    xferHdl->chnSampleCnt = 0u;                                    /* For multi-channel sample modes. */

	if ((xferHdl->chnAmt > MAX_LOGICAL_CHANNELS) || (xferHdl->requestedChn >= MAX_LOGICAL_CHANNELS))
	{
		return kStatus_Fail;
	}

	status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, &sys_status0_reg, kNafeRegDataSize_16bits);  /* Read the SYS_STATUS0 register */
	if ((status == kStatus_Fail) || (!(sys_status0_reg & SYS_STATUS0_CHIP_READY_MASK))) /* Check if the CHIP is ready (powered on) */
    {
    	return kStatus_Fail;
    }

    switch(xferHdl->sampleMode)
    {
        case kNafeSampleMode_scsrBlock:
            if (devHdl->currentChnIndex != xferHdl->requestedChn)
            {
                /* Check if the requested channel is enabled */
                assert(devHdl->sysConfig->enabledChnMask & (1 << xferHdl->requestedChn));
                status = NAFE_setChannel(devHdl, xferHdl->requestedChn);

                if (status != kStatus_Success)
                {
                	return status;
                }
            }
            status = NAFE_scsrBlock(devHdl, xferHdl);
            break;
        case kNafeSampleMode_sccrBlock:
            if (devHdl->currentChnIndex != xferHdl->requestedChn)
            {
                /* Check if the requested channel is enabled */
                assert(devHdl->sysConfig->enabledChnMask & (1 << xferHdl->requestedChn));
                status = NAFE_setChannel(devHdl, xferHdl->requestedChn);

                if (status != kStatus_Success)
                {
                	return status;
                }
            }
            status = NAFE_sccrBlock(devHdl, xferHdl);
            break;
        case kNafeSampleMode_mcsrBlock:
        	if (devHdl->currentSampleMode != xferHdl->sampleMode)
        	{
                /* Check if the requested channel is enabled */
                assert(devHdl->sysConfig->enabledChnMask & (1 << xferHdl->requestedChn));
                status = NAFE_setChannel(devHdl, xferHdl->requestedChn);

                if (status != kStatus_Success)
                {
                	return status;
                }
        	}
        	status = NAFE_mcsrBlock(devHdl, xferHdl);
            break;
        case kNafeSampleMode_mcmrBlock:
        	status = NAFE_mcmrBlock(devHdl, xferHdl);
            break;
        case kNafeSampleMode_mccrBlock:
        	status = NAFE_mccrBlock(devHdl, xferHdl);
            break;
        default:
        	status = kStatus_Fail;
            break;
    }

    return status;
}

/*!
 * @brief Resets the AFE.
 *
 * @param[in, out] devHdl Handle for the device structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_resetDevice(NAFE_devHdl_t *devHdl)
{
	status_t status = NAFE_sendCmdBlock(devHdl, NAFE_CMD_RESET);
    NAFE_HAL_delay(50u * 1000u);

    return status;
}


/*!
 * @brief Generates a combined command for accessing registers, including the device address, W/R access type, and register address.
 *
 * @param[in] devAddr Device address.
 * @param[in] regAddr Register address.
 * @param[in] accessType Access type (Write or Read).
 * @param[out] combCmd Pointer to the memory where the combined command should be stored.
 *
 */
static void NAFE_combCmdForAccessRegs(uint16_t devAddr, uint16_t regAddr,
                            NAFE_regAccessType_t accessType, uint32_t *combCmd)
{
    assert(devAddr == 0u || devAddr == 1u);

    *combCmd = devAddr;
    *combCmd <<= 15u;               /*bit15: hardware address */
    *combCmd |= accessType << 14u;  /*bit14: W = 0, R = 1 */
    *combCmd |= regAddr << 1u;      /*bit1~bit13: register address */
}

/*!
 * @brief Generates combined command for instruction commands, including the device address and instruction commands.
 *
 * @param[in] devAddr Device address.
 * @param[in] cmd Command for the AFE.
 * @param[out] combCmd Pointer to the memory where the combined command should be stored.
 *
 */
static void NAFE_combCmdForInstructionCmds(uint16_t devAddr, uint32_t cmd,
                            uint32_t *combCmd)
{
    assert(devAddr == 0u || devAddr == 1u);

    *combCmd = devAddr;
    *combCmd <<= 15u;               /*bit15: hardware address */
 /* *combCmd |= 0 << 14u;  		      bit14: always W = 0 */
    *combCmd |= cmd << 1u;          /*bit1~bit13: instruction command */
}

/*!
 * @brief Writes into a register.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] regAddr Address of the destination register.
 * @param[in] data Data to write.
 * @param[in] regSize Size of the destination register (16/24 bits).
 *
 * @retval Returns status.
 */
static inline status_t NAFE_writeRegBlock(NAFE_devHdl_t *devHdl, uint16_t regAddr,
                                      uint32_t data, NAFE_regDataSize_t regSize)
{
    uint32_t combCmd = 0;

    NAFE_combCmdForAccessRegs(devHdl->devAddr, regAddr, kNafeRegAccess_write, &combCmd);
    return NAFE_HAL_writeRegBlock((FLEXIO_SPI_Type *) devHdl->halHdl, combCmd, data, (uint16_t) regSize);
}

/*!
 * @brief Reads from a register.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] regAddr Address of the destination register.
 * @param[in] data Pointer to where the data should be stored.
 * @param[in] regSize Size of the destination register (16/24 bits).
 *
 * @retval Returns status.
 */
static inline status_t NAFE_readRegBlock(NAFE_devHdl_t *devHdl, uint16_t regAddr,
                                     uint32_t *data, NAFE_regDataSize_t regSize)
{
    uint32_t combCmd = 0;

    NAFE_combCmdForAccessRegs(devHdl->devAddr, regAddr, kNafeRegAccess_read, &combCmd);
    return NAFE_HAL_readRegBlock((FLEXIO_SPI_Type *) devHdl->halHdl, combCmd, data, (uint16_t) regSize);
}

/*!
 * @brief Sends a command to the AFE.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] cmd The command to send.
 *
 * @retval Returns status.
 */
static inline status_t NAFE_sendCmdBlock(NAFE_devHdl_t *devHdl, uint32_t cmd)
{
    uint32_t combCmd = 0;

    NAFE_combCmdForInstructionCmds(devHdl->devAddr, cmd, &combCmd);
    return NAFE_HAL_sendCmdBlock((FLEXIO_SPI_Type *) devHdl->halHdl, combCmd);
}

/*!
 * @brief Triggers the sample gathering.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_triggerSample(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
    uint32_t cmd = 0;
    status_t status = kStatus_Fail;

    switch(xferHdl->sampleMode)
    {
        case kNafeSampleMode_scsrBlock:
        case kNafeSampleMode_mcsrBlock:
            cmd = xferHdl->sampleMode - kNafeSampleMode_scsrBlock + NAFE_CMD_SS;

            /* Send the command. */
            status = NAFE_sendCmdBlock(devHdl, cmd);

            devHdl->currentSampleMode = xferHdl->sampleMode;
            break;

        case kNafeSampleMode_sccrBlock:
        case kNafeSampleMode_mcmrBlock:
        case kNafeSampleMode_mccrBlock:
            if (devHdl->currentSampleMode != xferHdl->sampleMode)
            {
                cmd = xferHdl->sampleMode - kNafeSampleMode_scsrBlock + NAFE_CMD_SS;

                /* Send the command. */
                status = NAFE_sendCmdBlock(devHdl, cmd);
                
                devHdl->currentSampleMode = xferHdl->sampleMode;
            }
            break;

        default:
            cmd = xferHdl->sampleMode - kNafeSampleMode_scsrBlock + NAFE_CMD_SS;

            /* Send the command. */
            status = NAFE_sendCmdBlock(devHdl, cmd);

            devHdl->currentSampleMode = xferHdl->sampleMode;
            break;
    }

    return status;
}

/*!
 * @brief Terminates the continuous sample sequence.
 *
 * @param[in, out] devHdl Handle for the device structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_terminateContiSample(NAFE_devHdl_t *devHdl)
{
	status_t status = NAFE_sendCmdBlock(devHdl, NAFE_CMD_ABORT);

    NAFE_HAL_delay(2u);

    devHdl->currentSampleMode = kNafeSampleMode_none;
    return status;
}

/*!
 * @brief Converts the result array from 24-bit signed int to double.
 *
 * @param[out] pData Pointer to the result array.
 * @param[in] chGain Channel gain.
 * @param[in] dataSizeInBits Length of frame in btis.
 * @param[in] len Amount of frames.
 *
 */
static void NAFE_formatResultArray(void *pData, NAFE_chnGain_t chGain, NAFE_regDataSize_t dataSizeInBits, uint32_t len)
{
    uint32_t *u32Result = (uint32_t *)pData + len - 1u;
    uint16_t *u16Result = (uint16_t *)pData + len - 1u;
    double *f64Result = (double *)pData + len - 1u;
    int32_t s32Tmp;
    uint32_t u32Tmp;

    float PGA = 0.0;

    /* NAFE13388 supports all 7 gain options, NAFE11388 supports options 0, 1 and 2 only - choosing other options results in option 0 (kNafeChnGain_0p2x) */
    switch(chGain)
    {
    	case kNafeChnGain_0p2x:
    		PGA = 0.2;
    		break;

    	case kNafeChnGain_0p4x:
    		PGA = 0.4;
    		break;

    	case kNafeChnGain_0p8x:
    		PGA = 0.8;
    		break;

    	case kNafeChnGain_1x:
    		PGA = 1.0;
    		break;

    	case kNafeChnGain_2x:
			PGA = 2.0;
			break;

    	case kNafeChnGain_4x:
    		PGA = 4.0;
    		break;

    	case kNafeChnGain_8x:
    		PGA = 8.0;
    		break;

    	case kNafeChnGain_16x:
    		PGA = 16.0;
    		break;

    	default:
    		PGA = 0.2;
    		break;
    }

    if(dataSizeInBits == kNafeRegDataSize_24bits)
    {
        while (len > 0)
        {
            *u32Result <<= 8u;
            s32Tmp = (int32_t)*u32Result;
            s32Tmp >>= 8u;
            *f64Result = (s32Tmp * 10.0) / (0x1000000 * PGA);
            u32Result--;
            f64Result--;
            len--;
        }
    }
    else if(dataSizeInBits == kNafeRegDataSize_16bits)
    {
        while (len > 0)
        {
            u32Tmp = *u16Result;
            u32Tmp <<= 16u;
            s32Tmp = (int32_t)u32Tmp;
            s32Tmp >>= 8u;
            *f64Result = (s32Tmp * 10.0) / (0x1000000 * PGA);
            u16Result--;
            f64Result--;
            len--;
        }
    }
}

/*!
 * @brief Activates the specified channel.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] chnIndex Index of the channel to activate.
 *
 * @retval Returns status.
 */
static status_t NAFE_setChannel(NAFE_devHdl_t *devHdl, uint16_t chnIndex)
{
	if (chnIndex >= MAX_LOGICAL_CHANNELS)
	{
		return kStatus_Fail;
	}

	status_t status = NAFE_sendCmdBlock(devHdl, chnIndex);
    devHdl->currentChnIndex = chnIndex;

    return status;
}

/*!
 * @brief Starts the blocking SCSR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_scsrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
	uint8_t attempts = 0;
	status_t status = kStatus_Fail;
    uint32_t sys_status0_reg = 0x0000;

    status = NAFE_triggerSample(devHdl, xferHdl);

	if (status != kStatus_Success)
	{
		return status;
	}

    /* Wait for the conversion to be done. */
    while (1)
    {
    	attempts++;
    	status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
              &sys_status0_reg, kNafeRegDataSize_16bits);

    	if (status != kStatus_Success)
    	{
    		return status;
    	}

        if (!(sys_status0_reg & SYS_STATUS0_SINGLE_CH_ACTIVE_MASK)) /* Check the status of the ADC conversion */
        {
            break;
        }
        

    	if (attempts >= ATTEMPTS_BEFORE_TIMEOUT)
    	{
    		return kStatus_Fail;
    	}

        sys_status0_reg = 0x0000; /* Clear the status variable */
        NAFE_HAL_delay(NAFE_READING_DELAY); /* Delay at least 40us */
    }

    /* Read the result. */
    status = NAFE_readRegBlock(devHdl, (uint16_t) NAFE_REG_CH_DATA0 + devHdl->currentChnIndex, \
    		(uint32_t*) xferHdl->pResult, xferHdl->adcResolutionBits);

	if (status != kStatus_Success)
	{
		PRINTF("SCSR ERR\r\n");
		return status;
	}

    NAFE_formatResultArray(xferHdl->pResult, devHdl->chConfig->gain, xferHdl->adcResolutionBits, 1u);
    /* Do not call NAFE_terminateContiSample() or rest devHdl->currentSampleMode,
       So that keeping the sample status in SCSR. And enable the sync pin trigger
       next if required.
    */
    return kStatus_Success;
}

/*!
 * @brief Starts the blocking SCCR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_sccrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
	void *pResult;
	status_t status = NAFE_triggerSample(devHdl, xferHdl);

    if (status != kStatus_Success)
    {
    	return status;
    }

    while (xferHdl->contSampleCnt < xferHdl->contSampleAmt)
    {
        if (xferHdl->adcResolutionBits == kNafeRegDataSize_24bits)
        {
            pResult = (uint32_t *)xferHdl->pResult + xferHdl->contSampleCnt;
        }
        else if (xferHdl->adcResolutionBits == kNafeRegDataSize_16bits)
        {
            pResult = (uint16_t *)xferHdl->pResult + xferHdl->contSampleCnt;
        }
        else
        {
        	/* Only 16 and 24 bit resolution is supported */
        	return kStatus_Fail;
        }

        /* Delay at least 40us - Without the DRDY signal, there is no way of checking that the ADC   *
         * has finished the current conversion. The ADC wil be busy for the whole SCCR operation.    *
         * 40us should be enough but in case of bad data, this value should be increased.            */
        NAFE_HAL_delay(NAFE_READING_DELAY);

        /* Read the result. */
        status = NAFE_readRegBlock(devHdl, (uint16_t) NAFE_REG_CH_DATA0 + devHdl->currentChnIndex, \
        		(uint32_t*) pResult, xferHdl->adcResolutionBits);

        if (status != kStatus_Success)
        {
        	return status;
        }

        xferHdl->contSampleCnt++;
    }

    status = NAFE_terminateContiSample(devHdl);

    if (status != kStatus_Success)
    {
    	return status;
    }

    NAFE_formatResultArray(xferHdl->pResult, devHdl->chConfig->gain, xferHdl->adcResolutionBits, xferHdl->contSampleAmt);

    return kStatus_Success;
}

/*!
 * @brief Starts the blocking MCSR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_mcsrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
	status_t status = kStatus_Fail;
    uint32_t sys_status0_reg = 0x0000;
    uint8_t adc_conv_ch = 0;

    status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
                  &sys_status0_reg, kNafeRegDataSize_16bits);

    if (status != kStatus_Success)
    {
    	return status;
    }

    adc_conv_ch = (sys_status0_reg & SYS_STATUS0_ADC_CONV_CH_ACTIVE_MASK) >> 4;

    status = NAFE_triggerSample(devHdl, xferHdl);

    if (status != kStatus_Success)
    {
    	return status;
    }

    /* Wait for the conversion to be done. */
    while (1)
    {
        status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
              &sys_status0_reg, kNafeRegDataSize_16bits);

        if (status != kStatus_Success)
        {
        	return status;
        }

        if (!(sys_status0_reg & SYS_STATUS0_SINGLE_CH_ACTIVE_MASK)) /* Check the status of the ADC conversion */
        {
            break;
        }

        sys_status0_reg = 0x0000; /* Clear the status variable */
    }

    /* Read the result. */
    status = NAFE_readRegBlock(devHdl, (uint16_t) NAFE_REG_CH_DATA0 + adc_conv_ch, \
    		(uint32_t*) xferHdl->pResult, xferHdl->adcResolutionBits);

    if (status != kStatus_Success)
    {
    	return status;
    }

    NAFE_formatResultArray(xferHdl->pResult, devHdl->chConfig->gain, xferHdl->adcResolutionBits, 1u);
    /* Do not call NAFE_terminateContiSample() or rest devHdl->currentSampleMode,
       So that keeping the sample status in SCSR. And enable the sync pin trigger
       next if required.
    */

    return kStatus_Success;
}

/*!
 * @brief Starts the blocking MCMR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_mcmrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
	void *pResult;
	status_t status = NAFE_triggerSample(devHdl, xferHdl);

    if (status != kStatus_Success)
    {
    	return status;
    }

    if (devHdl->sysConfig->readyPinSeqMode == kNafeReadyPinSeqMode_onSequencer)
    {
        /* Wait for all conversions to be done. */
        uint32_t sys_status0_reg = 0x0000;
        while (1)
        {
            status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
                  &sys_status0_reg, kNafeRegDataSize_16bits);
            
            if (status != kStatus_Success)
            {
            	return status;
            }

            if ((!(sys_status0_reg & SYS_STATUS0_MULTI_CHANNEL_ACTIVE_MASK)) && (!(sys_status0_reg & SYS_STATUS0_SINGLE_CH_ACTIVE_MASK)))
            {
                break;
            }
            
            sys_status0_reg = 0x0000; /* Clear the status variable */
            NAFE_HAL_delay(NAFE_READING_DELAY); /* Delay at least 20us */
        }
    }
    for (uint32_t i = 0u; i < xferHdl->chnAmt; i++)
    {
        if (devHdl->sysConfig->readyPinSeqMode == kNafeReadyPinSeqMode_onConversion)
        {
            /* Wait for each conversion to be done. */
            uint32_t sys_status0_reg = 0x0000;
            while (1)
            {
                status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
                      &sys_status0_reg, kNafeRegDataSize_16bits);
                
                if (status != kStatus_Success)
                {
                	return status;
                }

                if (!(sys_status0_reg & SYS_STATUS0_SINGLE_CH_ACTIVE_MASK)) /* Check the status of the ADC conversion */
                {
                    break;
                }
                
                sys_status0_reg = 0x0000; /* Clear the status variable */
                NAFE_HAL_delay(NAFE_READING_DELAY); /* Delay at least 40us */
            }
        }

        if (xferHdl->adcResolutionBits == kNafeRegDataSize_24bits)
        {
            pResult = (uint32_t *)xferHdl->pResult + i;
        }
        else if (xferHdl->adcResolutionBits == kNafeRegDataSize_16bits)
        {
            pResult = (uint16_t *)xferHdl->pResult + i;
        }
        else
        {
        	/* Only 16 and 24 bit resolution is supported */
        	return kStatus_Fail;
        }

        /* Read the result. */
        status = NAFE_readRegBlock(devHdl, (uint16_t) NAFE_REG_CH_DATA0 + devHdl->chConfig[i].chnIndex, \
        		(uint32_t*) pResult, xferHdl->adcResolutionBits);

        if (status != kStatus_Success)
        {
        	return status;
        }

        devHdl->currentChnIndex = i;
    }

    NAFE_formatResultArray(xferHdl->pResult, devHdl->chConfig->gain, xferHdl->adcResolutionBits, xferHdl->chnAmt);

    /* Do not call NAFE_terminateContiSample() or rest devHdl->currentSampleMode,
       So that keeping the sample status in MCMR. And enable the sync pin trigger
       next if required (for Rev.B).
    */

    return kStatus_Success;
}

/*!
 * @brief Starts the blocking MCCR operation.
 *
 * @param[in, out] devHdl Handle for the device structure.
 * @param[in] xferHdl Handle for the transfer structure.
 *
 * @retval Returns status.
 */
static status_t NAFE_mccrBlock(NAFE_devHdl_t *devHdl, NAFE_xferHdl_t *xferHdl)
{
	void *pResult;
	status_t status = NAFE_triggerSample(devHdl, xferHdl);

    if (status != kStatus_Success)
    {
    	return status;
    }

    while (xferHdl->contSampleCnt < xferHdl->contSampleAmt)
    {
        if (devHdl->sysConfig->readyPinSeqMode == kNafeReadyPinSeqMode_onSequencer)
        {
        	/* Wait for all conversions to be done. */
            uint32_t sys_status0_reg = 0x0000;
            while (1)
            {
                status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
                      &sys_status0_reg, kNafeRegDataSize_16bits);

                if (status != kStatus_Success)
                {
                	return status;
                }

                if (!(sys_status0_reg & SYS_STATUS0_SINGLE_CH_ACTIVE_MASK)) /* Check the status of the ADC conversion */
                {
                	break;
                }

                sys_status0_reg = 0x0000; /* Clear the status variable */
                NAFE_HAL_delay(NAFE_READING_DELAY); /* Delay at least 40us */
            }
        }

        for (uint32_t i = 0u; i < xferHdl->chnAmt; i++)
        {
            if (devHdl->sysConfig->readyPinSeqMode == kNafeReadyPinSeqMode_onConversion)
            {
                /* Wait for each conversion to be done. */
                uint32_t sys_status0_reg = 0x0000;
                while (1)
                {
                    status = NAFE_readRegBlock(devHdl, NAFE_REG_SYS_STATUS0, \
                          &sys_status0_reg, kNafeRegDataSize_16bits);
                    
                    if (status != kStatus_Success)
                    {
                    	return status;
                    }

                    if (!(sys_status0_reg & SYS_STATUS0_SINGLE_CH_ACTIVE_MASK)) /* Check the status of the ADC conversion */
                    {
                        break;
                    }

                    sys_status0_reg = 0x0000; /* Clear the status variable */
                    NAFE_HAL_delay(NAFE_READING_DELAY); /* Delay at least 40us */
                }
            }

            if (xferHdl->adcResolutionBits == kNafeRegDataSize_24bits)
            {
                pResult = (uint32_t *)xferHdl->pResult + xferHdl->contSampleAmt * i + xferHdl->contSampleCnt;
            }
            else if (xferHdl->adcResolutionBits == kNafeRegDataSize_16bits)
            {
                pResult = (uint16_t *)xferHdl->pResult + xferHdl->contSampleAmt * i + xferHdl->contSampleCnt;
            }
            else
            {
            	/* Only 16 and 24 bit resolution is supported */
            	return kStatus_Fail;
            }

            /* It seems that if there is no delay here, sometimes the first reading on a channel is wrong. */
            NAFE_HAL_delay(NAFE_READING_DELAY);

            /* Read the result. */
            status = NAFE_readRegBlock(devHdl, (uint16_t) NAFE_REG_CH_DATA0 + devHdl->chConfig[i].chnIndex, \
                (uint32_t*) pResult, xferHdl->adcResolutionBits);

            if (status != kStatus_Success)
            {
            	return status;
            }

            devHdl->currentChnIndex = i;
        }
        xferHdl->contSampleCnt++;
    }

    status = NAFE_terminateContiSample(devHdl);

    if (status != kStatus_Success)
    {
    	return status;
    }

    NAFE_formatResultArray(xferHdl->pResult, devHdl->chConfig->gain, xferHdl->adcResolutionBits, xferHdl->contSampleAmt * xferHdl->chnAmt);

    return kStatus_Success;
}

/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "nafe_hal_flexio.h"
#include "pin_mux.h"
#include "fsl_gpio.h"

#define COMMAND_BITCOUNT 16

/*!
 * @brief Initiates the FLEXIO SPI Master.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in, out] masterConfig Pointer to the master configuration structure.
 * @param[in] srcClock_Hz Source clock frequency.
 */
void NAFE_HAL_init(FLEXIO_SPI_Type *halHdl, flexio_spi_master_config_t *masterConfig, uint32_t srcClock_Hz)
{
    FLEXIO_SPI_MasterInit(halHdl, masterConfig, srcClock_Hz);
}

/*!
 * @brief Writes data into a AFE register.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in] cmd The command for the AFE.
 * @param[in] data The data that should be written to the register.
 * @param[in] dataBits The length of the data in bits.
 *
 * @retval Returns status based on the success of the write operation.
 */
status_t NAFE_HAL_writeRegBlock(FLEXIO_SPI_Type *halHdl, uint32_t cmd,
                            uint32_t data, uint16_t dataBits)
{
	uint32_t tmpRetVal = 0;
    uint16_t bitcount = (uint16_t) COMMAND_BITCOUNT + dataBits;
    uint64_t combinedCmdData = 0;
    status_t statusCheck = kStatus_Fail;
#if SPI_RETRY_TIMES
    uint32_t waitTimes;
#endif /* #if SPI_RETRY_TIMES */
    combinedCmdData = cmd << dataBits | data;

    helper_FLEXIO_SPI_SetSCKTimerBitcount(halHdl, bitcount);

	statusCheck = helper_FLEXIO_SPI_WriteBlockingWithBitcount(halHdl, FLEXIO_SPI_DIRECTION, &combinedCmdData, 1, bitcount);
	if (statusCheck != kStatus_Success)
	{
		return statusCheck;
	}

	/* By writing into the MOSI register, the slave automatically writes into the MISO register. This value must be read but should be ignored. */
#if SPI_RETRY_TIMES
        waitTimes = SPI_RETRY_TIMES;
        while ((0U == (FLEXIO_SPI_GetStatusFlags(halHdl) & (uint32_t)kFLEXIO_SPI_RxBufferFullFlag)) &&
               (0U != --waitTimes))
    	{
    		SDK_DelayAtLeastUs(SPI_WAIT_US, BOARD_BOOTCLOCKRUN_CORE_CLOCK);
    	}

        if (waitTimes == 0U)
        {
            return kStatus_FLEXIO_SPI_Timeout;
        }
#else
        while (0U == (FLEXIO_SPI_GetStatusFlags(base) & (uint32_t)kFLEXIO_SPI_RxBufferFullFlag))
        {}
#endif /* #if SPI_RETRY_TIMES */

    tmpRetVal = (halHdl)->flexioBase->SHIFTBUFBIS[(halHdl)->shifterIndex[1]];
    (void) tmpRetVal;

    return kStatus_Success;
}

/*!
 * @brief Sends a command to read data from a AFE register, then reads and returns the response.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in] cmd The command for the AFE.
 * @param[in] data Pointer to the memory where the read data will be stored.
 * @param[in] dataBits The expected length of the data in bits.
 *
 * @retval Returns status based on the success of the write and read operations.
 */
status_t NAFE_HAL_readRegBlock(FLEXIO_SPI_Type *halHdl, uint32_t cmd,
                           uint32_t *data, uint16_t dataBits)
{
	uint16_t bitcount = COMMAND_BITCOUNT + dataBits;
	status_t statusCheck = kStatus_Fail;

    helper_FLEXIO_SPI_SetSCKTimerBitcount(halHdl, bitcount);

    statusCheck = helper_FLEXIO_SPI_WriteBlockingWithBitcount(halHdl, FLEXIO_SPI_DIRECTION, &cmd, 1, COMMAND_BITCOUNT);
	if (statusCheck != kStatus_Success)
	{
		return statusCheck;
	}

	statusCheck = NAFE_HAL_readSpiRxd(halHdl, data, dataBits, 1u);
    return statusCheck;
}

/*!
 * @brief Sends a command to the AFE.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in] cmd The command for the AFE.
 *
 * @retval Returns status based on the success of the write operation.
 */
status_t NAFE_HAL_sendCmdBlock(FLEXIO_SPI_Type *halHdl, uint32_t cmd)
{
	uint32_t tmpRetVal = 0;
	status_t statusCheck = kStatus_Fail;
#if SPI_RETRY_TIMES
    uint32_t waitTimes;
#endif /* #if SPI_RETRY_TIMES */

	helper_FLEXIO_SPI_SetSCKTimerBitcount(halHdl, COMMAND_BITCOUNT);

	statusCheck = helper_FLEXIO_SPI_WriteBlockingWithBitcount(halHdl, FLEXIO_SPI_DIRECTION, &cmd, 1, COMMAND_BITCOUNT);
	if (statusCheck != kStatus_Success)
	{
		return statusCheck;
	}

	/* By writing into the MOSI register, the slave automatically writes into the MISO register. This value must be read but should be ignored. */
#if SPI_RETRY_TIMES
        waitTimes = SPI_RETRY_TIMES;
        while ((0U == (FLEXIO_SPI_GetStatusFlags(halHdl) & (uint32_t)kFLEXIO_SPI_RxBufferFullFlag)) &&
               (0U != --waitTimes))
    	{
    		SDK_DelayAtLeastUs(SPI_WAIT_US, BOARD_BOOTCLOCKRUN_CORE_CLOCK);
    	}

        if (waitTimes == 0U)
        {
            return kStatus_FLEXIO_SPI_Timeout;
        }
#else
        while (0U == (FLEXIO_SPI_GetStatusFlags(base) & (uint32_t)kFLEXIO_SPI_RxBufferFullFlag))
        {}
#endif /* #if SPI_RETRY_TIMES */
    tmpRetVal = (halHdl)->flexioBase->SHIFTBUFBIS[(halHdl)->shifterIndex[1]];
    (void) tmpRetVal;

    return kStatus_Success;
}

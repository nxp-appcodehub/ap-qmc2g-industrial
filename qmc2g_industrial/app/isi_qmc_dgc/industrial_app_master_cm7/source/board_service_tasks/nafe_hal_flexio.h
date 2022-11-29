/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _NAFE_HAL_FLEXIO_H_
#define _NAFE_HAL_FLEXIO_H_

#include <nafe1x388.h>
#include <stdint.h>
#include <stdbool.h>

#include "fsl_device_registers.h"
#include "board.h"
#include "fsl_flexio.h"
#include "fsl_flexio_spi.h"
#include "helper_flexio_spi.h"
#include "fsl_common.h"
#include "clock_config.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
typedef enum
{
    kNafeHalInterruptType_frameComplete = 1u << 0u,
    kNafeHalInterruptType_receiveData = 1u << 1u,
} NAFE_HAL_spiStatus_t;


/*******************************************************************************
 * APIs
 ******************************************************************************/
/*!
 * @brief Initiates the FLEXIO SPI Master.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in, out] masterConfig Pointer to the master configuration structure.
 * @param[in] srcClock_Hz Source clock frequency.
 */
void NAFE_HAL_init(FLEXIO_SPI_Type *halHdl, flexio_spi_master_config_t *masterConfig, uint32_t srcClock_Hz);

/*!
 * @brief Delays at least by the specified amount of us.
 *
 * @param[in] us Microseconds to delay for.
 */
static inline void NAFE_HAL_delay(uint32_t us)
{
    SDK_DelayAtLeastUs(us, BOARD_BOOTCLOCKRUN_CORE_CLOCK);
}

/*!
 * @brief Reads data from the MISO buffer.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[out] data Pointer to the memory where the data should be stored.
 * @param[in] dataBits The length of the data in bits.
 * @param[in] len Amount of frames to read.
 *
 * @retval Returns status based on the success of read operations.
 */
static inline status_t NAFE_HAL_readSpiRxd(FLEXIO_SPI_Type *halHdl, void *data,
                                       uint32_t dataBits, uint32_t len)
{
#if SPI_RETRY_TIMES
    uint32_t waitTimes;
#endif /* #if SPI_RETRY_TIMES */

	while (len--)
	{
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

		uint32_t tmpData = helper_FLEXIO_SPI_ReadData_32bit(halHdl, FLEXIO_SPI_DIRECTION);

		if(dataBits == 24u)
		{
			if (FLEXIO_SPI_DIRECTION == kFLEXIO_SPI_MsbFirst)
			{
				*((uint32_t *) data++) = (uint32_t) tmpData;
			}
			else
			{
				uint32_t reverse = (uint32_t) tmpData;
				reverse = (reverse & 0xFFFF0000) >> 16 | (reverse & 0x0000FFFF) << 16;
				reverse = (reverse & 0xFF00FF00) >> 8 | (reverse & 0x00FF00FF) << 8;
				reverse = (reverse & 0xF0F0F0F0) >> 4 | (reverse & 0x0F0F0F0F) << 4;
				reverse = (reverse & 0xCCCCCCCC) >> 2 | (reverse & 0x33333333) << 2;
				reverse = (reverse & 0xAAAAAAAA) >> 1 | (reverse & 0x55555555) << 1;

				*((uint32_t *) data++) = reverse;
			}
		}
		else if(dataBits == 16u)
		{
			if (FLEXIO_SPI_DIRECTION == kFLEXIO_SPI_MsbFirst)
			{
				*((uint16_t *) data++) = (uint16_t) tmpData;
			}
			else
			{
				uint16_t reverse = (uint16_t) tmpData;
				reverse = (reverse & 0xFF00) >> 8 | (reverse & 0x00FF) << 8;
				reverse = (reverse & 0xF0F0) >> 4 | (reverse & 0x0F0F) << 4;
				reverse = (reverse & 0xCCCC) >> 2 | (reverse & 0x3333) << 2;
				reverse = (reverse & 0xAAAA) >> 1 | (reverse & 0x5555) << 1;

				*((uint16_t *) data++) = reverse;
			}
		}
		else if(dataBits == 8u)
		{
			if (FLEXIO_SPI_DIRECTION == kFLEXIO_SPI_MsbFirst)
			{
				*((uint8_t *) data++) = (uint8_t) tmpData;
			}
			else
			{
				uint8_t reverse = (uint8_t) tmpData;
				reverse = (reverse & 0xF0) >> 4 | (reverse & 0x0F) << 4;
				reverse = (reverse & 0xCC) >> 2 | (reverse & 0x33) << 2;
				reverse = (reverse & 0xAA) >> 1 | (reverse & 0x55) << 1;

				*((uint8_t *) data++) = reverse;
			}
		}
		else
		{
			return kStatus_Fail;
		}
	}

	return kStatus_Success;
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
                            uint32_t data, uint32_t dataBits);

/*!
 * @brief Sends a command to read data from a AFE register, then reads and returns the response.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in] cmd The command for the AFE.
 * @param[in] data Pointer to the memory where the read data will be stored.
 * @param[in] dataBits The expected length of the data in bits.
 *
 * @retval Returns status based on the success of the write operation.
 */
status_t NAFE_HAL_readRegBlock(FLEXIO_SPI_Type *halHdl, uint32_t cmd,
                           uint32_t *data, uint32_t dataBits);

/*!
 * @brief Sends a command to the AFE.
 *
 * @param[in, out] halHdl Handle for the NAFE_HAL structure.
 * @param[in] cmd The command for the AFE.
 *
 * @retval Returns status based on the success of the write operation.
 */
status_t NAFE_HAL_sendCmdBlock(FLEXIO_SPI_Type *halHdl, uint32_t cmd);

#endif /* #ifndef _NAFE_HAL_FLEXIO_H_ */

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _HELPER_FLEXLIO_SPI_H_
#define _HELPER_FLEXLIO_SPI_H_

#include <stdint.h>

#include "fsl_device_registers.h"
#include "fsl_flexio_spi.h"

/* FlexIO SPI SDK drivers don't support 32-bit data transmission.       *
 * This is a macro used for a workaround solution in the QMC2G project. */
#define FLEXIO_SPI_32BITMODE 0x20U
#define SPI_BAUDRATE				2000000
#define FLEXIO_SPI_DIRECTION kFLEXIO_SPI_MsbFirst

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
typedef enum
{
	GD3000_ON_PSB,
	AFE_ON_PSB,
	ABS_ENC_ON_PSB,
	AFE_ON_DB
} spi_selection_t;

/*******************************************************************************
 * APIs
 ******************************************************************************/
/*!
 * @brief Sets TIMCTL of the FLEXIO SPI.
 *
 * @param[in] choice SPI device.
 * @param[in] baseArr FLEXIO SPI base array.
 */
void helper_FLEXIO_SPI_Set_TIMCTL(spi_selection_t choice, FLEXIO_SPI_Type *baseArr);

/*!
 * @brief Returns the contents of the MISO buffer.
 *
 * @param[in, out] base Pointer to the FLEXIO_SPI_Type structure.
 * @param[in] direction Direction from which the bits were fed into the buffer.
 */
static inline uint32_t helper_FLEXIO_SPI_ReadData_32bit(FLEXIO_SPI_Type *base, flexio_spi_shift_direction_t direction)
{
    if (direction == kFLEXIO_SPI_MsbFirst)
    {
        return (uint32_t)(base->flexioBase->SHIFTBUFBIS[base->shifterIndex[1]]);
    }
    else
    {
        return (uint32_t)(base->flexioBase->SHIFTBUF[base->shifterIndex[1]]);
    }
}

/*!
 * @brief Sets the SCK timer of the FLEXIO SPI to transfer a specified amount of bits.
 *
 * @note Some protocols need to write, wait and read and require the CS signal to be LOW
 *      for the whole time. Since CS is usually configured to be triggered by SCK,
 * 	    keeping the SCK running is one way of adhering to such protocols.
 *
 * @param[in, out] base Pointer to the FLEXIO_SPI_Type structure.
 * @param[in] bitcount Amount of bits to transfer
 */
void helper_FLEXIO_SPI_SetSCKTimerBitcount(FLEXIO_SPI_Type *base, uint32_t bitcount);

/*!
 * @brief Sends a buffer of data frames.
 *
 * @note This function blocks using the polling method until all bytes have been sent.
 * @note This is an extension of the FLEXIO_SPI_WriteBlocking from fsl_flexio_spi that
 * 		allows for the frames to have varying lengths of up to 32 bits.
 *
 * @param[in, out] base Pointer to the FLEXIO_SPI_Type structure.
 * @param[in] direction Shift direction of MSB first or LSB first.
 * @param[in] buffer The data frames to send.
 * @param[in] framesAmnt The amount of data frames to send.
 * @param[in] bitcount The length of a frame in bits
 * @retval kStatus_Success Successfully create the handle.
 * @retval kStatus_FLEXIO_SPI_Timeout The transfer timed out and was aborted.
 */
status_t helper_FLEXIO_SPI_WriteBlockingWithBitcount(FLEXIO_SPI_Type *base, flexio_spi_shift_direction_t direction,
														const void *buffer, uint32_t framesAmt, uint16_t bitcount);

#endif /* #ifndef _HELPER_FLEXLIO_SPI_H_ */

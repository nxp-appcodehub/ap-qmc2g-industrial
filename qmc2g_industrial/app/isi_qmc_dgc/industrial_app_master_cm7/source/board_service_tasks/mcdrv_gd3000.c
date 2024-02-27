/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "mcdrv_gd3000.h"
#include "fsl_common.h"
#include "clock_config.h"
#include "helper_flexio_spi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define GD3000_CMD_BITCOUNT    8u


/*******************************************************************************
 * Prototypes
 ******************************************************************************/



/*!
 * @brief  Initializes a GD3000 structure
 *
 * @param[in, out] gdPointer   Pointer to the structure.
 * @param[in, out] base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_init(GD3000_t *gdPointer, FLEXIO_SPI_Type *base)
{
	uint8_t ui8IntMask0 = 0;
	uint8_t ui8IntMask1 = 0;

	gdPointer->ui8ResetRequest = 0;

	/* Clear interrupt flags, set deadtime to 0 */
	GD3000_clearFlags(base);
	GD3000_setZeroDeadtime(base);

	/* Enable interrupts */
	gdPointer->sConfigData.uInterruptEnable.B.overTemp = 1;
	gdPointer->sConfigData.uInterruptEnable.B.desaturation = 1;
	gdPointer->sConfigData.uInterruptEnable.B.framingErr = 1;
	gdPointer->sConfigData.uInterruptEnable.B.lowVls = 1;
	gdPointer->sConfigData.uInterruptEnable.B.overCurrent = 1;
    gdPointer->sConfigData.uInterruptEnable.B.phaseErr = 1;
    gdPointer->sConfigData.uInterruptEnable.B.resetEvent = 0;
    gdPointer->sConfigData.uInterruptEnable.B.writeErr = 1;

    ui8IntMask0 = SET_INT_MASK0_CMD|(gdPointer->sConfigData.uInterruptEnable.ui8R & 0x0F);
    ui8IntMask1 = SET_INT_MASK1_CMD|((gdPointer->sConfigData.uInterruptEnable.ui8R >> 4) & 0x0F);

    GD3000_sendData(base, ui8IntMask0);
    GD3000_sendData(base, ui8IntMask1);

    /* Setup mode */
    gdPointer->sConfigData.uMode.B.disableDesat = 0;
    gdPointer->sConfigData.uMode.B.enableFullOn = 0;
    gdPointer->sConfigData.uMode.B.lock = 0;
    GD3000_sendData(base, SET_MODE_CMD|(gdPointer->sConfigData.uMode.ui8R & 0x0F));

    /* Clear interrupt flags */
    GD3000_clearFlags(base);
}

/*!
 * @brief  Reads GD3000 status registers
 *
 * @param[in, out]  gdPointer   Pointer to the structure.
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_getSR(GD3000_t *gdPointer, FLEXIO_SPI_Type *base)
{
	/* Status Register 0 reading = 0x00 */
	GD3000_sendData(base, READ_STATUS0_CMD);
	gdPointer->sStatus.uStatus0.ui8R = GD3000_sendData(base, READ_STATUS0_CMD);

	/* Status Register 1 reading = 0x01 */
	GD3000_sendData(base, READ_STATUS1_CMD);
	gdPointer->sStatus.uStatus1.ui8R = GD3000_sendData(base, READ_STATUS1_CMD);

	/* Status Register 2 reading = 0x02 */
	GD3000_sendData(base, READ_STATUS2_CMD);
	gdPointer->sStatus.uStatus2.ui8R = GD3000_sendData(base, READ_STATUS2_CMD);

	/* Status Register 3 reading = 0x03 */
	GD3000_sendData(base, READ_STATUS3_CMD);
	gdPointer->sStatus.ui8Status3 = GD3000_sendData(base, READ_STATUS3_CMD);
}

/*!
 * @brief  Clears GD3000 status flags
 *
 * @param  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_clearFlags(FLEXIO_SPI_Type *base)
{
	GD3000_sendData(base, CLR_INT0_CMD|0xF);
	GD3000_sendData(base, CLR_INT1_CMD|0xF);
}

/*!
 * @brief  Zeroes the GD3000 dead time
 *
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_setZeroDeadtime(FLEXIO_SPI_Type *base)
{
	GD3000_sendData(base, SET_DEADTIME_CMD & 0xFE);
}

/*!
 * @brief  Sends data to GD3000 of FLEXIO SPI
 *
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 * @param[in]  ui8Data	    Data to send.
 */
uint8_t GD3000_sendData(FLEXIO_SPI_Type *base, uint8_t ui8Data)
{
	uint8_t ui8Tmp;
#if SPI_RETRY_TIMES
	uint32_t waitTimes = 0;
#endif /* #if SPI_RETRY_TIMES */

	helper_FLEXIO_SPI_SetSCKTimerBitcount(base, GD3000_CMD_BITCOUNT);
	status_t statusCheck = helper_FLEXIO_SPI_WriteBlockingWithBitcount(base, FLEXIO_SPI_DIRECTION, &ui8Data, 1, GD3000_CMD_BITCOUNT);

	if (statusCheck != kStatus_Success)
	{
		return 0;
	}

#if SPI_RETRY_TIMES
	waitTimes = SPI_RETRY_TIMES;
	while ((0U == (FLEXIO_SPI_GetStatusFlags(base) & (uint32_t)kFLEXIO_SPI_RxBufferFullFlag)) &&
		   (0U != --waitTimes))
	{
		SDK_DelayAtLeastUs(SPI_WAIT_US, BOARD_BOOTCLOCKRUN_CORE_CLOCK);
	}

	if (waitTimes == 0U)
	{
		return 0;
	}
#else
	while (0U == (FLEXIO_SPI_GetStatusFlags(base) & (uint32_t)kFLEXIO_SPI_RxBufferFullFlag))
	{}
#endif /* #if SPI_RETRY_TIMES */

	ui8Tmp = (uint8_t) helper_FLEXIO_SPI_ReadData_32bit(base, FLEXIO_SPI_DIRECTION);

	return ui8Tmp;
}

/* End of file */

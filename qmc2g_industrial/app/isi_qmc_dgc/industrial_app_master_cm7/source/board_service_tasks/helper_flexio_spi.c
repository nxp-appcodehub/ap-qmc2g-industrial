/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "pin_mux.h"
#include "fsl_gpio.h"
#include "clock_config.h"
#include "helper_flexio_spi.h"

/*!
 * @brief Sets TIMCTL of the FLEXIO SPI.
 *
 * @param[in] choice SPI device.
 * @param[in] baseArr FLEXIO SPI base array.
 */
void helper_FLEXIO_SPI_Set_TIMCTL(spi_selection_t choice, FLEXIO_SPI_Type *baseArr)
{

	switch(choice)
	{
	case GD3000_ON_PSB:
		for (int i = 0; i < 4; i++)
		{
			baseArr[i].flexioBase->TIMCTL[baseArr[i].timerIndex[1]] = FLEXIO_TIMCTL_PINCFG(kFLEXIO_PinConfigOutput)|
															FLEXIO_TIMCTL_PINSEL(baseArr[i].CSnPinIndex)|
															FLEXIO_TIMCTL_TIMOD(kFLEXIO_TimerModeSingle16Bit)|
															FLEXIO_TIMCTL_PINPOL(kFLEXIO_PinActiveLow);
		}
		break;
	case AFE_ON_PSB:
	case ABS_ENC_ON_PSB:
	case AFE_ON_DB:
		for (int i = 0; i < 4; i++)
		{
			baseArr[i].flexioBase->TIMCTL[baseArr[i].timerIndex[1]] = FLEXIO_TIMCTL_TRGSEL(FLEXIO_TIMER_TRIGGER_SEL_TIMn(baseArr[i].timerIndex[0])) |
														  FLEXIO_TIMCTL_TRGPOL(kFLEXIO_TimerTriggerPolarityActiveHigh) |
														  FLEXIO_TIMCTL_TRGSRC(kFLEXIO_TimerTriggerSourceInternal) |
														  FLEXIO_TIMCTL_PINCFG(kFLEXIO_PinConfigOutput) | FLEXIO_TIMCTL_PINSEL(baseArr[i].CSnPinIndex) |
														  FLEXIO_TIMCTL_PINPOL(kFLEXIO_PinActiveLow) | FLEXIO_TIMCTL_TIMOD(kFLEXIO_TimerModeSingle16Bit);
		}
		break;
	default:
		for (int i = 0; i < 4; i++)
		{
			baseArr[i].flexioBase->TIMCTL[baseArr[i].timerIndex[1]] = FLEXIO_TIMCTL_PINCFG(kFLEXIO_PinConfigOutput)|
															FLEXIO_TIMCTL_PINSEL(baseArr[i].CSnPinIndex)|
															FLEXIO_TIMCTL_TIMOD(kFLEXIO_TimerModeSingle16Bit)|
															FLEXIO_TIMCTL_PINPOL(kFLEXIO_PinActiveLow);
		}
		break;
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
void helper_FLEXIO_SPI_SetSCKTimerBitcount(FLEXIO_SPI_Type *base, uint32_t bitcount)
{
    uint16_t timerCmp = 0;

    timerCmp = ((uint16_t)bitcount * 2U - 1U) << 8U;
    timerCmp |= ((uint16_t) base->flexioBase->TIMCMP[base->timerIndex[0]]) & 0x00FF;

    base->flexioBase->TIMCMP[base->timerIndex[0]] = FLEXIO_TIMCMP_CMP(timerCmp);
}

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
status_t helper_FLEXIO_SPI_WriteBlockingWithBitcount(FLEXIO_SPI_Type *base,
                                  flexio_spi_shift_direction_t direction,
                                  const void *buffer,
                                  uint32_t framesAmt,
                                  uint16_t bitcount)
{
    assert(buffer != NULL);
    assert(LPSPI_TCR_FRAMESZ_MASK != 0U);
    
#if SPI_RETRY_TIMES
    uint32_t waitTimes;
#endif /* #if SPI_RETRY_TIMES */

    while (0U != framesAmt--)
    {
        /* Wait until data transfer complete. */
#if SPI_RETRY_TIMES
        waitTimes = SPI_RETRY_TIMES;
        while ((0U == (FLEXIO_SPI_GetStatusFlags(base) & (uint32_t)kFLEXIO_SPI_TxBufferEmptyFlag)) &&
               (0U != --waitTimes))
    	{
    		SDK_DelayAtLeastUs(SPI_WAIT_US, BOARD_BOOTCLOCKRUN_CORE_CLOCK);
    	}

        if (waitTimes == 0U)
        {
            return kStatus_FLEXIO_SPI_Timeout;
        }
#else
        while (0U == (FLEXIO_SPI_GetStatusFlags(base) & (uint32_t)kFLEXIO_SPI_TxBufferEmptyFlag))
        {}
#endif  /* #if SPI_RETRY_TIMES */

        if (bitcount <= 32)
        {
			if (direction == kFLEXIO_SPI_MsbFirst)
			{
				/* Shifts the frame by the required amount of bits so that the msb of the frame is at the bit-index of 31 */
				base->flexioBase->SHIFTBUFBIS[base->shifterIndex[0]] = *((const uint32_t *) buffer) << (32 - bitcount);
			}
			else
			{
				base->flexioBase->SHIFTBUF[base->shifterIndex[0]] = *((const uint32_t *) buffer);
			}

			if ((bitcount % 8) > 0)
			{
				buffer += (bitcount / 8) + 1;
			}
			else
			{
				buffer += bitcount / 8;
			}
        }
        else
        {
        	/* Larger than 32 bit frames would require shifter buffer chaining. */
        	return kStatus_Fail;
        }
    }

    return kStatus_Success;
}

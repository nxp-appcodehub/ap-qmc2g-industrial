/*
 * Copyright 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mc_hal_drivers/mcdrv_adc_imxrt117x.h>
#include "mlib.h"
#include "mc_common.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
//#define QMC1G
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Reads and calculates 3 phase samples based on SVM sector
 *
 * @param this   Pointer to the current object
 *
 * @return boot_t True when SVM sector is correct
 */
RAM_FUNC_CRITICAL bool_t MCDRV_Curr3Ph2ShGet(mcdrv_adc_t *this)
{
	bool_t bStatusPass = FALSE;
	GMCLIB_3COOR_T_F16 sIABCtemp;

    int16_t i16Rslt0;
    int16_t i16Rslt1;

    switch (*this->pui16SVMSector)
    {
        case 2:
        case 3:
            /* direct sensing of phase A and C, calculation of B */
        	i16Rslt0 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec23.pui16RsltRegPhaA))<<3);
        	i16Rslt1 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec23.pui16RsltRegPhaC))<<3);
            sIABCtemp.f16A = MLIB_Sub_F16(i16Rslt0, this->sCurrSec23.ui16OffsetPhaA)<<1;
            sIABCtemp.f16C = MLIB_Sub_F16(i16Rslt1, this->sCurrSec23.ui16OffsetPhaC)<<1;
            sIABCtemp.f16B = MLIB_Neg_F16(MLIB_AddSat_F16(sIABCtemp.f16A, sIABCtemp.f16C));
            bStatusPass = TRUE;
            break;
        case 4:
        case 5:
            /* direct sensing of phase A and B, calculation of C */
        	i16Rslt0 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec45.pui16RsltRegPhaA))<<3);
        	i16Rslt1 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec45.pui16RsltRegPhaB))<<3);
            sIABCtemp.f16A = MLIB_Sub_F16(i16Rslt0, this->sCurrSec45.ui16OffsetPhaA)<<1;
            sIABCtemp.f16B = MLIB_Sub_F16(i16Rslt1, this->sCurrSec45.ui16OffsetPhaB)<<1;
            sIABCtemp.f16C = MLIB_Neg_F16(MLIB_AddSat_F16(sIABCtemp.f16A, sIABCtemp.f16B));
            bStatusPass = TRUE;
            break;
        case 1:
        case 6:
            /* direct sensing of phase B and C, calculation of A */
        	i16Rslt0 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec16.pui16RsltRegPhaB))<<3);
        	i16Rslt1 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec16.pui16RsltRegPhaC))<<3);
            sIABCtemp.f16B = MLIB_Sub_F16(i16Rslt0, this->sCurrSec16.ui16OffsetPhaB)<<1;
            sIABCtemp.f16C = MLIB_Sub_F16(i16Rslt1, this->sCurrSec16.ui16OffsetPhaC)<<1;
            sIABCtemp.f16A = MLIB_Neg_F16(MLIB_AddSat_F16(sIABCtemp.f16B, sIABCtemp.f16C));
            bStatusPass = TRUE;
            break;
        default:
		#ifndef QMC1G
			sIABCtemp.f16A = this->psIABC->f16A;
			sIABCtemp.f16B = this->psIABC->f16B;
			sIABCtemp.f16C = this->psIABC->f16C;
		#else
			sIABCtemp.f16A = -this->psIABC->f16A;
			sIABCtemp.f16B = -this->psIABC->f16B;
			sIABCtemp.f16C = -this->psIABC->f16C;
		#endif
        	break;
    }
    
    /* pass measured phase currents to the main module structure */
#ifndef QMC1G
    this->psIABC->f16A = sIABCtemp.f16A;
    this->psIABC->f16B = sIABCtemp.f16B;
    this->psIABC->f16C = sIABCtemp.f16C;
#else
    this->psIABC->f16A = -sIABCtemp.f16A;
    this->psIABC->f16B = -sIABCtemp.f16B;
    this->psIABC->f16C = -sIABCtemp.f16C;
#endif

    return (bStatusPass);
}

/*!
 * @brief Set initial channel assignment for phase currents & DCB voltage


            TRIG0 ->  ADC1_SEG0(Ia), ADC1_SEG1(Udc)
            TRIG4 ->  ADC2_SEG0(Ib), ADC2_SEG1(Ic)

            ADC input voltage is scaled to 30/64 internally by HW. ADC reference is 1.5V, to make sure input 3.3 will be
            corresponding to 1.5V, SW needs to add another scale.

            C0 = 3.3*30/64
            C1 = 1.5/C0 = 1.5*64/(3.3*30) = 0.969697

            (ADC_RSLT<<3)*C1 will be in range of 0~32767 when input voltage is 0~3.3V


 * @param this   Pointer to the current object
 *
 * @return  True on successful assignment
 */
bool_t MCDRV_Curr3Ph2ShChanAssignInit(mcdrv_adc_t *this)
{
    bool_t bStatusPass = TRUE;

    this->a32Gain = ACC32(0.969697);
    this->ui16OffsetFiltWindow = CALIB_MA_NUM;

    /* Phase current assignment */
    if((this->psChannelAssignment->sLPADC1IA.ui16ChanNum == NOT_EXIST)&&(this->psChannelAssignment->sLPADC2IA.ui16ChanNum == NOT_EXIST))
    {
    	/* Ia is not assigned to either ADC */
		bStatusPass = FALSE;
		return bStatusPass;
    }

    if((this->psChannelAssignment->sLPADC1IB.ui16ChanNum == NOT_EXIST)&&(this->psChannelAssignment->sLPADC2IB.ui16ChanNum == NOT_EXIST))
    {
    	/* Ib is not assigned to either ADC */
		bStatusPass = FALSE;
		return bStatusPass;
    }

    if((this->psChannelAssignment->sLPADC1IC.ui16ChanNum == NOT_EXIST)&&(this->psChannelAssignment->sLPADC2IC.ui16ChanNum == NOT_EXIST))
    {
    	/* Ic is not assigned to either ADC */
		bStatusPass = FALSE;
		return bStatusPass;
    }

    if((this->psChannelAssignment->sLPADC1IA.ui16ChanNum != NOT_EXIST)&&(this->psChannelAssignment->sLPADC2IA.ui16ChanNum != NOT_EXIST))
    {
    	/* Ia is assigned to both ADCs */
    	if(this->psChannelAssignment->sLPADC1IB.ui16ChanNum != NOT_EXIST)
    	{
    		/* Ib -> ADC1 */
    		if(this->psChannelAssignment->sLPADC2IC.ui16ChanNum != NOT_EXIST)
    		{
    			/* Ic -> ADC2 */
    			this->sCurrSec16.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    			this->sCurrSec16.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);

    	    	this->sCurrSec16.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IB.ui16Cmd;
    	    	this->sCurrSec16.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IC.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IA.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IC.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IB.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IA.ui16Cmd;

    		}
    		else
    		{
    			/* Ic -> ADC1 */
    			bStatusPass = FALSE;
    			return bStatusPass;
    		}
    	}
    	else
    	{
    		/* Ib -> ADC2 */
    		if(this->psChannelAssignment->sLPADC1IC.ui16ChanNum != NOT_EXIST)
    		{
    			/* Ic -> ADC1 */
    			this->sCurrSec16.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    			this->sCurrSec16.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);

    	    	this->sCurrSec16.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IC.ui16Cmd;
    	    	this->sCurrSec16.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IB.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IC.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IA.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IA.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IB.ui16Cmd;
    		}
    		else
    		{
    			/* Ic -> ADC2 */
    			bStatusPass = FALSE;
    			return bStatusPass;
    		}
    	}
    }

    if((this->psChannelAssignment->sLPADC1IB.ui16ChanNum != NOT_EXIST)&&(this->psChannelAssignment->sLPADC2IB.ui16ChanNum != NOT_EXIST))
    {
    	/* Ib is assigned to both ADCs */
    	if(this->psChannelAssignment->sLPADC1IA.ui16ChanNum != NOT_EXIST)
    	{
    		/* Ia -> ADC1 */
    		if(this->psChannelAssignment->sLPADC2IC.ui16ChanNum != NOT_EXIST)
    		{
    			/* Ic -> ADC2 */
    			this->sCurrSec16.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    			this->sCurrSec16.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);

    	    	this->sCurrSec16.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IB.ui16Cmd;
    	    	this->sCurrSec16.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IC.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IA.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IC.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IA.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IB.ui16Cmd;

    		}
    		else
    		{
    			/* Ic -> ADC1 */
    			bStatusPass = FALSE;
    			return bStatusPass;
    		}
    	}
    	else
    	{
    		/* Ia -> ADC2 */
    		if(this->psChannelAssignment->sLPADC1IC.ui16ChanNum != NOT_EXIST)
    		{
    			/* Ic -> ADC1 */
    			this->sCurrSec16.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    			this->sCurrSec16.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);

    	    	this->sCurrSec16.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IC.ui16Cmd;
    	    	this->sCurrSec16.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IB.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IC.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IA.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IB.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IA.ui16Cmd;
    		}
    		else
    		{
    			/* Ic -> ADC2 */
    			bStatusPass = FALSE;
    			return bStatusPass;
    		}
    	}
    }

    if((this->psChannelAssignment->sLPADC1IC.ui16ChanNum != NOT_EXIST)&&(this->psChannelAssignment->sLPADC2IC.ui16ChanNum != NOT_EXIST))
    {
    	/* Ic is assigned to both ADCs */
    	if(this->psChannelAssignment->sLPADC1IB.ui16ChanNum != NOT_EXIST)
    	{
    		/* Ib -> ADC1 */
    		if(this->psChannelAssignment->sLPADC2IA.ui16ChanNum != NOT_EXIST)
    		{
    			/* Ia -> ADC2 */
    			this->sCurrSec16.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    			this->sCurrSec16.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);

    	    	this->sCurrSec16.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IB.ui16Cmd;
    	    	this->sCurrSec16.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IC.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IC.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IA.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IB.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IA.ui16Cmd;

    		}
    		else
    		{
    			/* Ia -> ADC1 */
    			bStatusPass = FALSE;
    			return bStatusPass;
    		}
    	}
    	else
    	{
    		/* Ib -> ADC2 */
    		if(this->psChannelAssignment->sLPADC1IA.ui16ChanNum != NOT_EXIST)
    		{
    			/* Ia -> ADC1 */
    			this->sCurrSec16.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    			this->sCurrSec16.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec23.pui16RsltRegPhaC = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaA = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0);
    	    	this->sCurrSec45.pui16RsltRegPhaB = (uint16_t const volatile *)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0);

    	    	this->sCurrSec16.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IC.ui16Cmd;
    	    	this->sCurrSec16.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IB.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IA.ui16Cmd;
    	    	this->sCurrSec23.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IC.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC1Seg0CmdNum = this->psChannelAssignment->sLPADC1IA.ui16Cmd;
    	    	this->sCurrSec45.ui16ADC2Seg0CmdNum = this->psChannelAssignment->sLPADC2IB.ui16Cmd;
    		}
    		else
    		{
    			/* Ia -> ADC2 */
    			bStatusPass = FALSE;
    			return bStatusPass;
    		}
    	}
    }


    /* DC bus voltage and auxiliary signal assignment */
    if(this->psChannelAssignment->sLPADC1VDcb.ui16ChanNum != NOT_EXIST)
    {
    	/* Udc -> ADC1 */
    	if(this->psChannelAssignment->sLPADC2Aux.ui16ChanNum != NOT_EXIST)
    	{
        	/* Aux -> ADC2 */
    		this->pui16RsltRegVDcb = (uint16_t const volatile *)((uint32_t)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0)+2);
        	this->ui16ADC1Seg1CmdNum = this->psChannelAssignment->sLPADC1VDcb.ui16Cmd;
        	this->pui16RsltRegAux = (uint16_t const volatile *)((uint32_t)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0)+2);
        	this->ui16ADC2Seg1CmdNum = this->psChannelAssignment->sLPADC2Aux.ui16Cmd;
    	}
    	else
    	{
    		/* Aux -> ADC1 */
			bStatusPass = FALSE;
			return bStatusPass;
    	}
    }
    else
    {
    	/* Udc -> ADC2 */
    	if(this->psChannelAssignment->sLPADC1Aux.ui16ChanNum != NOT_EXIST)
    	{
    		/* Aux -> ADC1 */
        	this->pui16RsltRegVDcb = (uint16_t const volatile *)((uint32_t)(&ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_RESULT_1_0)+2);
        	this->ui16ADC2Seg1CmdNum = this->psChannelAssignment->sLPADC2VDcb.ui16Cmd;
        	this->pui16RsltRegAux = (uint16_t const volatile *)((uint32_t)(&ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_RESULT_1_0)+2);
        	this->ui16ADC1Seg1CmdNum = this->psChannelAssignment->sLPADC1Aux.ui16Cmd;
    	}
    	else
    	{
    		/* Aux -> ADC2 */
			bStatusPass = FALSE;
			return bStatusPass;
    	}

    }

    return (bStatusPass);
}


/*!
 * @brief Set new channel assignment for next sampling based on SVM sector
 *
 * @param this   Pointer to the current object
 *
 * @return  True when SVM sector is correct
 */
RAM_FUNC_CRITICAL bool_t MCDRV_Curr3Ph2ShChanAssign(mcdrv_adc_t *this)
{
	bool_t bStatusPass = FALSE;
    
    switch (*this->pui16SVMSector)
    {
        /* direct sensing of phases A and C */
        case 2:
        case 3:
        	ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_CHAIN_1_0 &= ~ADC_ETC_TRIGn_CHAIN_1_0_CSEL0_MASK;
        	ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_CHAIN_1_0 &= ~ADC_ETC_TRIGn_CHAIN_1_0_CSEL0_MASK;

        	ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_CHAIN_1_0 |= ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(this->sCurrSec23.ui16ADC1Seg0CmdNum);
        	ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_CHAIN_1_0 |= ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(this->sCurrSec23.ui16ADC2Seg0CmdNum);
        	bStatusPass = TRUE;
            break;

        /* direct sensing of phases A and B  */
        case 4:
        case 5:
        	ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_CHAIN_1_0 &= ~ADC_ETC_TRIGn_CHAIN_1_0_CSEL0_MASK;
        	ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_CHAIN_1_0 &= ~ADC_ETC_TRIGn_CHAIN_1_0_CSEL0_MASK;

        	ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_CHAIN_1_0 |= ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(this->sCurrSec45.ui16ADC1Seg0CmdNum);
        	ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_CHAIN_1_0 |= ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(this->sCurrSec45.ui16ADC2Seg0CmdNum);
        	bStatusPass = TRUE;
            break;

        /* direct sensing of phases B and C */
        case 1:
        case 6:
        	ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_CHAIN_1_0 &= ~ADC_ETC_TRIGn_CHAIN_1_0_CSEL0_MASK;
        	ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_CHAIN_1_0 &= ~ADC_ETC_TRIGn_CHAIN_1_0_CSEL0_MASK;

        	ADC_ETC->TRIG[this->ui8MotorNum-1].TRIGn_CHAIN_1_0 |= ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(this->sCurrSec16.ui16ADC1Seg0CmdNum);
        	ADC_ETC->TRIG[this->ui8MotorNum-1+4].TRIGn_CHAIN_1_0 |= ADC_ETC_TRIGn_CHAIN_1_0_CSEL0(this->sCurrSec16.ui16ADC2Seg0CmdNum);
        	bStatusPass = TRUE;
            break;
        default:
        	break;
    }
    
    return (bStatusPass);
}

/*!
 * @brief Initializes phase current channel offset measurement
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_Curr3Ph2ShCalibInit(mcdrv_adc_t *this)
{

    /* clear offset values */
    this->sCurrSec16.ui16OffsetPhaB = 0;
    this->sCurrSec16.ui16OffsetPhaC = 0;
    this->sCurrSec23.ui16OffsetPhaA = 0;
    this->sCurrSec23.ui16OffsetPhaC = 0;
    this->sCurrSec45.ui16OffsetPhaA = 0;
    this->sCurrSec45.ui16OffsetPhaB = 0;

    this->sCurrSec16.ui16CalibPhaB = 0;
    this->sCurrSec16.ui16CalibPhaC = 0;
    this->sCurrSec23.ui16CalibPhaA = 0;
    this->sCurrSec23.ui16CalibPhaC = 0;
    this->sCurrSec45.ui16CalibPhaA = 0;
    this->sCurrSec45.ui16CalibPhaB = 0;

    /* initialize offset filters */
    this->sCurrSec16.sFiltPhaB.u16Sh = this->ui16OffsetFiltWindow;
    this->sCurrSec16.sFiltPhaC.u16Sh = this->ui16OffsetFiltWindow;
    this->sCurrSec23.sFiltPhaA.u16Sh = this->ui16OffsetFiltWindow;
    this->sCurrSec23.sFiltPhaC.u16Sh = this->ui16OffsetFiltWindow;
    this->sCurrSec45.sFiltPhaA.u16Sh = this->ui16OffsetFiltWindow;
    this->sCurrSec45.sFiltPhaB.u16Sh = this->ui16OffsetFiltWindow;

    GDFLIB_FilterMAInit_F16((frac16_t)0, &this->sCurrSec16.sFiltPhaB);
    GDFLIB_FilterMAInit_F16((frac16_t)0, &this->sCurrSec16.sFiltPhaC);
    GDFLIB_FilterMAInit_F16((frac16_t)0, &this->sCurrSec23.sFiltPhaA);
    GDFLIB_FilterMAInit_F16((frac16_t)0, &this->sCurrSec23.sFiltPhaC);
    GDFLIB_FilterMAInit_F16((frac16_t)0, &this->sCurrSec45.sFiltPhaA);
    GDFLIB_FilterMAInit_F16((frac16_t)0, &this->sCurrSec45.sFiltPhaB);

}

/*!
 * @brief Function reads current offset samples and filter them based on SVM sector
 *
 * @param this   Pointer to the current object
 *
 * @return  True when SVM sector is correct
 */
RAM_FUNC_CRITICAL bool_t MCDRV_Curr3Ph2ShCalib(mcdrv_adc_t *this)
{
    bool_t bStatusPass = FALSE;

    int16_t i16Rslt0;
    int16_t i16Rslt1;

    switch (*this->pui16SVMSector)
    {
        case 2:
        case 3:
            /* sensing of offset IA -> ADCA and IC -> ADCC */
        	i16Rslt0 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec23.pui16RsltRegPhaA))<<3);
        	i16Rslt1 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec23.pui16RsltRegPhaC))<<3);
            this->sCurrSec23.ui16CalibPhaA = GDFLIB_FilterMA_F16((frac16_t)(i16Rslt0), &this->sCurrSec23.sFiltPhaA);
            this->sCurrSec23.ui16CalibPhaC = GDFLIB_FilterMA_F16((frac16_t)(i16Rslt1), &this->sCurrSec23.sFiltPhaC);
            bStatusPass = TRUE;
            break;
        case 4:
        case 5:
            /* sensing of offset IA -> ADCA and IB -> ADCC */
        	i16Rslt0 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec45.pui16RsltRegPhaA))<<3);
        	i16Rslt1 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec45.pui16RsltRegPhaB))<<3);
            this->sCurrSec45.ui16CalibPhaA = GDFLIB_FilterMA_F16((frac16_t)(i16Rslt0), &this->sCurrSec45.sFiltPhaA);
            this->sCurrSec45.ui16CalibPhaB = GDFLIB_FilterMA_F16((frac16_t)(i16Rslt1), &this->sCurrSec45.sFiltPhaB);
            bStatusPass = TRUE;
            break;
        case 1:
        case 6:
            /* sensing of offset IB -> ADCA and IC -> ADCC */
        	i16Rslt0 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec16.pui16RsltRegPhaB))<<3);
        	i16Rslt1 = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->sCurrSec16.pui16RsltRegPhaC))<<3);
            this->sCurrSec16.ui16CalibPhaB = GDFLIB_FilterMA_F16((frac16_t)(i16Rslt0), &this->sCurrSec16.sFiltPhaB);
            this->sCurrSec16.ui16CalibPhaC = GDFLIB_FilterMA_F16((frac16_t)(i16Rslt1), &this->sCurrSec16.sFiltPhaC);
            bStatusPass = TRUE;
            break;
        default:
        	break;
    }

    return (bStatusPass);
}

/*!
 * @brief Function passes measured offset values to main structure
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_Curr3Ph2ShCalibSet(mcdrv_adc_t *this)
{

    /* pass calibration data for sector 1 and 6 */
    this->sCurrSec16.ui16OffsetPhaB = this->sCurrSec16.ui16CalibPhaB;
    this->sCurrSec16.ui16OffsetPhaC = this->sCurrSec16.ui16CalibPhaC;

    /* pass calibration data for sector 2 and 3 */
    this->sCurrSec23.ui16OffsetPhaA = this->sCurrSec23.ui16CalibPhaA;
    this->sCurrSec23.ui16OffsetPhaC = this->sCurrSec23.ui16CalibPhaC;

    /* pass calibration data for sector 4 and 5 */
    this->sCurrSec45.ui16OffsetPhaA = this->sCurrSec45.ui16CalibPhaA;
    this->sCurrSec45.ui16OffsetPhaB = this->sCurrSec45.ui16CalibPhaB;

}

/*!
 * @brief Function reads and passes DCB voltage sample
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_VoltDcBusGet(mcdrv_adc_t *this)
{
    int16_t i16Rslt;

    /* read DC-bus voltage sample from defined ADCx result register */
    i16Rslt = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->pui16RsltRegVDcb))<<3);
    *this->pf16UDcBus = (frac16_t)(i16Rslt); /* ADC_ETC trigger0 (ADC1) chain1 */

}

/*!
 * @brief Function reads and passes auxiliary sample
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_AuxValGet(mcdrv_adc_t *this)
{
    int16_t i16Rslt;

    /* read Auxiliary channel sample from defined ADCx result register */
    i16Rslt = MLIB_Mul_F16as(this->a32Gain, (frac16_t)(*(this->pui16RsltRegAux))<<3);
    *this->pui16AuxChan = (frac16_t)(i16Rslt); /* ADC_ETC trigger4 (ADC2) chain1 */
    
}

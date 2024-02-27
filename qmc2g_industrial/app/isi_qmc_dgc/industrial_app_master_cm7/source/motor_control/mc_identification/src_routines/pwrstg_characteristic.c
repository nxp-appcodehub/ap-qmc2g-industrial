/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pwrstg_characteristic.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Power Stage Characterization routine
 *
 * @param *sTransferCharFcn input structure of type #mid_get_char_t for passing
 *                          all necessary parameters.
 *
 * @return None
 */
static float_t fltRs_voltage_drop;      /* Auxiliary variable for Rs voltage drop calculation */
static float_t fltUdReqFilt;            /* Filtered Ud required value */
static float_t fltIdfbckFilt;           /* Filtered Id feedback value */

void MID_GetTransferCharacteristic(mid_get_char_t* sTransferCharFcn)
{

    /* Initialisation */
    if(sTransferCharFcn->ui16Active == 0)
    {
        sTransferCharFcn->ui16Active      = TRUE;
        sTransferCharFcn->ui16LoopCounter = 0;
        sTransferCharFcn->fltIdReqActual  = MLIB_Neg_FLT(sTransferCharFcn->fltIdCalib);
        *(sTransferCharFcn->pfltIdReq)    = sTransferCharFcn->fltIdReqActual;
        sTransferCharFcn->ui16LUTIndex    = 0;
        sTransferCharFcn->sUdReqMA32Filter.fltLambda = 1.0/20.0;
        GDFLIB_FilterMAInit_FLT(0.0, &sTransferCharFcn->sUdReqMA32Filter);
        sTransferCharFcn->sIdfbckMA32Filter.fltLambda = 1.0/20.0;
        GDFLIB_FilterMAInit_FLT(0.0, &sTransferCharFcn->sIdfbckMA32Filter);
    }

    /* LoopCounter for time keeping */
    sTransferCharFcn->ui16LoopCounter++;
    
    /* Filter required voltage and feedback current*/
    fltUdReqFilt       = GDFLIB_FilterMA_FLT(*(sTransferCharFcn->pfltUdReq), &sTransferCharFcn->sUdReqMA32Filter);
    fltIdfbckFilt      = GDFLIB_FilterMA_FLT(*(sTransferCharFcn->pfltIdfbck), &sTransferCharFcn->sIdfbckMA32Filter);

    /* After 600ms settling of Id start calculation */
    if(sTransferCharFcn->ui16LoopCounter >= MID_TIME_600MS)
    {
        /* Faults */
        /* Check if Rs is low enough to reach 2A */
        if((MLIB_Abs_FLT(*(sTransferCharFcn->pfltIdfbck)) < (sTransferCharFcn->fltIdCalib - MID_K_I_50MA)) && (sTransferCharFcn->ui16LUTIndex == 0))
        {
            g_sMID.ui16FaultMID |= MID_FAULT_TOO_HIGH_RS;
            sTransferCharFcn->ui16Active   = FALSE;
            *(sTransferCharFcn->pfltIdReq) = 0.0;
        }
        /* Check if motor is connected */
        if((MLIB_Abs_FLT(*(sTransferCharFcn->pfltIdfbck)) < MID_K_I_50MA) && (sTransferCharFcn->ui16LUTIndex == 0))
        {
            g_sMID.ui16FaultMID |= MID_FAULT_NO_MOTOR;
            sTransferCharFcn->ui16Active   = FALSE;
            *(sTransferCharFcn->pfltIdReq) = 0.0;
        }

        /* Calculate voltage drop from Rs and Id */
        /* float eq. V_Rs = Rs * Idfbck */
        fltRs_voltage_drop = MLIB_Mul_FLT(sTransferCharFcn->fltRs, fltIdfbckFilt);
        
        /* Calculate Error voltage and store it to f16ErrorLookUp */
        /* float eq. Error voltage = (Required voltage - Rs voltage drop) / DCbus */
        sTransferCharFcn->fltUdErrorLookUp[sTransferCharFcn->ui16LUTIndex] = MLIB_Div_FLT(MLIB_Sub_FLT(fltUdReqFilt, fltRs_voltage_drop), *(sTransferCharFcn->pfltUDCbus));

        /* Prepare for next point measurement */
        sTransferCharFcn->ui16LUTIndex++;
        sTransferCharFcn->fltIdReqActual = MLIB_Add_FLT(sTransferCharFcn->fltIdReqActual, sTransferCharFcn->fltIdIncrement);
        *(sTransferCharFcn->pfltIdReq) = sTransferCharFcn->fltIdReqActual;
        sTransferCharFcn->ui16LoopCounter = 0;

        /* End after last current was measured */
        if(sTransferCharFcn->ui16LUTIndex >= MID_CHAR_CURRENT_POINT_NUMBERS)
        {
            sTransferCharFcn->ui16Active   = FALSE;
            *(sTransferCharFcn->pfltIdReq) = 0.0;
        }
    }
}


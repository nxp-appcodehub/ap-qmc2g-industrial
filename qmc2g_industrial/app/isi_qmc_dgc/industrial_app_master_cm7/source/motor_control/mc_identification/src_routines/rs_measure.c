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

#include "rs_measure.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Rs measurement routine
 *
 * @param *sRsMeasFcn input structure of type #mid_get_rs_t for passing
 *                          all necessary parameters.
 *
 * @return None
 */
void MID_getRs(mid_get_rs_t* sRsMeasFcn)
{
    float_t               fltRsUdReqFilt, fltRsIdfbckFilt;

    /* Initialization */
    if(sRsMeasFcn->ui16Active == 0)
    {
        sRsMeasFcn->ui16Active              = TRUE;
        sRsMeasFcn->ui16LoopCounter         = 0;
        sRsMeasFcn->fltRs                   = 0.0;
        sRsMeasFcn->sUdReqMA32Filter.fltLambda  = 1.0/20.0;
        GDFLIB_FilterMAInit_FLT(0.0, &sRsMeasFcn->sUdReqMA32Filter);
        sRsMeasFcn->sIdfbckMA32Filter.fltLambda = 1.0/20.0;
        GDFLIB_FilterMAInit_FLT(0.0, &sRsMeasFcn->sIdfbckMA32Filter);
        
        /* Set the measuring current Id_meas*/
        *(sRsMeasFcn->pfltIdReq) = sRsMeasFcn->fltIdMeas;
    }

    /* LoopCounter for time keeping */
    sRsMeasFcn->ui16LoopCounter++;
    
    /* Filter required voltage and feedback current*/
    fltRsUdReqFilt       = GDFLIB_FilterMA_FLT(*(sRsMeasFcn->pfltUdReq), &sRsMeasFcn->sUdReqMA32Filter);
    fltRsIdfbckFilt      = GDFLIB_FilterMA_FLT(*(sRsMeasFcn->pfltIdfbck), &sRsMeasFcn->sIdfbckMA32Filter);

    /* After 1200ms start calculation */
    if(sRsMeasFcn->ui16LoopCounter == MID_TIME_1200MS)
    {
        /* Set required current to zero */
        *(sRsMeasFcn->pfltIdReq) = 0.0;

        /* Calculate Rs from Ud_correct and Id */
        /* float eq. Rs = UdCorrect / Idfbck */
        sRsMeasFcn->fltRs = MLIB_Div_FLT(fltRsUdReqFilt, fltRsIdfbckFilt);

        /* Set Id_req to zero */
        *(sRsMeasFcn->pfltIdReq) = 0.0;

        /* Check Faults */
        /* Check if motor is connected */
        if(MLIB_Abs_FLT(*(sRsMeasFcn->pfltIdfbck)) < MID_K_I_50MA)
            g_sMID.ui16FaultMID |= MID_FAULT_NO_MOTOR;

        /* Check if Rs is negative or saturated*/
        if(sRsMeasFcn->fltRs < 0.0)
            g_sMID.ui16WarnMID |= MID_WARN_RS_OUT_OF_RANGE;

        /* Check if measuring current was reached */
        if(*(sRsMeasFcn->pfltIdfbck) < MLIB_Sub_FLT(sRsMeasFcn->fltIdMeas, MID_K_I_50MA))
            g_sMID.ui16WarnMID |= MID_WARN_DC_CUR_NOT_REACHED;
    }

    /* Wait additional 1200ms to stabilize Id at 0A */
    /* Exit the function after 2400ms */
    if(sRsMeasFcn->ui16LoopCounter > MID_TIME_2400MS)
    {
        sRsMeasFcn->ui16Active = FALSE;
    }
}


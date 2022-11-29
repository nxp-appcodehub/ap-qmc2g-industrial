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

#include "ke_measure.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Ke measurement routine
 *
 * @param *sKeMeasFcn   input structure of type #mid_get_ke_t for passing
 *                      all necessary parameters.
 *
 * @return None
 */
void MID_getKe(mid_get_ke_t* sKeMeasFcn)
{
    float_t fltEdFilt, fltEqFilt;
    float_t fltEdFiltSquare, fltEqFiltSquare;
    float_t fltEtotal;

    /* Initialisation */
    if(sKeMeasFcn->ui16Active == FALSE)
    {
        sKeMeasFcn->ui16Active                      = TRUE;
        sKeMeasFcn->ui16LoopCounter                 = 0;
        sKeMeasFcn->fltFreqElRamp                   = 0.0;
        sKeMeasFcn->sFreqElRampParam.fltRampUp      = sKeMeasFcn->fltFreqElReq / MID_SPEED_RAMP_TIME / 10000;
        sKeMeasFcn->sFreqElRampParam.fltRampDown    = sKeMeasFcn->fltFreqElReq / MID_SPEED_RAMP_TIME / 10000;
        sKeMeasFcn->sEdMA32Filter.fltLambda         = 1.0/10.0;
        GDFLIB_FilterMAInit_FLT(0.0, &sKeMeasFcn->sEdMA32Filter);
        sKeMeasFcn->sEqMA32Filter.fltLambda         = 1.0/10.0;
        GDFLIB_FilterMAInit_FLT(0.0, &sKeMeasFcn->sEqMA32Filter);
        sKeMeasFcn->sFreqIntegrator.a32Gain         = ACC32(1 * sKeMeasFcn->fltFreqMax / 10000 * 2);
        GFLIB_IntegratorInit_F16(0, &sKeMeasFcn->sFreqIntegrator);
        GFLIB_RampInit_FLT(0.0, &sKeMeasFcn->sFreqElRampParam);
    }
    /* Set Id required */
    *(sKeMeasFcn->pfltIdReq) = sKeMeasFcn->fltIdReqOpenLoop;
    /* Ramp electrical speed */
    sKeMeasFcn->fltFreqElRamp = GFLIB_Ramp_FLT(sKeMeasFcn->fltFreqElReq, &sKeMeasFcn->sFreqElRampParam);
    /* Integrate electrical speed to get electrical position */
    *sKeMeasFcn->pf16PosEl = GFLIB_Integrator_F16(MLIB_ConvSc_F16ff(sKeMeasFcn->fltFreqElRamp, sKeMeasFcn->fltFreqMax), &sKeMeasFcn->sFreqIntegrator);

    /* Bemf filtering */
    fltEdFilt = GDFLIB_FilterMA_FLT(*(sKeMeasFcn->pfltEd), &sKeMeasFcn->sEdMA32Filter);
    fltEqFilt = GDFLIB_FilterMA_FLT(*(sKeMeasFcn->pfltEq), &sKeMeasFcn->sEqMA32Filter);

    if(sKeMeasFcn->fltFreqElRamp == sKeMeasFcn->fltFreqElReq)
    {
        sKeMeasFcn->ui16LoopCounter++;

        if(sKeMeasFcn->ui16LoopCounter > MID_TIME_2400MS)
        {
            /* Total Bemf calculation */
            fltEdFiltSquare = MLIB_Mul_FLT(fltEdFilt, fltEdFilt);
            fltEqFiltSquare = MLIB_Mul_FLT(fltEqFilt, fltEqFilt);
            fltEtotal = GFLIB_Sqrt_FLT(MLIB_Add_FLT(fltEdFiltSquare, fltEqFiltSquare));

            /* Ke calculation */
            sKeMeasFcn->fltKe = MLIB_Div_FLT(fltEtotal, sKeMeasFcn->fltFreqElReq);

            /* Check Faults */
            /* Check if Ke is negative or saturated*/
            if(sKeMeasFcn->fltKe < 0.0)
                g_sMID.ui16WarnMID |= MID_WARN_KE_OUT_OF_RANGE;

            /* When finished exit the function */
            sKeMeasFcn->ui16Active = FALSE;
        }
    }
}


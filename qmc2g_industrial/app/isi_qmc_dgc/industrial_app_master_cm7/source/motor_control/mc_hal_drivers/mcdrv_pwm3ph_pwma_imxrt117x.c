/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <mc_hal_drivers/mcdrv_pwm3ph_pwma_imxrt117x.h>
#include "mc_common.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Function updates eFlexPWM DUTY related value register
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_eFlexPwm3PhDutyUpdate(mcdrv_pwm3ph_pwma_t *this)
{
    frac16_t f16DutyRegVal, f16ModuloTemp;
    GMCLIB_3COOR_T_F16 sDutyABCtemp;
    
    uint8_t ui8MaskTemp = 0U;

    /* pointer to duty cycle structure */
    sDutyABCtemp = *this->psDutyABC;

    /* get modulo value from module 0 VAL1 register  */
    f16ModuloTemp = this->pui32PwmBaseAddress->SM[this->ui16PhASubNum].VAL1 + 1;

    /* phase A */
    f16DutyRegVal = MLIB_Mul_F16(f16ModuloTemp, sDutyABCtemp.f16A);
    this->pui32PwmBaseAddress->SM[this->ui16PhASubNum].VAL3 = f16DutyRegVal;
    this->pui32PwmBaseAddress->SM[this->ui16PhASubNum].VAL2 = MLIB_Neg_F16(f16DutyRegVal);

    /* phase B */
    f16DutyRegVal = MLIB_Mul_F16(f16ModuloTemp, sDutyABCtemp.f16B);
    this->pui32PwmBaseAddress->SM[this->ui16PhBSubNum].VAL3 = f16DutyRegVal;
    this->pui32PwmBaseAddress->SM[this->ui16PhBSubNum].VAL2 = MLIB_Neg_F16(f16DutyRegVal);

    /* phase C */
    f16DutyRegVal = MLIB_Mul_F16(f16ModuloTemp, sDutyABCtemp.f16C);
    this->pui32PwmBaseAddress->SM[this->ui16PhCSubNum].VAL3 = f16DutyRegVal;
    this->pui32PwmBaseAddress->SM[this->ui16PhCSubNum].VAL2 = MLIB_Neg_F16(f16DutyRegVal);

    /* set LDOK bits */
    ui8MaskTemp = ((uint16_t)(1U) << (this->ui16PhASubNum))|((uint16_t)(1U) << (this->ui16PhBSubNum))|((uint16_t)(1U) << (this->ui16PhCSubNum));
    this->pui32PwmBaseAddress->MCTRL |= PWM_MCTRL_LDOK(ui8MaskTemp);
}

/*!
 * @brief Function enables PWM outputs
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_eFlexPwm3PhOutEnable(mcdrv_pwm3ph_pwma_t *this)
{
    uint8_t ui8MaskTemp = 0U;  
    
    ui8MaskTemp = ((uint16_t)(1U) << (this->ui16PhASubNum))|((uint16_t)(1U) << (this->ui16PhBSubNum))|((uint16_t)(1U) << (this->ui16PhCSubNum));
      
    /* PWM outputs of sub-modules 0,1 and 2 enabled */
    /* PWM_A output */
    this->pui32PwmBaseAddress->OUTEN =
        (this->pui32PwmBaseAddress->OUTEN & ~PWM_OUTEN_PWMA_EN_MASK) | PWM_OUTEN_PWMA_EN(ui8MaskTemp) 
          | (this->pui32PwmBaseAddress->OUTEN);

    /* PWM_B output */
    this->pui32PwmBaseAddress->OUTEN =
        (this->pui32PwmBaseAddress->OUTEN & ~PWM_OUTEN_PWMB_EN_MASK) | PWM_OUTEN_PWMB_EN(ui8MaskTemp) 
          | (this->pui32PwmBaseAddress->OUTEN);
}

/*!
 * @brief Function disables PWM outputs
 *
 * @param this   Pointer to the current objectf
 *
 * @return none
 */
RAM_FUNC_CRITICAL void MCDRV_eFlexPwm3PhOutDisable(mcdrv_pwm3ph_pwma_t *this)
{
    uint32_t    ui32MaskTemp = 0U;
    uint16_t     ui16PhSubTemp = 0U; 

    ui16PhSubTemp = ~(((uint16_t)(1U) << (this->ui16PhASubNum))|((uint16_t)(1U) << (this->ui16PhBSubNum))|((uint16_t)(1U) << (this->ui16PhCSubNum)));
    
    /* PWM outputs of used PWM sub-modules disabled */
    /* PWM_A output */
    ui32MaskTemp = ((this->pui32PwmBaseAddress->OUTEN & PWM_OUTEN_PWMA_EN_MASK) 
                    >> PWM_OUTEN_PWMA_EN_SHIFT) & ui16PhSubTemp;
    
    this->pui32PwmBaseAddress->OUTEN =
      (this->pui32PwmBaseAddress->OUTEN & ~PWM_OUTEN_PWMA_EN_MASK) | PWM_OUTEN_PWMA_EN(ui32MaskTemp);
     
    /* PWM_B output */  
    ui32MaskTemp = ((this->pui32PwmBaseAddress->OUTEN & PWM_OUTEN_PWMB_EN_MASK) 
                    >> PWM_OUTEN_PWMB_EN_SHIFT) & ui16PhSubTemp;
    
    this->pui32PwmBaseAddress->OUTEN =
        (this->pui32PwmBaseAddress->OUTEN & ~PWM_OUTEN_PWMB_EN_MASK) | PWM_OUTEN_PWMB_EN(ui32MaskTemp);

}

/*!
 * @brief Function return actual value of fault flag and pin status
 *
 * @param this   Pointer to the current object
 *
 * @return True if there's any fault, otherwise return false
 */
RAM_FUNC_CRITICAL bool_t MCDRV_eFlexPwm3PhFaultGet(mcdrv_pwm3ph_pwma_t *this)
{
    bool_t bStatusPass = FALSE;
	uint16_t ui16StatusFlags, ui16StatusPins;

	/* read fault flags */
    ui16StatusFlags = (((this->pui32PwmBaseAddress->FSTS & PWM_FSTS_FFLAG_MASK) >> PWM_FSTS_FFLAG_SHIFT) &
                    ((uint16_t)(1) << this->ui16FaultFixNum | (uint16_t)(1) << this->ui16FaultAdjNum));

    /* read fault pins status */
    /* Reading pin status because fault flag is only triggered by signal edge, there can be situations where fault signals are
     * asserted the moment system is powered on, and eFlexPWM module hasn't been initialized yet. In this case, fault flags won't
     * be set even though fault signals are valid.
     *  */
    ui16StatusPins = (((this->pui32PwmBaseAddress->FSTS & PWM_FSTS_FFPIN_MASK) >> PWM_FSTS_FFPIN_SHIFT) &
            ((uint16_t)(1) << this->ui16FaultFixNum | (uint16_t)(1) << this->ui16FaultAdjNum));

    /* clear faults flag */
    this->pui32PwmBaseAddress->FSTS = ((this->pui32PwmBaseAddress->FSTS & ~(PWM_FSTS_FFLAG_MASK)) |
                                       ((uint16_t)(1) << this->ui16FaultFixNum | (uint16_t)(1) << this->ui16FaultAdjNum));

    if((ui16StatusFlags > 0)||(ui16StatusPins > 0))
    {
    	bStatusPass = TRUE;
    }
    else
    {
    	bStatusPass = FALSE;
    }
    return (bStatusPass);
}


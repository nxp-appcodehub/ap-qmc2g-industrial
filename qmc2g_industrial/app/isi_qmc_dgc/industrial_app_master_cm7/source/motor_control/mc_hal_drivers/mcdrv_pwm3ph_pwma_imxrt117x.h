/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _MCDRV_PWM3PH_PWMA_H_
#define _MCDRV_PWM3PH_PWMA_H_

#include "mlib.h"
#include "mlib_types.h"
#include "fsl_device_registers.h"
#include "gmclib.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MCDRV_PWM0 (1)
#define MCDRV_PWM1 (2)


typedef struct _mcdrv_pwm3ph_pwma
{
    GMCLIB_3COOR_T_F16 *psDutyABC;    /* pointer to the 3-phase pwm duty cycles */
    PWM_Type *pui32PwmBaseAddress; /* PWMA base address */
    uint16_t ui16PhASubNum;        /* PWMA phase A sub-module number */
    uint16_t ui16PhBSubNum;        /* PWMA phase B sub-module number */
    uint16_t ui16PhCSubNum;        /* PWMA phase C sub-module number */
    uint16_t ui16FaultFixNum;      /* PWMA fault number for fixed over-current fault detection */
    uint16_t ui16FaultAdjNum;      /* PWMA fault number for adjustable over-current fault detection */
} mcdrv_pwm3ph_pwma_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Function updates eFlexPWM DUTY related value register
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
void MCDRV_eFlexPwm3PhDutyUpdate(mcdrv_pwm3ph_pwma_t *this);

/*!
 * @brief Function enables PWM outputs
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
void MCDRV_eFlexPwm3PhOutEnable(mcdrv_pwm3ph_pwma_t *this);

/*!
 * @brief Function disables PWM outputs
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */

void MCDRV_eFlexPwm3PhOutDisable(mcdrv_pwm3ph_pwma_t *this);

/*!
 * @brief Function return actual value of fault flag and pin status
 *
 * @param this   Pointer to the current object
 *
 * @return True if there's any fault, otherwise return false
 */

bool_t MCDRV_eFlexPwm3PhFaultGet(mcdrv_pwm3ph_pwma_t *this);

/*!
 * @brief Function enables sub-module0~2 counters of an eFlexPWM
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */

__attribute__((always_inline)) static inline void PWM_SM012_RUN(mcdrv_pwm3ph_pwma_t *this)
{
	this->pui32PwmBaseAddress->MCTRL |= PWM_MCTRL_RUN(0x07);
}

/*!
 * @brief Function enables sub-module0~3 counters of an eFlexPWM
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
__attribute__((always_inline)) static inline void PWM_SM0123_RUN(mcdrv_pwm3ph_pwma_t *this)
{
	this->pui32PwmBaseAddress->MCTRL |= PWM_MCTRL_RUN(0x0F);
}



#ifdef __cplusplus
}
#endif

#endif /* _MCDRV_PWM3PH_PWMA_H_ */


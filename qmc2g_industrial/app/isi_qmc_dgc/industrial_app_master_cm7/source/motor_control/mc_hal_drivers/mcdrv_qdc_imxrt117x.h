/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef MCDRV_QDC_H_
#define MCDRV_QDC_H_

#include "amclib.h"
#include "amclib_FP.h"
#include "fsl_device_registers.h"
#include "mlib.h"
#include "mlib_FP.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/


typedef struct _qdc_to
{
	frac16_t    f16PosErr; // Position error to the tracking observer
	AMCLIB_TRACK_OBSRV_T_FLT    sTO; // Tracking observer
	frac16_t    f16PosEstim;
	float_t     fltSpeedEstim;
}qdc_to_t;

typedef struct _qdc_speed
{
	int16_t i16POSDH;
	uint16_t ui16POSDPERH;
	uint16_t ui16LASTEDGEH;

	int16_t  i16PosDiff;    // Position counter differences between 2 consecutive speed calculations
	uint16_t ui16Period;     // Time duration corresponding to the position counter differences
	uint16_t ui16Period_1;   // Last time duration

	int8_t   i8SpeedSign;   // Speed sign in this step
	int8_t   i8SpeedSign_1; // Speed sign in last step
	frac32_t f32Speed;      // Calculated speed
	frac16_t f16SpeedFilt;

	GDFLIB_FILTER_IIR1_T_F32 			sQDCSpeedFilter;
	frac32_t f32SpeedCalConst;
	float_t  fltSpeedFrac16ToAngularCoeff;

	float_t fltSpeed;		// float type, electrical angular speed
	int32_t i32Position;
	int32_t i32Position_1;
}qdc_speed_t;

typedef struct _qdc_block
{
	qdc_to_t    sSpeedEstim;    // A module to estimate speed by tracking observer
	qdc_speed_t sSpeed;			// A module to calculate speed by QDC HW feature


	/* Encoder attributes */
    uint16_t    ui16Line;       // Encoder lines
	uint16_t    ui16PolePair;   // Motor pole pairs
    frac32_t    f32ReciprocalLines;  // Reciprocal of 4*Lines
    uint32_t    ui32Mod;        // QDC counter MOD value
    ENC_Type    *pQDC_base;     // QDC module base address

    /* Rotor position related for FOC algorithm */
    uint32_t    ui32InitCount;      // QDC counter value when the initial position has been identified
    frac32_t    f32PosMechInit;     // The mechanical rotor position corresponding to the initial QDC counter value
    frac32_t    f32PosMechOffset;   // Rotor real mechanical position at the initial position
    uint32_t    ui32CurrentCount;   // Current QDC counter value
    int32_t     i32Q10Cnt2PosGain;      // A gain to convert QDC counter value to scaled position value -1~1. This gain is Q22.10 format
	frac32_t    f32PosMech;         // Rotor real mechanical position
	frac32_t    f32PosElec;         // Rotor real electrical position, Q1.31
	frac16_t    f16PosElec;			// Rotor real electrical position, Q1.15

	/* Rotor position related for position loop control */
    int32_t     i32Q16InitRev;      // Revolution value when initial rotor position has been identified. Q16.16 format, integer part represents revolutions, fractional part represents the part which is "less than 1 revolution"
	int32_t     i32Q16Rev;			// Current revolution value, Q16.16
	int32_t     i32Q16DeltaRev;     // Revolutions between current rotor position and the initial rotor position, Q16.16

	bool_t      bPosAbsoluteFlag; // A flag indicating whether rotor position is an absolute correct value
	uint16_t    ui16Dummy;          // A dummy variable to get captured position counter value
	uint8_t  	ui8MotorNum;

	/* Index signal related */
	uint32_t    ui32IndexCount;    // QDC counter value when index signal occurs
	frac32_t    f32PosMechIndex;   // The mechanical rotor position corresponding to the QDC counter value when index signal occurs
	frac32_t    f32PosElecIndex;
	frac16_t    f16PosElecIndex;
}qdc_block_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/*!
 * @brief Get how many rounds the rotor has revolved with regard to its initial position:i32Q16InitRev, the result is stored in i32Q16DeltaRev, which is Q16.16 format.
         It can represent -32768 ~ 32767.99974 revolutions.

 * @param this   Pointer to the current object
 *
 * @return boot_t true on success
 */
__attribute__((always_inline)) static inline void MCDRV_GetRotorDeltaRev(qdc_block_t *this)
{
	this->i32Q16DeltaRev = this->i32Q16Rev - this->i32Q16InitRev;
}

/*!
 * @brief Initialize rotor speed calculation(Enhanced M/T method) related parameters.
 *
 * @param this   Pointer to the current object
 *
 * @return boot_t true on success
 */
extern bool_t MCDRV_QdcSpeedCalInit(qdc_block_t *this);

/*!
 * @brief Calculate rotor speed by QDC enhanced M/T speed measurement feature.
 *        The speed is stored in sSpeed.f16SpeedFilt(Q1.15,filtered), sSpeed.fltSpeed(float,filtered) and sSpeed.f32Speed(Q1.31, not filtered).
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
extern void MCDRV_QdcSpeedCalUpdate(qdc_block_t *this);

/*!
 * @brief Initialize rotor speed calculation(Tracking Observer method) related parameters.
 *
 * @param this   Pointer to the current object
 *
 * @return boot_t true on success
 */
extern bool_t MCDRV_QdcToSpeedCalInit(qdc_block_t *this);

/*!
 * @brief Calculate rotor speed by tracking observer method.
 *        The speed is stored in sSpeedEstim.fltSpeedEstim(float,not filtered).
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
extern void MCDRV_QdcToSpeedCalUpdate(qdc_block_t *this);

/*!
 * @brief Initialize QDC driver parameters.
 *
 * @param this   Pointer to the current object
 *
 * @return boot_t true on success
 */
extern bool_t MCDRV_QdcInit(qdc_block_t *this);

/*!
 * @brief When initial rotor position has been identified:

         1. Get position counter value, and store it to ui32InitCount
         2. Transform this counter value to mechanical position value, store it to f32PosMechInit, which is Q1.31 format, corresponding to -pi~pi
         3. Store rotor flux initial mechanical position (between rotor flux and stator A-axis) to f32PosMechOffset, which is Q1.31 format.

 * @param this               Pointer to the current object
         f32PosMechOffset   The mechanical angle between rotor flux and stator A-axis when initial rotor position is identified. Q1.31 format.
 * @return none
 */
extern void MCDRV_GetRotorInitPos(qdc_block_t *this, frac32_t f32PosMechOffset);

/*!
 * @brief Get the initial revolution value (Q16.16 format) from REV counter and position counter, store it to i32Q16InitRev. This initial revolution value will be used as a base.
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
extern void MCDRV_GetRotorInitRev(qdc_block_t *this);

/*!
 * @brief Get current rotor mechanical/electrical position after rotor initial position has been identified.
         Remember to call MCDRV_GetRotorInitPos before invoking this function.

             1. Get current position counter value, and store it to ui32CurrentCount
             2. Transform this counter value to rotor flux real mechanical/position value (relative to A-axis)

 * @param this   Pointer to the current object
 *
 * @return none
 */
extern void MCDRV_GetRotorCurrentPos(qdc_block_t *this);

/*!
 * @brief Get current rotor revolution value from Rev counter and position counter, store it to i32Q16Rev, which is Q16.16 format.
         It can represent -32768 ~ 32767.99974 revolutions.

 * @param this   Pointer to the current object
 *
 * @return none
 */
extern void MCDRV_GetRotorCurrentRev(qdc_block_t *this);

#ifdef __cplusplus
}
#endif

#endif /* MCDRV_QDC_H_ */

/*
 * Copyright 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MCDRV_ADC_IMXRT117x_H_
#define _MCDRV_ADC_IMXRT117x_H_

#include "gdflib.h"
#include "mlib_types.h"
#include "gmclib.h"
#include "fsl_device_registers.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NOT_EXIST      0xFFFF
#define CALIB_MA_NUM   8 // Moving average filter number for phase current offsets


typedef struct _channel_info
{
	uint16_t ui16ChanNum; 			/* Channel number */
	uint16_t ui16Side;				/* A or B side */
	uint16_t ui16Cmd;				/* Command number. Channel is assigned to this command */

}channel_info_t;

typedef struct _channel_table
{
	channel_info_t sLPADC1IA;
	channel_info_t sLPADC2IA;
	channel_info_t sLPADC1IB;
	channel_info_t sLPADC2IB;
	channel_info_t sLPADC1IC;
	channel_info_t sLPADC2IC;
	channel_info_t sLPADC1VDcb;
	channel_info_t sLPADC2VDcb;
	channel_info_t sLPADC1Aux;
	channel_info_t sLPADC2Aux;
	uint16_t ui16Average;           /* Average number = 2^ui16Average */
}channel_table_t;

typedef struct _pha_bc
{
    GDFLIB_FILTER_MA_T_A32 sFiltPhaB; /* phase B offset filter */
    GDFLIB_FILTER_MA_T_A32 sFiltPhaC; /* phase C offset filter */

    uint16_t *pui16RsltRegPhaB;            /* phase B result register */
    uint16_t *pui16RsltRegPhaC;            /* phase C result register */
    uint16_t ui16CalibPhaB;              /* phase B offset calibration */
    uint16_t ui16CalibPhaC;              /* phase C offset calibration */
    uint16_t ui16OffsetPhaB;             /* phase B offset result */
    uint16_t ui16OffsetPhaC;             /* phase C offset result */
    uint16_t ui16ADC1Seg0CmdNum;
    uint16_t ui16ADC2Seg0CmdNum;
} pha_bc_t;

typedef struct _pha_ac
{
    GDFLIB_FILTER_MA_T_A32 sFiltPhaA; /* phase A offset filter */
    GDFLIB_FILTER_MA_T_A32 sFiltPhaC; /* phase C offset filter */

    uint16_t *pui16RsltRegPhaA;            /* phase A result register */
    uint16_t *pui16RsltRegPhaC;            /* phase C result register */
    uint16_t ui16CalibPhaA;              /* phase A offset calibration */
    uint16_t ui16CalibPhaC;              /* phase C offset calibration */
    uint16_t ui16OffsetPhaA;             /* phase A offset result */
    uint16_t ui16OffsetPhaC;             /* phase C offset result */
    uint16_t ui16ADC1Seg0CmdNum;
    uint16_t ui16ADC2Seg0CmdNum;
} pha_ac_t;

typedef struct _pha_ab
{
    GDFLIB_FILTER_MA_T_A32 sFiltPhaA; /* phase A offset filter */
    GDFLIB_FILTER_MA_T_A32 sFiltPhaB; /* phase B offset filter */

    uint16_t *pui16RsltRegPhaA;            /* phase A result register */
    uint16_t *pui16RsltRegPhaB;            /* phase B result register */
    uint16_t ui16CalibPhaA;              /* phase A offset calibration */
    uint16_t ui16CalibPhaB;              /* phase B offset calibration */
    uint16_t ui16OffsetPhaA;             /* phase A offset result */
    uint16_t ui16OffsetPhaB;             /* phase B offset result */
    uint16_t ui16ADC1Seg0CmdNum;
    uint16_t ui16ADC2Seg0CmdNum;
} pha_ab_t;

typedef struct _mcdrv_adc
{
	GMCLIB_3COOR_T_F16 *psIABC; /* pointer to the 3-phase currents */
	frac16_t *pf16UDcBus;       /* pointer to the DC bus voltage */
	frac16_t *pui16AuxChan;     /* pointer to the auxiliary signal */
    pha_bc_t sCurrSec16;        /* ADC setting for SVM sectors 1&6 */
    pha_ac_t sCurrSec23;        /* ADC setting for SVM sectors 2&3 */
    pha_ab_t sCurrSec45;        /* ADC setting for SVM sectors 4&5 */

    uint16_t *pui16RsltRegVDcb; /* DCB voltage result register */

    uint16_t *pui16RsltRegAux; /* Auxiliary result register */

    uint16_t *pui16SVMSector;

    uint16_t ui16OffsetFiltWindow; /* ADC Offset filter window */

    uint16_t ui16ADC1Seg1CmdNum;
    uint16_t ui16ADC2Seg1CmdNum;

    channel_table_t *psChannelAssignment;
    uint8_t  ui8MotorNum; /* M1 -> ADC_ETC trig0&4 */
    						/* M2 -> ADC_ETC trig1&5 */
    						/* M3 -> ADC_ETC trig2&6 */
    						/* M4 -> ADC_ETC trig3&7 */
    acc32_t a32Gain; /* Gain due to difference ADC reference and on-board HW scale */
} mcdrv_adc_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern mcdrv_adc_t g_sM1AdcSensor;


/*******************************************************************************
 * API
 ******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Reads and calculates 3 phase samples based on SVM sector
 *
 * @param this   Pointer to the current object
 *
 * @return boot_t True when SVM sector is correct
 */
bool_t MCDRV_Curr3Ph2ShGet(mcdrv_adc_t *this);

/*!
 * @brief Set initial channel assignment for phase currents & DCB voltage


            TRIG0 ->  ADC1_SEG0(Ia), ADC1_SEG1(Udc)
            TRIG4 ->  ADC2_SEG0(Ib), ADC2_SEG1(Ic)

            ADC input voltage is scaled to 30/64 internally by HW. ADC reference is 1.8V, to make sure input 3.3 will be
            corresponding to 1.8V, SW needs to add another scale.

            C0 = 3.3*30/64
            C1 = 1.8/C0 = 1.8*64/(3.3*30) = 1.1636363 = 0.58182*2

            (ADC_RSLT<<3)*C1 will be in range of 0~32767 when input voltage is 0~3.3V


 * @param this   Pointer to the current object
 *
 * @return  True on successful assignment
 */
bool_t MCDRV_Curr3Ph2ShChanAssignInit(mcdrv_adc_t *this);

/*!
 * @brief Set new channel assignment for next sampling based on SVM sector
 *
 * @param this   Pointer to the current object
 *
 * @return  True when SVM sector is correct
 */
bool_t MCDRV_Curr3Ph2ShChanAssign(mcdrv_adc_t *this);

/*!
 * @brief Initializes phase current channel offset measurement
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
void MCDRV_Curr3Ph2ShCalibInit(mcdrv_adc_t *this);

/*!
 * @brief Function reads current offset samples and filter them based on SVM sector
 *
 * @param this   Pointer to the current object
 *
 * @return  True when SVM sector is correct
 */
bool_t MCDRV_Curr3Ph2ShCalib(mcdrv_adc_t *this);

/*!
 * @brief Function passes measured offset values to main structure
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
void MCDRV_Curr3Ph2ShCalibSet(mcdrv_adc_t *this);

/*!
 * @brief Function passes measured offset values to main structure
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
void MCDRV_VoltDcBusGet(mcdrv_adc_t *this);

/*!
 * @brief Function reads and passes auxiliary sample
 *
 * @param this   Pointer to the current object
 *
 * @return none
 */
void MCDRV_AuxValGet(mcdrv_adc_t *this);

#ifdef __cplusplus
}
#endif

#endif /* _MCDRV_ADC_IMXRT117x_H_ */

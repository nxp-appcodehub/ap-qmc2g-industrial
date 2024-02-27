/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef MOTOR_CONTROL_MC_HAL_DRIVERS_MCDRV_GD3000_H_
#define MOTOR_CONTROL_MC_HAL_DRIVERS_MCDRV_GD3000_H_

#include <stdint.h>
#include "fsl_flexio_spi.h"

/******************************************************************************
| defines and macros
|----------------------------------------------------------------------------*/

#define READ_STATUS0_CMD  0x00
#define READ_STATUS1_CMD  0x01
#define READ_STATUS2_CMD  0x02
#define READ_STATUS3_CMD  0x03
#define SET_INT_MASK0_CMD 0x20
#define SET_INT_MASK1_CMD 0x30
#define SET_MODE_CMD      0x40
#define CLR_INT0_CMD	  0x60
#define CLR_INT1_CMD	  0x70
#define SET_DEADTIME_CMD  0x80

/******************************************************************************
| typedefs and structures
|----------------------------------------------------------------------------*/


/*------------------------------------------------------------------------*//*!
@brief  Structure containing
*//*-------------------------------------------------------------------------*/
typedef union
{
    uint8_t ui8R;
    struct {
        uint8_t    overTemp       :1;
        uint8_t    desaturation   :1;
        uint8_t    lowVls         :1;
        uint8_t    overCurrent    :1;
        uint8_t    phaseErr       :1;
        uint8_t    framingErr     :1;
        uint8_t    writeErr       :1;
        uint8_t    resetEvent     :1;
    }B;
}gd3000ConfigMask_t;

/*------------------------------------------------------------------------*//*!
@brief  Structure containing
*//*-------------------------------------------------------------------------*/
typedef union
{
    uint8_t ui8R;
    struct {
        uint8_t    lock         :1;       /*!< lock configuration regs */
        uint8_t    enableFullOn :1;       /*!< enable FULL ON PWM's without DT */
        uint8_t                 :1;
        uint8_t    disableDesat :1;       /*!< disable phase desaturation error */
        uint8_t                 :1;
        uint8_t                 :1;
        uint8_t                 :1;
        uint8_t                 :1;
    }B;
}gd3000Mode_t;

/*------------------------------------------------------------------------*//*!
@brief  Structure containing status register 0 image of the GD3000 device
*//*-------------------------------------------------------------------------*/
typedef union {
    uint8_t ui8R;
    struct {
        uint8_t     overTemp     :1;    /*!< TLIM flag detected on any channel */
        uint8_t     desaturation :1;    /*!< DESAT flag detected on any channel */
        uint8_t     lowVls       :1;    /*!< Low VLS voltage flag */
        uint8_t     overCurrent  :1;    /*!< Over-current event flag */
        uint8_t     phaseErr     :1;    /*!< Phase error flag */
        uint8_t     framingErr   :1;    /*!< Framing error flag */
        uint8_t     writeErr     :1;    /*!< Write Error After the Lock flag */
        uint8_t     resetEvent   :1;    /*!< reset event flag, is set upon exiting /RST */
    } B;
}gd3000SR0_t;

/*------------------------------------------------------------------------*//*!
@brief  Structure containing status register 1 image of the GD3000 device
*//*-------------------------------------------------------------------------*/
typedef union {
    uint8_t ui8R;
    struct {
        uint8_t     lockbit      :1;     /*!< LockBit indicates the IC regs are locked */
        uint8_t     fullon       :1;     /*!< present status of FULLON MODE */
        uint8_t                  :1;     /*!< reserved */
        uint8_t     deadtime_cal :1;     /*!< Deadtime calibration occurred */
        uint8_t     calib_overfl :1;     /*!< flag for a Deadtime Calibration Overflow */
        uint8_t     zds          :1;     /*!< Zero deadtime is commanded */
        uint8_t     desat_mode   :1;     /*!< current state of the Desaturation/Phase Error turn-off mode */
        uint8_t                  :1;     /*!< reserved */
    } B;
}gd3000SR1_t;

/*------------------------------------------------------------------------*//*!
@brief  Structure containing status register 2 image of the GD3000 device
*//*-------------------------------------------------------------------------*/
typedef union {
    uint8_t ui8R;
    struct {
        uint8_t     mask0_0      :1;     /*!< status of the MASK0.0 bit */
        uint8_t     mask0_1      :1;     /*!< status of the MASK0.1 bit */
        uint8_t     mask0_2      :1;     /*!< status of the MASK0.2 bit */
        uint8_t     mask0_3      :1;     /*!< status of the MASK0.3 bit */
        uint8_t     mask1_0      :1;     /*!< status of the MASK1.0 bit */
        uint8_t     mask1_1      :1;     /*!< status of the MASK1.1 bit */
        uint8_t     mask1_2      :1;     /*!< status of the MASK1.2 bit */
        uint8_t     mask1_3      :1;     /*!< status of the MASK1.3 bit */
    } B;
}gd3000SR2_t;


/*------------------------------------------------------------------------*//*!
@brief  Structure containing
*//*-------------------------------------------------------------------------*/
typedef struct
{
    gd3000SR0_t                uStatus0;    /*!< status register 0 */
    gd3000SR1_t                uStatus1;    /*!< status register 1 */
    gd3000SR2_t                uStatus2;    /*!< status register 2 */
    uint8_t                    ui8Status3;    /*!< status register 3 */
}GD3000Status_t;

/*------------------------------------------------------------------------*//*!
@brief  Structure containing
*//*-------------------------------------------------------------------------*/
typedef struct
{
    uint16_t                   ui16Deadtime;       /*!< define dead time of HS and LS transistors, value in [ns]*/
    gd3000ConfigMask_t         uInterruptEnable;
    gd3000Mode_t               uMode;
}GD3000ConfigData_t;

/*------------------------------------------------------------------------*//*!
@brief  Structure containing
*//*-------------------------------------------------------------------------*/
typedef struct
{
    uint16_t        ui16RequiredDeadTimeNs;
    uint16_t        ui16ActualDeadTimeNs;
    uint8_t         ui8GD3000mask0;
    uint8_t         ui8GD3000mask1;
    uint8_t         ui8GD3000mode;
}GD3000Data_t;



/*------------------------------------------------------------------------*//*!
@if         USER_GUIDE_DOC
@ingroup    PWM3PH_GROUP

@brief      Definition of public structure data type.

@details

@endif
*//*-------------------------------------------------------------------------*/
typedef struct
{
    GD3000Data_t              sData;    // User input data
    GD3000Status_t            sStatus;
    GD3000ConfigData_t        sConfigData;
    uint8_t                   ui8ResetRequest;
}GD3000_t;

/******************************************************************************
| exported variables
|----------------------------------------------------------------------------*/

/******************************************************************************
| exported function prototypes
|----------------------------------------------------------------------------*/

/*!
 * @brief  Initializes a GD3000 structure
 *
 * @param[in, out]  gdPointer   Pointer to the structure.
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_init(GD3000_t *gdPointer, FLEXIO_SPI_Type *base);

/*!
 * @brief  Reads GD3000 status registers
 *
 * @param[in, out]  gdPointer   Pointer to the structure.
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_getSR(GD3000_t *gdPointer, FLEXIO_SPI_Type *base);

/*!
 * @brief  Clears GD3000 status flags
 *
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_setZeroDeadtime(FLEXIO_SPI_Type *base);

/*!
 * @brief  Zeroes the GD3000 dead time
 *
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 */
void GD3000_clearFlags(FLEXIO_SPI_Type *base);

/*!
 * @brief  Sends data to GD3000 of FLEXIO SPI
 *
 * @param[in, out]  base	    Pointer to a FLEXIO SPI handle.
 * @param[in]  ui8Data	    Data to send.
 */
uint8_t GD3000_sendData(FLEXIO_SPI_Type *base, uint8_t ui8Data);


#endif /* MOTOR_CONTROL_MC_HAL_DRIVERS_MCDRV_GD3000_H_ */

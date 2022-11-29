/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef _API_MOTORCONTROL_H_
#define _API_MOTORCONTROL_H_

#include "api_qmc_common.h"
#include "stdbool.h"
#include "FreeRTOS.h"


/*******************************************************************************
 * Definitions => Macros
 ******************************************************************************/
#define MC_PSB_FAULTS_MASK (kMC_OverCurrent | kMC_UnderDcBusVoltage | kMC_OverDcBusVoltage | kMC_OverLoad | kMC_OverSpeed |\
							kMC_RotorBlocked | kMC_GD3000_OverTemperature | kMC_GD3000_Desaturation | kMC_GD3000_LowVLS | kMC_GD3000_OverCurrent |\
							kMC_GD3000_PhaseError | kMC_GD3000_Reset)

#define BS_PSB_FAULTS_MASK (kMC_PsbOverTemperature1 | kMC_PsbOverTemperature2)
#define ALL_MC_PSB_FAULTS_BITS_MASK (kMC_NoFaultMC |  MC_PSB_FAULTS_MASK )
#define ALL_BS_PSB_FAULTS_BITS_MASK (kMC_NoFaultBS |  BS_PSB_FAULTS_MASK )
#define ALL_PSB_FAULTS_BITS_MASK (ALL_MC_PSB_FAULTS_BITS_MASK | ALL_BS_PSB_FAULTS_BITS_MASK)
/*******************************************************************************
 * Definitions => Enumerations
 ******************************************************************************/

 /*!
 * @brief List the available motors
 */
typedef enum _mc_motor_id
{
    kMC_Motor1 = 0U, /*!< First motor */
    kMC_Motor2 = 1U, /*!< Second motor */
    kMC_Motor3 = 2U, /*!< Third motor */
    kMC_Motor4 = 3U, /*!< Fourth motor */
} mc_motor_id_t;

/*!
 * @brief Current motor control application status.
 */
typedef enum _mc_app_switch
{
    kMC_App_Off           = 0U, /*!< Motor control switched off */
    kMC_App_On            = 1U, /*!< Manual control of the position/speed enabled */
	kMC_App_Freeze        = 2U, /*!< All control of the position/speed disabled */
	kMC_App_FreezeAndStop = 3U, /*!< All control of the position/speed disabled and motors turned off */
} mc_app_switch_t;

/*!
 * @brief Motor control state.
 */
typedef enum _mc_state
{
    kMC_Fault = 0U, /*!< Any of the motor control faults has happened */
    kMC_Init  = 1U, /*!< Initialization phase of motor control (ramp up) */
    kMC_Stop  = 2U, /*!< Motors stopped */
    kMC_Run   = 3U, /*!< Motors run in one of the control methods defined by mc_method_selection_t */
} mc_state_t;

/*!
 * @brief Lists available motor control method types
 */
typedef enum _mc_method_selection
{
    kMC_ScalarControl       = 0U, /*!< Scalar motor control (V/f) */
    kMC_FOC_SpeedControl    = 1U, /*!< Direct speed setting (based on Field-Oriented-Control) */
    kMC_FOC_PositionControl = 2U, /*!< Direct position setting (based on Field-Oriented-Control) */
} mc_method_selection_t;

 /*!
 * @brief Enumeration that list motor or power stage board specific faults.
 *
 * mc_motor_id_t part of fault_source_t indicates to which motor the fault applies. Multiple faults
 * for the same motor may be encoded into the fault_source_t and sent in a single FAULT signal.
 * Faults on different motors require separate signals to be sent.
 * Upon detection of a fault listed in this enumeration, the affected motor must be stopped immediately.
 * Other motors shall be stopped according to the configuration. As these fault events are detected by
 * the Fast Motor Control Loop, it also is responsible to perform the immediate actions.
 */
typedef enum _mc_fault
{
    kMC_NoFaultMC              = 0x00000000U, /*!< No fault occured or a previous fault reported from the MC loop was resolved . */
	kMC_NoFaultBS			   = 0x00000004U, /*!< No fault occured or a previous fault reported from the Board service task was resolved . */
    kMC_OverCurrent            = 0x00000008U, /*!< Over-current event detected. */
    kMC_UnderDcBusVoltage      = 0x00000010U, /*!< DC bus under-voltage event detected */
    kMC_OverDcBusVoltage       = 0x00000020U, /*!< DC bus over-voltage event detected */
    kMC_OverLoad               = 0x00000040U, /*!< Motor overloaded */
    kMC_OverSpeed              = 0x00000080U, /*!< Motor ran into overspeed */
    kMC_RotorBlocked           = 0x00000100U, /*!< Rotor shaft is blocked */
    kMC_PsbOverTemperature1    = 0x00000200U, /*!< Over-temperature on power stage board (sensor 1) */
    kMC_PsbOverTemperature2    = 0x00000400U, /*!< Over-temperature on power stage board (sensor 2) */
	kMC_GD3000_OverTemperature = 0x00000800U, /*!< GD3000 over temperature */
	kMC_GD3000_Desaturation    = 0x00001000U, /*!< GD3000 desaturation detected */
	kMC_GD3000_LowVLS          = 0x00002000U, /*!< GD3000 VLS under voltage */
	kMC_GD3000_OverCurrent     = 0x00004000U, /*!< GD3000 over current */
	kMC_GD3000_PhaseError      = 0x00008000U, /*!< GD3000 phase error */
	kMC_GD3000_Reset           = 0x00010000U, /*!< GD3000 is in reset state */
} mc_fault_t;



/*******************************************************************************
 * Definitions => Structures
 ******************************************************************************/

/*!
 * @brief A motor position that can be accessed as a raw Q15.Q16 value or as individual components.
 */
typedef union _mc_motor_position
{
    struct {
        uint16_t ui16RotorPosition; /*!< Value represents one revolution angle fragment 360degree / 65536 = 0.0055 degree */
        int16_t  i16NumOfTurns;     /*!< Value represents number of revolutions */
    } sPosValue;
    int32_t i32Raw;                 /*!< Actual motor position; signed 32-bit (fractional Q15.16 value) */
} mc_motor_position_t;

/*!
 * @brief Holds the motor command parameters for the motor indicated by eMotorId : mc_motor_id_t.
 */

typedef struct _mc_motor_command
{
    mc_motor_id_t         eMotorId;          /*!< Motor to which this command applies */
    mc_app_switch_t       eAppSwitch;        /*!< Defines whether motor control is enabled */
    mc_method_selection_t eControlMethodSel; /*!< Control method to be used */
    union
	{

        float             fltSpeed;                         /*!< [RPM] Speed parameter for FOC speed control method */

        struct
		{
            float               fltScalarControlVHzGain;   /*!< [V/Hz] Voltage/Frequency parameter for scalar control method */
            float               fltScalarControlFrequency; /*!< [Hz] Frequency parameter for scalar control method */

        } sScalarParam;
        struct
		{
            mc_motor_position_t uPosition;                 /*!< Position parameter for FOC position control method */
            bool                bIsRandomPosition;         /*!< Indicates that the position is not on a trajectory and may need filtering */
        } sPosParam;
    } uSpeed_pos;

} mc_motor_command_t;

/*!
 * @brief Holds the parameters of the motor status that are written by the Fast Motor Control Loop.
 */
typedef struct _mc_motor_status_fast
{
    mc_state_t eMotorState;  /*!< Actual motor control state */
    mc_fault_t eFaultStatus; /*!< Actual motor control fault */
    float      fltIa;        /*!< Actual phase A current */
    float      fltIb;        /*!< Actual phase B current */
    float      fltIc;        /*!< Actual phase C current */
    float      fltValpha;    /*!< Actual alpha component voltage */
    float      fltVbeta;     /*!< Actual beta component voltage */
    float      fltVDcBus;    /*!< Actual DC bus voltage */
} mc_motor_status_fast_t;

/*!
 * @brief Holds the parameters of the motor status that are written by the Slow Motor Control Loop.
 */
typedef struct _mc_motor_status_slow
{
    mc_app_switch_t     eAppSwitch; /*!< Actual motor control application status (on / off) */
    float               fltSpeed;   /*!< Actual motor speed */
    mc_motor_position_t uPosition;  /*!< Actual motor position */
} mc_motor_status_slow_t;

/*!
 * @brief Holds the motor status parameters of the motor indicated by motorId.
 */
typedef struct _mc_motor_status
{
    mc_motor_status_fast_t sFast;    /*!< Part of the status that is written by the Fast Motor Control Loop */
    mc_motor_status_slow_t sSlow;    /*!< Part of the status that is written by the Slow Motor Control Loop */
    mc_motor_id_t          eMotorId; /*!< Indicates which motor this status belongs to */
} mc_motor_status_t;


/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Put one mc_motor_command_t (for a single motor) into the message queue for execution and notify the Data Hub task.
 *
 * @param[in] cmd Motor command to be queued for execution
 */
qmc_status_t MC_QueueMotorCommand(const mc_motor_command_t* cmd);

 /*!
 * @brief Get one mc_motor_status_t element from the queue, if available.
 *
 * @param[in]  handle Handle of the queue to receive the status from
 * @param[in]  timeout Timeout in milliseconds
 * @param[out] status Pointer to write the retrieved motor status to
 */
qmc_status_t MC_DequeueMotorStatus(const qmc_msg_queue_handle_t* handle, uint32_t timeout, mc_motor_status_t* status);

 /*!
 * @brief Enables or disables the execution of motor commands sent via the TSN connection.
 *
 * @param[in] isOn true = command injection enabled; false = command injection disabled
 */
void MC_SetTsnCommandInjection(bool isOn);

 /*!
 * @brief Request a new message queue handle to receive motor status messages.
 *
 * Operation may return kStatus_QMC_ErrMem if queue cannot be created or all statically created
 * queues are in use. If not every status update is required, a prescaler different from 1 can be set.
 *
 * @param[out] handle Address of the pointer to write the retrieved handle to
 * @param[in]  prescaler Decimation value, e.g. every n-th update is put in the queue.
 */
qmc_status_t MC_GetNewStatusQueueHandle(qmc_msg_queue_handle_t** handle, uint32_t prescaler);

 /*!
 * @brief Hand back a message queue handle obtained by MC_GetNewStatusQueueHandle(handle : qmc_msg_queue_handle_t*, prescaler : uint32_t) : qmc_status_t.
 *
 * @param[in] handle Pointer to the handle that is no longer in use
 */
qmc_status_t MC_ReturnStatusQueueHandle(const qmc_msg_queue_handle_t* handle);

/*!
* @brief Unfreeze a motor and tell the motor control API that it can stop ignoring further commands to the motor.
*
* @param[in] motor_id Id of the motor to be unfrozen
*/
qmc_status_t MC_UnfreezeMotor(mc_motor_id_t motor_id);


#endif /* _API_MOTORCONTROL_H_ */

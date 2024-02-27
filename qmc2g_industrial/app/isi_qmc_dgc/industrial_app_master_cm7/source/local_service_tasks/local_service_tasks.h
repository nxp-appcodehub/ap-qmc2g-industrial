/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef LOCAL_SERVICE_TASKS_H
#define LOCAL_SERVICE_TASKS_H

#include "api_qmc_common.h"

typedef enum _ls_log_label_id
{
    kLST_Log_Label1 = 0u,
	kLST_Log_Label2 = 1u,
	kLST_Log_Label3 = 2u
} ls_log_label_id_t;


typedef enum _ls_color_mode
{
    kLST_Color_Operational,
	kLST_Color_Maintenance,
	kLST_Color_Fault,
	kLST_Color_Error,
	kLST_Color_Off
} ls_color_mode_t;

typedef enum _ls_motor_label_id
{
    kLST_Motor_Label_State,
	kLST_Motor_Label_Control_En,
	kLST_Motor_Label_Speed,
	kLST_Motor_Label_Position,
	kLST_Motor_Label_Temperature,
	kLST_Motor_Label_Fault,
	kLST_Motor_Label_Phase_A_Curr,
	kLST_Motor_Label_Phase_B_Curr,
	kLST_Motor_Label_Phase_C_Curr,
	kLST_Motor_Label_Alpha_V,
	kLST_Motor_Label_Beta_V,
	kLST_Motor_Label_Db_Bus_V
} ls_motor_label_id_t;

/*!
 * @brief The main Local Service task. It takes care of the GUI and button events, monitors tampering attempts and logs them.
 *
 * @param pvParameters Unused.
 */
void LocalServiceTask(void *pvParameters);

/*!
 * @brief The init task that prepares the environment during initialization for the Local Service Task to work properly.
 *
 * @return QMC status
 */
qmc_status_t LocalServiceInit(void);

#endif /* LOCAL_SERVICE_TASKS_H */

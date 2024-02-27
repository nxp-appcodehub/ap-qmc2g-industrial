/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include "api_qmc_common.h"
#include "qmc_features_config.h"
#include "api_motorcontrol.h"
#include "plug_json.h"
#include "core_json.h"
#include "json_api_common.h"
#include "json_motor_api_service_task.h"

TaskHandle_t g_motor_api_task_handle;
SemaphoreHandle_t g_api_motor_semaphore;

/* implement a plug_json_callback */
PLUG_JSON_CB(json_motor_api);

/* information of enums used in motorcontroll than need to be used in json responses */

#define MC_APP_SWITCH(_)                      \
    _(kMC_App_Off, "off")                     \
    _(kMC_App_On, "on")                       \
    _(kMC_App_Freeze, "freeze")               \
    _(kMC_App_FreezeAndStop, "freezeAndStop") \
    /* */

#define MC_STATE(_)       \
    _(kMC_Fault, "fault") \
    _(kMC_Init, "init")   \
    _(kMC_Stop, "stop")   \
    _(kMC_Run, "run")     \
    /* */

#define MC_MOTOR_ID(_) \
    _(kMC_Motor1, "1") \
    _(kMC_Motor2, "2") \
    _(kMC_Motor3, "3") \
    _(kMC_Motor4, "4")

#define MC_METHOD_SELECTION(_)             \
    _(kMC_ScalarControl, "scalar")         \
    _(kMC_FOC_SpeedControl, "speed")       \
    _(kMC_FOC_PositionControl, "position") \
    /* */

#define MC_FAULT(_)                                   \
    _(kMC_NoFaultMC, "noFaultMC")                     \
    _(kMC_NoFaultBS, "noFaultBS")                     \
    _(kMC_OverCurrent, "overCurrent")                 \
    _(kMC_UnderDcBusVoltage, "underDcBusVoltage")     \
    _(kMC_OverDcBusVoltage, "overDcBusVoltage")       \
    _(kMC_OverLoad, "overLoad")                       \
    _(kMC_OverSpeed, "overSpeed")                     \
    _(kMC_RotorBlocked, "rotorBlocked")               \
    _(kMC_PsbOverTemperature1, "psbOverTemperature1") \
    _(kMC_PsbOverTemperature2, "psbOverTemperature2") \
    _(kMC_GD3000_OverTemperature, "overTemperature")  \
    _(kMC_GD3000_Desaturation, "desaturation")        \
    _(kMC_GD3000_LowVLS, "lowVLS")                    \
    _(kMC_GD3000_OverCurrent, "overCurrent")          \
    _(kMC_GD3000_PhaseError, "phaseError")            \
    _(kMC_GD3000_Reset, "reset")                      \
    /* */

/* create from and to string functions based on these definitions */

/* macros in common_api. */
/*                      name    ,  enum type,  enum values table macro, (storage class) */
GENERATE_ENUM_TO_STRING(mc_state, mc_state_t, MC_STATE, static);
GENERATE_ENUM_TO_STRING(mc_app_switch, mc_app_switch_t, MC_APP_SWITCH, static);
GENERATE_ENUM_FROM_STRING(mc_app_switch, mc_app_switch_t, MC_APP_SWITCH, static);
GENERATE_ENUM_TO_STRING(mc_method_selection, mc_method_selection_t, MC_METHOD_SELECTION, static);
GENERATE_ENUM_FROM_STRING(mc_method_selection, mc_method_selection_t, MC_METHOD_SELECTION, static);
GENERATE_ENUM_TO_STRING(mc_fault, mc_fault_t, MC_FAULT, static);
GENERATE_ENUM_TO_STRING(mc_motor_id, mc_motor_id_t, MC_MOTOR_ID);
GENERATE_ENUM_FROM_STRING(mc_motor_id, mc_motor_id_t, MC_MOTOR_ID, static);

/**
 * @brief print a motor position json representation
 *
 * @param mpos motor position
 * @param response output FILE
 *
 * @return kStatus_QMC_Ok | kStatus_QMC_Err
 */
static qmc_status_t json_motor_position(mc_motor_position_t *mpos, FILE *response)
{
    int rc = fprintf(response, "{\"numOfTurns\":%" PRIi16 ",\"rotorPosition\":%" PRIu16 "}",
                     mpos->sPosValue.i16NumOfTurns, mpos->sPosValue.ui16RotorPosition);
    return rc < 0 ? kStatus_QMC_Err : kStatus_QMC_Ok;
}

/**
 * @brief print a motor_status_fast json object
 *
 * @param msf mc_motor_status_fast_t structure pointer
 * @param response output FILE
 *
 * @return kStatus_QMC_Ok on success
 * @return kStatus_QMC_ErrArgInvalid if any string conversions fail
 * @return kStatus_QMC_Err on error
 */
static qmc_status_t json_motor_status_fast(mc_motor_status_fast_t *msf, FILE *response)
{
    const char *mc_state = mc_state_to_string(msf->eMotorState);
    const char *mc_fault = mc_fault_to_string(msf->eFaultStatus);
    if (!*mc_state || !*mc_fault)
    {
        return kStatus_QMC_ErrArgInvalid;
    }
    int rc = fprintf(response,
                     "{"
                     "\"state\":\"%s\", "
                     "\"faultStatus\":\"%s\", "
                     "\"ia\":%.8f, "
                     "\"ib\":%.8f, "
                     "\"ic\":%.8f, "
                     "\"valpha\":%.8f, "
                     "\"vbeta\":%.8f, "
                     "\"vDcBus\":%.8f}",

                     mc_state, mc_fault, (double)msf->fltIa, (double)msf->fltIb, (double)msf->fltIc,
                     (double)msf->fltValpha, (double)msf->fltVbeta, (double)msf->fltVDcBus);
    return rc < 0 ? kStatus_QMC_Err : kStatus_QMC_Ok;
}

/**
 * @brief print a motor status slow json representation
 *
 * @param mss mc_motor_status_slow_t pointer
 * @param response output FILE
 *
 * @return kStatus_QMC_Ok on success
 * @return kStatus_QMC_ErrArgInvalid if any string conversions fail
 * @return kStatus_QMC_Err on error
 */
static qmc_status_t json_motor_status_slow(mc_motor_status_slow_t *mss, FILE *response)
{
    int rc                 = -1;
    const char *app_switch = mc_app_switch_to_string(mss->eAppSwitch);
    qmc_status_t status    = kStatus_QMC_ErrArgInvalid;
    do
    {
        if (!*app_switch)
        {
            break;
        }
        rc     = fprintf(response,
                         "{"
                             "\"app\":\"%s\""
                             ", \"speed\":%.8f"
                             ", \"motorPosition\":",
                         app_switch, (double)mss->fltSpeed);
        status = json_motor_position(&mss->uPosition, response);
        if (rc < 0)
        {
            break;
        }
        fputc('}', response);
    } while (0);
    return rc < 0 ? kStatus_QMC_Err : status;
}

/**
 * @brief print a motor status json representation
 *
 * @param ms mc_motor_status_t pointer
 * @param response output FILE
 *
 * @return kStatus_QMC_Ok on success
 * @return kStatus_QMC_ErrArgInvalid if any string conversions fail
 * @return kStatus_QMC_Err on error
 */
static qmc_status_t json_motor_status(mc_motor_status_t *ms, FILE *response)
{
    const char *motor   = mc_motor_id_to_string(ms->eMotorId);
    qmc_status_t status = kStatus_QMC_ErrArgInvalid;
    do
    {
        if (!*motor)
        {
            break;
        }
        status = kStatus_QMC_Err;
        if (fputs("{\"id\":", response) < 0)
        {
            break;
        }
        if (fputs(motor, response) < 0)
        {
            break;
        }
        if (fputs(", \"fast\":", response) < 0)
        {
            break;
        }
        if ((status = json_motor_status_fast(&ms->sFast, response)) != kStatus_QMC_Ok)
        {
            fputs("null", response);
        }
        status = kStatus_QMC_Err;
        if (fputs(", \"slow\":", response) < 0)
        {
            break;
        }
        if ((status = json_motor_status_slow(&ms->sSlow, response)) != kStatus_QMC_Ok)
        {
            fputs("null", response);
        }
        status = kStatus_QMC_Err;
        if (fputc('}', response) < 0)
        {
            break;
        }
        status = kStatus_QMC_Ok;
    } while (0);
    return kStatus_QMC_Ok;
}

/**
 * @brief print a json representation of a motor command
 *
 * @param mc mc_motor_command_t pointer
 * @param response output FILE
 *
 * @return kStatus_QMC_Ok on success
 * @return kStatus_QMC_ErrArgInvalid if any string conversions fail
 * @return kStatus_QMC_Err on error
 */
static qmc_status_t json_motor_command(mc_motor_command_t *mc, FILE *response)
{
    int rc                 = -1;
    qmc_status_t status    = kStatus_QMC_ErrArgInvalid;
    const char *app_switch = mc_app_switch_to_string(mc->eAppSwitch);
    const char *method     = mc_method_selection_to_string(mc->eControlMethodSel);
    if (*app_switch && *method)
    {
        do
        {
            if ((rc = fprintf(response, "{\"app\":\"%s\", \"controlMethod\":\"%s\", ", app_switch, method)) < 0)
            {
                break;
            }
            switch (mc->eControlMethodSel)
            {
                case kMC_ScalarControl:
                    rc = fprintf(response, "\"gain\":%f, \"frequency\":%f}",
                                 (double)mc->uSpeed_pos.sScalarParam.fltScalarControlVHzGain,
                                 (double)mc->uSpeed_pos.sScalarParam.fltScalarControlFrequency);
                    break;
                case kMC_FOC_SpeedControl:
                    rc = fprintf(response, "\"speed\":%f}", (double)mc->uSpeed_pos.fltSpeed);
                    break;
                case kMC_FOC_PositionControl:
                    rc = fprintf(response,
                                 "\"motorPosition\":{\"numOfTurns\":%" PRIi16 ",\"rotorPosition\":%" PRIu16
                                 "}, \"isRandom\":%s}",
                                 mc->uSpeed_pos.sPosParam.uPosition.sPosValue.i16NumOfTurns,
                                 mc->uSpeed_pos.sPosParam.uPosition.sPosValue.ui16RotorPosition,
                                 mc->uSpeed_pos.sPosParam.bIsRandomPosition ? "true" : "false");
                default:
                    break;
            }
        } while (false);
    }
    return rc < 0 ? kStatus_QMC_Err : status;
}

/**
 * @brief motor api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */
http_status_t json_motor_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    int code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
    qmc_status_t status;
    mc_motor_id_t motor = 0;

    mc_motor_command_t mcmd;
    char *value          = NULL;
    size_t value_len     = 0;
    char *value_end      = NULL;
    JSONTypes_t type     = 0;
    JSONStatus_t jstatus = 0;
    float number         = 0.0f;

    bool one_motor = request->path_match_end[0] != '\0';

    if (one_motor && !mc_motor_id_from_string(&motor, &request->path_match_end[0], 1))
    {
        return HTTP_NOREPLY_NOT_FOUND;
    };
    if (one_motor && motor >= MC_MAX_MOTORS)
    {
        return HTTP_NOREPLY_NOT_FOUND;
    }
    status = kStatus_QMC_ErrArgInvalid;
    switch (request->method)
    {
        case HTTP_GET:
            if (xSemaphoreTake(g_api_motor_semaphore, pdMS_TO_TICKS(MOTOR_STATUS_AND_LOGS_DELAY_AT_LEAST_MS)) == pdTRUE)
            {
                if (one_motor)
                {
                    status = json_motor_status(&g_api_motor_status[motor], response);
                    if (status == kStatus_QMC_Ok)
                    {
                        code = HTTP_REPLY_OK;
                    }
                }
                else
                {
                    fputc('[', response);
                    do
                    {
                        if (motor > 0)
                            fputc(',', response);
                        json_motor_status(&g_api_motor_status[motor], response);
                    } while (++motor < MC_MAX_MOTORS);
                    fputc(']', response);
                    code = HTTP_REPLY_OK;
                }
                xSemaphoreGive(g_api_motor_semaphore);
            }
            else
            {
                code = HTTP_NOREPLY_SERVICE_UNAVAILABLE;
            }
            break;
        case HTTP_POST:
        case HTTP_PUT:
            code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            if (!one_motor)
            {
                break;
            }
            else
            {
                mcmd.eMotorId = motor;
            }
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("app"), &value, &value_len, &type);
            switch (jstatus)
            {
                case JSONNotFound:
                    /* no "app": key provided, copy the current appSwitch setting */
                    mcmd.eAppSwitch = g_api_motor_status[motor].sSlow.eAppSwitch;
                    status          = kStatus_QMC_Ok;
                    break;
                case JSONSuccess:
                    /* "app": value needs verification */
                    status = kStatus_QMC_ErrArgInvalid;
                    if (type != JSONString)
                    {
                        break;
                    }
                    if (!mc_app_switch_from_string(&mcmd.eAppSwitch, value, value_len))
                    {
                        break;
                    }
                    status = kStatus_QMC_Ok;
                default:
                    break;
            }
            if (status != kStatus_QMC_Ok)
            {
                break;
            }
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("controlMethod"), &value, &value_len, &type);
            switch (jstatus)
            {
                case JSONSuccess:
                    status = kStatus_QMC_ErrArgInvalid;
                    if (type != JSONString)
                    {
                        break;
                    }
                    if (!mc_method_selection_from_string(&mcmd.eControlMethodSel, value, value_len))
                    {
                        break;
                    }
                    status = kStatus_QMC_Ok;
                    break;
                default:
                    status = kStatus_QMC_ErrArgInvalid;
                    break;
            }
            if (status != kStatus_QMC_Ok)
            {
                break;
            }
            switch (mcmd.eControlMethodSel)
            {
                case kMC_ScalarControl:
                    jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("gain"), &value, &value_len, &type);
                    switch (jstatus)
                    {
                        case JSONSuccess:
                            status = kStatus_QMC_ErrArgInvalid;
                            if (type != JSONNumber)
                            {
                                break;
                            }
                            number = strntof(value, value_len, (const char **)&value_end);
                            if (value_end < (value + value_len) || isnan(number))
                            {
                                break;
                            }
                            mcmd.uSpeed_pos.sScalarParam.fltScalarControlVHzGain = number;
                            status                                               = kStatus_QMC_Ok;
                            break;
                        default:
                            status = kStatus_QMC_ErrArgInvalid;
                            break;
                    }
                    if (status != kStatus_QMC_Ok)
                    {
                        break;
                    }
                    jstatus =
                        JSON_SearchT((char *)json_body, json_body_len, SIZED("frequency"), &value, &value_len, &type);
                    switch (jstatus)
                    {
                        case JSONSuccess:
                            status = kStatus_QMC_ErrArgInvalid;
                            if (type != JSONNumber)
                            {
                                break;
                            }
                            number = strntof(value, value_len, (const char **)&value_end);
                            if (value_end < (value + value_len) || isnan(number))
                            {
                                break;
                            }
                            mcmd.uSpeed_pos.sScalarParam.fltScalarControlFrequency = number;
                            status                                                 = kStatus_QMC_Ok;
                            break;
                        default:
                            status = kStatus_QMC_ErrArgInvalid;
                            break;
                    }
                    break;
                case kMC_FOC_PositionControl:
                    jstatus =
                        JSON_SearchT((char *)json_body, json_body_len, SIZED("isRandom"), &value, &value_len, &type);
                    status = kStatus_QMC_ErrArgInvalid;
                    switch (jstatus)
                    {
                        case JSONSuccess:
                            if (type != JSONTrue && type != JSONFalse)
                            {
                                break;
                            }
                            mcmd.uSpeed_pos.sPosParam.bIsRandomPosition = (type == JSONTrue);
                            status                                      = kStatus_QMC_Ok;
                            break;
                        default:
                            break;
                    }
                    if (status != kStatus_QMC_Ok)
                    {
                        break;
                    }
                    jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("position.rotorPosition"), &value,
                                           &value_len, &type);
                    status  = kStatus_QMC_ErrArgInvalid;
                    switch (jstatus)
                    {
                        case JSONSuccess:
                            if (type != JSONNumber)
                            {
                                break;
                            }
                            number = strntof(value, value_len, (const char **)&value_end);
                            if (value_end < (value + value_len) || isnan(number) || number < 0 || number > UINT16_MAX)
                            {
                                break;
                            }
                            mcmd.uSpeed_pos.sPosParam.uPosition.sPosValue.ui16RotorPosition = (uint16_t)number;
                            status                                                          = kStatus_QMC_Ok;
                            break;
                        default:
                            break;
                    }
                    if (status != kStatus_QMC_Ok)
                    {
                        break;
                    }
                    status  = kStatus_QMC_ErrArgInvalid;
                    jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("position.numOfTurns"), &value,
                                           &value_len, &type);
                    switch (jstatus)
                    {
                        case JSONSuccess:
                            if (type != JSONNumber)
                            {
                                break;
                            }
                            number = strntof(value, value_len, (const char **)&value_end);
                            if (value_end < (value + value_len) || isnan(number) || number < INT16_MIN ||
                                number > INT16_MAX)
                            {
                                break;
                            }
                            mcmd.uSpeed_pos.sPosParam.uPosition.sPosValue.i16NumOfTurns = (int16_t)number;
                            status                                                      = kStatus_QMC_Ok;
                            break;
                        default:
                            break;
                    }
                    break;
                case kMC_FOC_SpeedControl:
                    status  = kStatus_QMC_ErrArgInvalid;
                    jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("speed"), &value, &value_len, &type);
                    switch (jstatus)
                    {
                        case JSONSuccess:
                            if (type != JSONNumber)
                            {
                                break;
                            }
                            number = strntof(value, value_len, (const char **)&value_end);
                            if (value_end < (value + value_len) || isnan(number))
                            {
                                break;
                            }
                            mcmd.uSpeed_pos.fltSpeed = number;
                            status                   = kStatus_QMC_Ok;
                            break;
                        default:
                            break;
                    }
                    break;
            }
            if (status != kStatus_QMC_Ok)
            {
                break;
            }
            status = MC_QueueMotorCommand(&mcmd);
            // ensure we send a reply with the error code
            code = HTTP_REPLY_UNPROCESSABLE_ENTITY;
            if (webservice_check_error(status, response))
            {
                code = HTTP_REPLY_OK;
                json_motor_command(&mcmd, response);
            }
            break;
        default:
            status = kStatus_QMC_ErrArgInvalid;
            // ensure we send a reply with the error code
            code = HTTP_REPLY_UNPROCESSABLE_ENTITY;
            (void)webservice_check_error(status, response);
            break;
    }
    return code;
}

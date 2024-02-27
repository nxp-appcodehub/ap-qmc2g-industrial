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
#include <stdint.h>

#include "api_logging.h"
#include "api_motorcontrol.h"
#include "api_usermanagement.h"
#include "api_qmc_common.h"
#include "plug_json.h"
#include "json_api_common.h"
#include "json_string.h"

#include "webservice/json_api_common.h"

PLUG_JSON_CB(json_log_api);

/**
 * @brief seek to a variable in an query string
 *
 * @param qstring the query string
 * @param vname name of the variable to locate
 *
 * @return pointer to the character after the variable name ('&' or '=' or '\0')
 *         or NULL if not found
 */
static const char *get_query_var(const char *qstring, const char *vname)
{
	const char *n, *q;
	q=qstring;
	while(*q != '\0')
	{
		// start at the variable name
		n=vname;

		// advance while the name matches and the name is not complete
		while(*n != '\0' && *n == *q)
		{
			n++;
			q++;
		};

		// if name is complete return the position in qstring
		if(*n == '\0' && ( *q == '=' || *q == '\0' || *q == '&'))
		{
			return q;
		}
		// otherwise skip to next variable in qstring
		while(*q != '\0' && *q++ != '&');
	}
	return NULL;
}


/* definition of string representations for log_source_id_t */
#define LOG_SOURCE(_)                                                                   \
    _(LOG_SRC_Unspecified, "unspecified")                                               \
    _(LOG_SRC_Webservice, "webservice")                                                 \
    _(LOG_SRC_FaultHandling, "faultHandling")                                           \
    _(LOG_SRC_CloudService, "cloudService")                                             \
    _(LOG_SRC_LocalService, "localService")                                             \
    _(LOG_SRC_BoardService, "boardService")                                             \
    _(LOG_SRC_AnomalyDetection, "anomalyDetection")                                     \
    _(LOG_SRC_MotorControl, "motorControl")                                             \
    _(LOG_SRC_SecureWatchdog, "secureWatchdog")                                         \
    _(LOG_SRC_TaskStartup, "taskStartup")                                               \
    _(LOG_SRC_RpcModule, "rpcModule")                                                   \
    _(LOG_SRC_SecureWatchdogServiceRequestNonce, "secureWatchdogServiceRequestNonce")   \
    _(LOG_SRC_SecureWatchdogServiceRequestTicket, "secureWatchdogServiceRequestTicket") \
    _(LOG_SRC_SecureWatchdogServiceKick, "secureWatchdogServiceKick")                   \
    _(LOG_SRC_FunctionalWatchdog, "functionalWatchdog")                                 \
    _(LOG_SRC_PowerLossInterrupt, "powerLossInterrupt")                                 \
    _(LOG_SRC_LoggingService, "loggingService")                                         \
    _(LOG_SRC_TSN, "TSN")                                                               \
    _(LOG_SRC_DataHub, "dataHub")                                                       \
    _(LOG_SRC_SecureBootloader, "secureBootloader")                                                       \
    _(LOG_SRC_UsrMgmt, "usrMgmt")                                                       \
    /* */

/* definition of string representations for log_category_id_t */
#define LOG_CATEGORY(_)                         \
    _(LOG_CAT_General, "general")               \
    _(LOG_CAT_Fault, "fault")                   \
    _(LOG_CAT_Authentication, "authentication") \
    _(LOG_CAT_Connectivity, "connectivity")     \
    /* */

/* string representations for log event codes */
#define LOG_EVENT_CODE(_)                                                          \
    _(LOG_EVENT_AfeDbCommunicationError, "afe db communication error")             \
    _(LOG_EVENT_AfePsbCommunicationError, "afe psb communication error")           \
    _(LOG_EVENT_DBTempSensCommunicationError, "db temp sens communication error")  \
    _(LOG_EVENT_DbOverTemperature, "db over temperature")                          \
    _(LOG_EVENT_EmergencyStop, "emergency stop")                                   \
    _(LOG_EVENT_FaultBufferOverflow, "fault buffer overflow")                      \
    _(LOG_EVENT_FaultQueueOverflow, "fault queue overflow")                        \
    _(LOG_EVENT_GD3000_Desaturation, "desaturation")                               \
    _(LOG_EVENT_GD3000_LowVLS, "low vls")                                          \
    _(LOG_EVENT_GD3000_OverCurrent, "over current")                                \
    _(LOG_EVENT_GD3000_OverTemperature, "over temperature")                        \
    _(LOG_EVENT_GD3000_PhaseError, "phase error")                                  \
    _(LOG_EVENT_GD3000_Reset, "reset")                                             \
    _(LOG_EVENT_InvalidFaultSource, "invalid fault source")                        \
    _(LOG_EVENT_McuOverTemperature, "mcu over temperature")                        \
    _(LOG_EVENT_NoFault, "no fault")                                               \
    _(LOG_EVENT_NoFaultBS, "no fault bs")                                          \
    _(LOG_EVENT_NoFaultMC, "no fault mc")                                          \
    _(LOG_EVENT_OverCurrent, "over current")                                       \
    _(LOG_EVENT_OverDcBusVoltage, "over dc bus voltage")                           \
    _(LOG_EVENT_OverLoad, "over load")                                             \
    _(LOG_EVENT_OverSpeed, "over speed")                                           \
    _(LOG_EVENT_PmicOverTemperature, "pmic over temperature")                      \
    _(LOG_EVENT_PmicUnderVoltage, "pmic under voltage")                            \
    _(LOG_EVENT_PsbOverTemperature1, "psb over temperature1")                      \
    _(LOG_EVENT_PsbOverTemperature2, "psb over temperature2")                      \
    _(LOG_EVENT_RotorBlocked, "rotor blocked")                                     \
    _(LOG_EVENT_UnderDcBusVoltage, "under dc bus voltage")                         \
    _(LOG_EVENT_Button1Pressed, "button 1 pressed")                                \
    _(LOG_EVENT_Button2Pressed, "button 2 pressed")                                \
    _(LOG_EVENT_Button3Pressed, "button 3 pressed")                                \
    _(LOG_EVENT_Button4Pressed, "button 4 pressed")                                \
    _(LOG_EVENT_EmergencyButtonPressed, "emergency button pressed")                \
    _(LOG_EVENT_LidOpenButton, "lid open button")                                  \
    _(LOG_EVENT_LidOpenSd, "lid open sd")                                          \
    _(LOG_EVENT_TamperingButton, "tampering button")                               \
    _(LOG_EVENT_TamperingSd, "tampering sd")                                       \
    _(LOG_EVENT_ResetSecureWatchdog, "reset secure watchdog")                      \
    _(LOG_EVENT_ResetFunctionalWatchdog, "reset functional watchdog")              \
    _(LOG_EVENT_FunctionalWatchdogInitFailed, "functional watchdog init failed")   \
    _(LOG_EVENT_FunctionalWatchdogKickFailed, "functional watchdog kick failed")   \
    _(LOG_EVENT_PowerLoss, "power loss")                                           \
    _(LOG_EVENT_AccountResumed, "account resumed")                                 \
    _(LOG_EVENT_AccountSuspended, "account suspended")                             \
    _(LOG_EVENT_LoginFailure, "login failure")                                     \
    _(LOG_EVENT_SessionTimeout, "session timeout")                                 \
    _(LOG_EVENT_TerminateSession, "terminate session")                             \
    _(LOG_EVENT_UserLogin, "user login")                                           \
    _(LOG_EVENT_UserLogout, "user logout")                                         \
    _(LOG_EVENT_QueueingCommandFailedInternal, "queueing command failed internal") \
    _(LOG_EVENT_QueueingCommandFailedTSN, "queueing command failed tsn")           \
    _(LOG_EVENT_QueueingCommandFailedQueue, "queueing command failed queue")       \
    _(LOG_EVENT_ResetRequest, "reset request")                                     \
    _(LOG_EVENT_InvalidResetCause, "invalid reset cause")                          \
    _(LOG_EVENT_InvalidArgument, "invalid argument")                               \
    _(LOG_EVENT_RPCCallFailed, "rpc call failed")                                  \
    _(LOG_EVENT_AWDTExpired, "awdt expired")                                       \
    _(LOG_EVENT_SignatureInvalid, "signature invalid")                             \
    _(LOG_EVENT_Timeout, "timeout")                                                \
    _(LOG_EVENT_SyncError, "sync error")                                           \
    _(LOG_EVENT_InternalError, "internal error")                                   \
    _(LOG_EVENT_NoBufsError, "no bufs error")                                      \
    _(LOG_EVENT_ConnectionError, "connection error")                               \
    _(LOG_EVENT_RequestError, "request error")                                     \
    _(LOG_EVENT_JsonParsingError, "json parsing error")                            \
    _(LOG_EVENT_RangeError, "range error")                                         \
	_(LOG_EVENT_Scp03ConnFailed, "scp03 conn failed")                              \
    _(LOG_EVENT_Scp03KeyReconFailed, "scp03 key recon failed")                     \
    _(LOG_EVENT_NewFWReverted, "new fw reverted")                                  \
    _(LOG_EVENT_NewFWRevertFailed, "new fw revert failed")                         \
    _(LOG_EVENT_NewFWCommitted, "new fw committed")                                \
    _(LOG_EVENT_NewFWCommitFailed, "new fw commit failed")                         \
    _(LOG_EVENT_AwdtExpired, "awdt expired")                                       \
    _(LOG_EVENT_CfgDataBackedUp, "cfg data backed up")                             \
    _(LOG_EVENT_CfgDataBackUpFailed, "cfg data back up failed")                    \
    _(LOG_EVENT_MainFwAuthFailed, "main fw auth failed")                           \
    _(LOG_EVENT_FwuAuthFailed, "fwu auth failed")                                  \
    _(LOG_EVENT_StackError, "stack error")                                         \
    _(LOG_EVENT_KeyRevocation, "key revocation")                                   \
    _(LOG_EVENT_InvalidFwuVersion, "invalid fwu version")                          \
    _(LOG_EVENT_ExtMemOprFailed, "ext mem opr failed")                             \
    _(LOG_EVENT_BackUpImgAuthFailed, "back up img auth failed")                    \
    _(LOG_EVENT_SdCardFailed, "sd card failed")                                    \
    _(LOG_EVENT_HwInitDeinitFailed, "hw init deinit failed")                       \
    _(LOG_EVENT_SvnsLpGprOpFailed, "snvs lpgpr op failed")                         \
    _(LOG_EVENT_Scp03KeyRotationFailed, "scp03 key rotation failed")               \
    _(LOG_EVENT_DecommissioningFailed, "decommissioning failed")                   \
    _(LOG_EVENT_VerReadFromSeFailed, "ver read from se failed")                    \
    _(LOG_EVENT_FwExecutionFailed, "fw execution failed")                          \
    _(LOG_EVENT_FwuCommitFailed, "fwu commit failed")                              \
    _(LOG_EVENT_DeviceDecommissioned, "device decommissioned")                     \
    _(LOG_EVENT_RpcInitFailed, "rpc init failed")                                  \
    _(LOG_EVENT_UnknownFWReturnStatus, "unknown fw return status")                 \
    _(LOG_EVENT_NoLogEntry, "no log entry")                                        \
    _(LOG_EVENT_UserCreated, "user created")                                       \
    _(LOG_EVENT_UserUpdate, "user update")                                         \
    _(LOG_EVENT_UserRemoved, "user removed")                                       \
/* */

/* generate _to_string functions for those enums */
GENERATE_ENUM_TO_STRING(log_source, log_source_id_t, LOG_SOURCE, static);
GENERATE_ENUM_TO_STRING(log_category, log_category_id_t, LOG_CATEGORY, static);
GENERATE_ENUM_TO_STRING(log_event_code, log_event_code_t, LOG_EVENT_CODE, static);

extern const char *mc_motor_id_to_string(mc_motor_id_t motor_id);

/**
 * @brief log record formating helper
 *
 * @param r log record
 * @param response output file
 */
static void print_json_log_record(log_record_t *r, FILE *response)
{
    static char buffer[512];
    const char *source = "", *category = "", *event = "", *motor;
    log_event_code_t event_code = 0u;
    int rc;
    qmc_timestamp_t ts =
        r->rhead.ts; /* clone r->rhead.ts, it's in a packed struct and a pointer to it might be unaligned */

    fprintf(response, "{\"id\":%" PRIu32 ", \"ts\":", r->rhead.uuid);
    print_json_timestamp(&ts, response);
    switch (r->type)
    {
        case kLOG_DefaultData:
            /*fallthrough*/
        case kLOG_SystemData:
            source     = log_source_to_string(r->data.defaultData.source);
            category   = log_category_to_string(r->data.defaultData.category);
            event_code = r->data.defaultData.eventCode;
            event      = log_event_code_to_string(r->data.defaultData.eventCode);
            break;
        case kLOG_FaultDataWithID:
            source     = log_source_to_string(r->data.faultDataWithID.source);
            category   = log_category_to_string(r->data.faultDataWithID.category);
            event_code = r->data.faultDataWithID.eventCode;
            event      = log_event_code_to_string(r->data.faultDataWithID.eventCode);
            break;
        case kLOG_FaultDataWithoutID:
            source     = log_source_to_string(r->data.faultDataWithoutID.source);
            category   = log_category_to_string(r->data.faultDataWithoutID.category);
            event_code = r->data.faultDataWithoutID.eventCode;
            event      = log_event_code_to_string(r->data.faultDataWithoutID.eventCode);
            break;
        case kLOG_ErrorCount:
            source     = log_source_to_string(r->data.errorCount.source);
            category   = log_category_to_string(r->data.errorCount.category);
            event_code = r->data.errorCount.errorCode;
            event      = status_code_string(r->data.errorCount.errorCode);
            break;
        case kLOG_UsrMgmt:
            source     = log_source_to_string(r->data.usrMgmt.source);
            category   = log_category_to_string(r->data.usrMgmt.category);
            event_code = r->data.usrMgmt.eventCode;
            event      = log_event_code_to_string(r->data.errorCount.errorCode);
            break;
        default:
            break;
    }
    if (!event)
    {
        event = status_code_string(500);
    }
    fprintf(response, ", \"src\":\"%s\", \"cat\":\"%s\", \"code\":%u, \"event\":", source, category,
            (unsigned int)event_code);
    switch (r->type)
    {
        case kLOG_DefaultData:
            fputs_json_string(event, response);
            fprintf(response, ", \"uid\":%u}", r->data.defaultData.user);
            break;
        case kLOG_ErrorCount:
            rc = snprintf(buffer, sizeof(buffer), "%u %s [%u]",r->data.errorCount.errorCode,event, (unsigned int)r->data.errorCount.count);
            fputs_json_string(rc > 0 ? buffer : event, response);
            fprintf(response, ",\"statusCode\":%u, \"count\":%u, \"uid\":%u}",r->data.errorCount.errorCode, r->data.errorCount.count, r->data.errorCount.user);
            break;
        case kLOG_UsrMgmt:
            fputs_json_string(event, response);
            fprintf(response, ",\"uid\":%u, \"subject\":%u}", r->data.usrMgmt.user, r->data.usrMgmt.subject);
            break;
        case kLOG_FaultDataWithID:
            fputs_json_string(event, response);
            motor = mc_motor_id_to_string((mc_motor_id_t)r->data.faultDataWithID.id);
            if (r->data.faultDataWithID.eventCode == LOG_EVENT_PmicUnderVoltage)
            {
                fprintf(response, " ,\"id\":%s}", motor);
            }
            else
            {
                fprintf(response, " ,\"motor\":%s}", motor);
            }
            break;
        case kLOG_SystemData:
            /*fallthrough*/
        case kLOG_FaultDataWithoutID:
            fputs_json_string(event, response);
            fputc('}', response);
            break;
        default:
            fputs("\"unknown event type\"}", response);
            break;
    };
}

enum
{
    BASE_TEN = 10
};
/**
 * @brief log api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */
http_status_t json_log_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    qmc_status_t status;
    static uint32_t const page_size = 50;
    uint32_t start                  = 0;
    uint32_t last                   = 0;
    uint32_t id                     = 0;
    uint32_t pageend                = 0;
    char const *pvar                = NULL;
    log_record_t record;
    usrmgmt_session_t *user_session = request->session;

    if (!user_session)
        return HTTP_NOREPLY_UNAUTHORIZED;
    if (!user_session || user_session->role != kUSRMGMT_RoleMaintenance)
    	return HTTP_NOREPLY_FORBIDDEN;

    start = LOG_GetLastLogId();
    last  = 0;

    pvar = get_query_var(request->query_string, "pre");
    if (pvar && *pvar++ == '=')
    {
        unsigned long long num;
        num = strtoull(pvar, NULL, BASE_TEN);
        if (num > UINT32_MAX)
            return HTTP_NOREPLY_BAD_REQUEST;
        start = num<2?1:num-1;
    }
    pvar = get_query_var(request->query_string, "last");
    if (pvar && *pvar++ == '=')
    {
        unsigned long long num;
        num = strtoull(pvar, NULL, BASE_TEN);
        if (num > UINT32_MAX)
            return HTTP_NOREPLY_BAD_REQUEST;
        last = num;
    }

    pageend = start > page_size ? start - page_size : 1;
    if (pageend <= last)
    {
        pageend = last + 1;
    }

    fputs("[", response);
    for (id = start; id >= pageend; id--)
    {
        if (id != start)
        {
            fputs(",\n", response);
        }
        status = LOG_GetLogRecord(id, &record);
        if (status != kStatus_QMC_Ok)
        {
            fputs("null", response);
        }
        print_json_log_record(&record, response);
    }
    fputs("]\n", response);
    /* print an etag to the buffer, and assign the etag pointer to it */
    int count = snprintf(state->buffer, state->buffer_size, "W/\"LOG-%lu-%lu\"", (long unsigned int)id,
                         (long unsigned int)start);
    if (count < state->buffer_size)
    {
        state->etag = state->buffer;
    }

    return HTTP_REPLY_OK;
}

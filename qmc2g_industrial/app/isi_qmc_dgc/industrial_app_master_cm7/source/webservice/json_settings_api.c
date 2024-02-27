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

#include "api_configuration.h"
#include "api_qmc_common.h"
#include "api_usermanagement.h"
#include "plug_json.h"
#include "json_string.h"
#include "core_json.h"
#include "json_api_common.h"

#include "qmc_features_config.h"

PLUG_JSON_CB(json_settings_api);

/*!
 * @brief named config parameters, shown in the UI, including type and mapping
 * */
static const char *s_config_key_list[][2] = {{"IP_config", "IP address"},
                                             {"IP_mask_config", "IP mask"},
                                             {"IP_gateway", "Default gateway"},
                                             {"IP_DNS", "DNS server address"},
                                             {"MAC_address", "Ethernet MAC address"},
                                             {"TSN_VLAN_ID", "TSN vLAN ID"},
                                             {"TSN_RX_Stream_MAC", "TSN RX stream MAC address"},
                                             {"TSN_TX_Stream_MAC", "TSN TX stream MAC address"},
                                             {"MOTD", "System Usage Message"},
											 {"AZURE_IOTHUB_HubName", "Azure IoT Hub name"},
                                             {NULL, NULL}};

/**
 * @brief settings api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */

http_status_t json_settings_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    int code                                      = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
    qmc_status_t status                           = kStatus_QMC_Err;
    unsigned char value[CONFIG_MAX_VALUE_LEN + 1] = {0};

    size_t length;
    char *new_value;
    JSONTypes_t type;

    JSONStatus_t jstatus;
    int i, j;

    usrmgmt_session_t *user_session = request->session;
           
    switch (request->method)
    {
        case HTTP_PUT:
            if (!user_session)
            {
                code = HTTP_NOREPLY_UNAUTHORIZED;
                break;
            }
            if (request->path_match_end == NULL || request->path_match_end[0] == '\0')
            {
                /* empty pattern suffix -> no key given */
                code = HTTP_NOREPLY_NOT_FOUND;
                break;
            }
            if (user_session->role != kUSRMGMT_RoleMaintenance)
            {
                code = HTTP_NOREPLY_FORBIDDEN;
                break;
            }
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("format"), &new_value, &length, &type);
            if (jstatus != JSONSuccess)
            {
                break;
            }
            if (type != JSONString)
            {
                break;
            }
            if (strncmp(new_value, "binary", 6) != 0)
            {
                break;
            }
            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("value"), &new_value, &length, &type);
            if (jstatus != JSONSuccess)
            {
                break;
            }
            if (type != JSONString)
            {
                break;
            }
            if (length % 2 || length > CONFIG_MAX_VALUE_LEN * 2)
            {
                break;
            }
            for (i = 0, j = 0; i < length; i += 2, j++)
            {
                if (!isxdigit((unsigned char)new_value[i]) || !isxdigit((unsigned char)new_value[i + 1]))
                {
                    break;
                }
                /* set the value, first four bit from first char second from second */
                value[j] = HEX_VALUE(new_value[i]) << 4 | HEX_VALUE(new_value[i + 1]);
            }
            status = CONFIG_SetBinValue((unsigned char *)request->path_match_end , &value[0], CONFIG_MAX_VALUE_LEN);
            if (status == kStatus_QMC_Ok)
            {
                status = CONFIG_UpdateFlash();
            }
            if (!webservice_check_error(status, response))
            {
                break;
            }
            /* fallthru */
        case HTTP_GET:
            if (request->path_match_end == NULL || request->path_match_end[0] == '\0')
            {
                const char *(*key)[2];
                code = HTTP_REPLY_OK;
                fputs("[\n  ", response);
                for (key = &s_config_key_list[0]; (*key)[0]; key++)
                {
                    if (key != &s_config_key_list[0])
                    {
                        fputs(",\n  ", response);
                    }
                    status = CONFIG_GetBinValue((unsigned char *)(*key)[0], &value[0], CONFIG_MAX_VALUE_LEN);
                    if (status != kStatus_QMC_Ok)
                    {
                        /* internal id is probably wrong */
                        fputs("null", response);
                        continue;
                    };
                    fputs("{\"key\":", response);
                    fputs_json_string((*key)[0], response);
                    if ((*key)[1])
                    {
                        fputs(", \"description\":", response);
                        fputs_json_string((*key)[1], response);
                    }
                    fputs(", \"format\":\"binary\", \"value\":\"", response);
                    for (j = CONFIG_MAX_VALUE_LEN; j > 0 && value[j] == 0;)
                    {
                        j--;
                    }
                    for (i = 0; i <= j; i++)
                    {
                        fprintf(response, "%02X", value[i]);
                    }
                    fputs("\"}", response);
                }
                fputs("\n]", response);
            }
            else
            {
                config_id_t conf_id;
                /* lookup the config value, */
                conf_id = CONFIG_GetIdfromKey((unsigned char *)request->path_match_end);
                if (conf_id == kCONFIG_KeyNone)
                {
                    code = HTTP_NOREPLY_NOT_FOUND;
                    break;
                }

                status = CONFIG_GetBinValueById(conf_id, &value[0], CONFIG_MAX_VALUE_LEN);

                code = HTTP_REPLY_OK;
                /* ignore user secrets */
                if (webservice_check_error(status, response))
                {
                    fputs("{\"key\":", response);
                    fputs_json_string(request->path_match_end, response);

                    if ((conf_id >= kCONFIG_Key_UserFirst && conf_id <= kCONFIG_Key_UserLast) ||
                        (conf_id >= kCONFIG_Key_UserHashesFirst && conf_id <= kCONFIG_Key_UserHashesLast))
                    {
                        fputs(", \"format\":\"hidden\"}", response);
                    }
                    else
                    {
                        fputs(", \"format\":\"binary\", \"value\":\"", response);
                        for (j = CONFIG_MAX_VALUE_LEN; j > 0 && value[j] == 0;)
                        {
                            j--;
                        }
                        for (i = 0; i <= j; i++)
                        {
                            fprintf(response, "%02X", value[i]);
                        }
                        fputs("\"}", response);
                    }
                }
            }
            break;
        default:
            break;
    }
    return code;
}

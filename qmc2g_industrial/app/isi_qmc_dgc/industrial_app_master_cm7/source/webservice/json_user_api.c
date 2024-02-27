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
#include "urldecode.h"

#include "webservice/json_api_common.h"

/* Limited Definitions of usrmgmt_role_t (these are valid for the api) */
#define USRMGMT_ROLE(_)                               \
    _(kUSRMGMT_RoleMaintenance, "maintenance")        \
    _(kUSRMGMT_RoleOperator, "operator")              \
    _(kUSRMGMT_RoleNone, "none")                      \
    _(kUSRMGMT_RoleEmpty, "empty")                    \
    _(kUSRMGMT_RoleLocalButton, "local button")       \
    _(kUSRMGMT_RoleLocalSd, "local SD")               \
    _(kUSRMGMT_RoleLocalEmergency, "local emergency") \
    /* */

/* generate usrmgmt_role_from_string */
GENERATE_ENUM_FROM_STRING(usrmgmt_role, usrmgmt_role_t, USRMGMT_ROLE, static);

/* generate usrmgmt_role_to_string */
GENERATE_ENUM_TO_STRING(usrmgmt_role, usrmgmt_role_t, USRMGMT_ROLE, static);

PLUG_JSON_CB(json_user_api);
PLUG_JSON_CB(json_user_list_api);

static void write_user_info(const usrmgmt_user_config_t *user_config, config_id_t user_id, FILE *response)
{
    fputs("{\"username\":", response);
    fputs_json_string((const char *)user_config->name, response);
    fprintf(response, ",\"uid\":%d", user_id);
    fputs(", \"locked_until\":", response);
    fputs_json_string(uint64toa(user_config->lockout_timestamp), response);
    fputs(", \"valid_until\":", response);
    fputs_json_string(uint64toa(user_config->validity_timestamp), response);
    fputs(", \"role\":", response);
    fputs_json_string(usrmgmt_role_to_string(user_config->role), response);
    fputs("}", response);
}

http_status_t json_user_list_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    qmc_status_t status;
    usrmgmt_session_t *user_session = request->session;
    config_id_t id;
    usrmgmt_user_config_t config;

    if (!user_session)
    {
        return HTTP_NOREPLY_UNAUTHORIZED;
    }
    fputs("[", response);
    int count = 0;
    int n = 0;
    do
    {
        status = USRMGMT_IterateUsers(&count, &id, &config);
        if (status != kStatus_QMC_Ok)
        {
            /* no further data */
            break;
        }
        if (config.role <= kUSRMGMT_RoleEmpty)
        {
            /* user slot is empty */
            continue;
        }
        if (user_session->role != kUSRMGMT_RoleMaintenance && user_session->uid != id)
        {
            /* no maintenance user and not the logged in user */
            continue;
        }
        if (n++ > 0)
        {
            fputs(",\n", response);
        }
        write_user_info(&config, id, response);
    } while (true);
    fputs("]\n", response);
    return HTTP_REPLY_OK;
}


static config_id_t get_user_information(char *auth_user,
                                        config_id_t user_id,
                                        usrmgmt_user_config_t *user_config,
                                        usrmgmt_session_t *user_session,
                                        char *user)
{
    {
        int count = 0;
        usrmgmt_user_config_t config;
        config_id_t id;
        while (USRMGMT_IterateUsers(&count, &id, &config) == kStatus_QMC_Ok)
        {
            if (auth_user && user_session && user_session->uid == id)
            {
                strncpy(auth_user, (char *)config.name, sizeof(config.name));
            }
            if (user != NULL && strcmp((const char *)config.name, user) == 0)
            {
                user_id = id;
                memcpy(&*user_config, &config, sizeof(*user_config));
            }
        }
    }
    return user_id;
}

/**
 * @brief user api endpoint
 *
 * @param request pointer to request struct
 * @param json_body  pointer to validated JSON data
 * @param json_body_len length of JSON data
 * @param state pointer to plug_json's json_response_t.
 * @param response output FILE
 *
 * @return http status code
 */

http_status_t json_user_api(
    plug_request_t *request, char *json_body, u16_t json_body_len, json_response_t *state, FILE *response)
{
    qmc_status_t status;
    JSONTypes_t type;
    JSONStatus_t jstatus;
    usrmgmt_role_t role = kUSRMGMT_RoleNone;
    char *new_password  = NULL;
    char *rolestr       = NULL;
    char *username      = NULL;
    static char auth_user[USRMGMT_USER_NAME_MAX_LENGTH];
    static char uri_user[USRMGMT_USER_NAME_MAX_LENGTH];
    usrmgmt_session_t *user_session = request->session;
    size_t new_pass_len, role_len;
    char *cur_pass;
    size_t cur_pass_len;
    usrmgmt_session_t *check_session;
    usrmgmt_user_config_t user_config;
    config_id_t user_id;
    bool update_password = false;

    const char *username_loc = NULL;

    if (request->path_match_end && request->path_match_end[0] != '\0')
    {
        username_loc = request->path_match_end;
    }

    if (!user_session)
    {
        return HTTP_NOREPLY_UNAUTHORIZED;
    }

    if (username_loc != NULL)
    {
        if (strlen(username_loc) < sizeof(uri_user))
        {
            url_decode(uri_user, username_loc);
            username = uri_user;
        }
        else
        {
            return HTTP_NOREPLY_BAD_REQUEST;
        }
    }

    user_id = get_user_information(&auth_user[0], kCONFIG_KeyNone, &user_config, user_session, username);

    /* to continue maintenance rights are needed or this is the same account as the logged in one */
    if (user_session->role != kUSRMGMT_RoleMaintenance && user_id != user_session->uid)
    {
        return HTTP_NOREPLY_FORBIDDEN;
    }

    switch (request->method)
    {
        case HTTP_GET:
            if (user_id == kCONFIG_KeyNone)
            {
                return HTTP_NOREPLY_NOT_FOUND;
            }
            write_user_info(&user_config, user_id, response);
            return HTTP_REPLY_OK;

        case HTTP_DELETE:
            if (user_session->role != kUSRMGMT_RoleMaintenance)
            {
                return HTTP_NOREPLY_FORBIDDEN;
            }
            if (user_id == kCONFIG_KeyNone)
            {
                return HTTP_NOREPLY_NOT_FOUND;
            }
            status = USRMGMT_RemoveUser(user_session->sid,(const unsigned char *)username);
            if (webservice_check_error(status, response))
            {
                return HTTP_NOREPLY_NO_CONTENT;
            }
            return HTTP_REPLY_NOT_FOUND;

        case HTTP_POST:
            if (user_id != kCONFIG_KeyNone)
            {
                return HTTP_NOREPLY_CONFLICT;
            }
            role=user_config.role;
            /*fallthru*/
        case HTTP_PUT:
            if (username == NULL || strcmp(username, auth_user) == 0 )
            {
                username        = auth_user;
                user_id         = user_session->uid;
                role            = user_session->role;
                update_password = true;
            }

            /* new user cration is only ok for mantenance users */
            if (!update_password && user_session->role != kUSRMGMT_RoleMaintenance)
            {
                return HTTP_NOREPLY_FORBIDDEN;
            }

            jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("new_passphrase"), &new_password,
                                   &new_pass_len, &type);
            if (jstatus != JSONSuccess)
            {
                return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            }
            if (type != JSONString)
            {
                return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            }
            jstatus =
                JSON_SearchT((char *)json_body, json_body_len, SIZED("passphrase"), &cur_pass, &cur_pass_len, &type);
            if (jstatus != JSONSuccess)
            {
                return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            }
            if (type != JSONString)
            {
                return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
            }
          
            /* POST requires a role */
            if(!update_password)
            {
              jstatus = JSON_SearchT((char *)json_body, json_body_len, SIZED("role"), &rolestr, &role_len, &type);
              if (jstatus != JSONNotFound && ( jstatus != JSONSuccess || type != JSONString || !usrmgmt_role_from_string(&role, rolestr, role_len)))
              {
                return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
              }
            }


            /* add null terminators to the json buffer */
            new_password[new_pass_len] = '\0';
            cur_pass[cur_pass_len]     = '\0';

            /* authenticate the user account specified */
            status = USRMGMT_CreateSession(NULL, NULL, NULL, &check_session, (const unsigned char *)auth_user,
                                           (const unsigned char *)cur_pass);
            if (status == kStatus_QMC_Ok &&
                (check_session == user_session ||
                 (user_session->role == kUSRMGMT_RoleMaintenance && role != kUSRMGMT_RoleNone)))
            {
                if (!update_password)
                {
                    status =
                        USRMGMT_AddUser(user_session->sid,(const unsigned char *)username, (const unsigned char *)new_password, role);
                    if (webservice_check_error(status, response))
                    {
                        user_id = get_user_information(NULL, kCONFIG_KeyNone, &user_config, NULL, username);
                        if (user_id != kCONFIG_KeyNone)
                        {
                            write_user_info(&user_config, user_id, response);
                            return HTTP_REPLY_OK;
                        }
                    }
                    return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
                }
                else
                {
                    status=USRMGMT_UpdateUser(user_session->sid,user_session->uid, (const unsigned char *)new_password, role);
                    if (webservice_check_error(status, response))
                    {
                        user_id = get_user_information(&auth_user[0], user_id, &user_config, user_session, username);
                        write_user_info(&user_config, user_id, response);
                        return HTTP_REPLY_OK;
                    }
                    return HTTP_REPLY_CONFLICT;
                }
            }
            return HTTP_NOREPLY_UNAUTHORIZED;
        default:
            break;
    }
    return HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
}

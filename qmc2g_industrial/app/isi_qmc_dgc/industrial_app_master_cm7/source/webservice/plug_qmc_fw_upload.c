/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "api_qmc_common.h"
#include "api_configuration.h"
#include "api_usermanagement.h"
#include "plug.h"
#include "app.h"
#include "webservice/json_api_common.h"
#include <stdbool.h>
#include <mbedtls/md.h>
#include <stdio.h>

static unsigned char PLUG_TMP_BUFFER_SECTION s_plug_qmc_fw_update_buffer[OCTAL_FLASH_SECTOR_SIZE];

#define SHA256_DIGEST_LENGTH 32

/**
 * @struct fw upload state struct
 */
struct fw_upload_state
{
    http_status_t code;
    qmc_status_t status; /*!< current error status */
    size_t offset;       /*!< offset position <*/
    u16_t sector_write_count;  /*!< flash sector write count*/
    u16_t sector_error_count;  /*!< flash sector error count*/
    u16_t buffer_free;          /*!< */
};

PLUG_EXTENSION(qmc_fw_upload, struct fw_upload_state);

/**
 * @brief  initialize the state when the rule is matched
 */
http_status_t qmc_fw_upload_on_match(plug_state_p state, const plug_rule_t *rule, plug_request_t *request)
{
    *state = (struct fw_upload_state){.status = kStatus_QMC_Ok,
                                      .code   = HTTP_REPLY_NO_CONTENT,
                                      .sector_write_count  = 0,
                                      .sector_error_count  = 0,
                                      .offset = 0,
                                      .buffer_free   = sizeof(s_plug_qmc_fw_update_buffer)};
    return state->code;
}

/**
 * @brief the upload callback processes tcp packet payloads from the body of the upload
 *
 * this in potentially called multiple times for one upload until all packets have been processed
 *
 * the user must already been authenticated by the on_body_data handler in plug_qmc_authorize
 */
http_status_t qmc_fw_upload_on_body_data(plug_state_p state,
                                         const plug_rule_t *rule,
                                         plug_request_t *request,
                                         plug_response_t *response,
                                         FILE *body,
                                         u16_t len)
{
    static mbedtls_md_context_t ctx;
    const mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    usrmgmt_session_t *user_session = request->session;

    /* check privilege to upload, session is only present if authentication succeeded */
    /* we need to check the accept_json flag as well, the upload should not be done */
    /* if the response is not acceptable */

    if (user_session != NULL && user_session->role == kUSRMGMT_RoleMaintenance && request->accept_json &&
        state->code > HTTP_REPLY_OK)
    {
        /* if this is the first call to this function, initialize hashing */
        if (state->offset == 0)
        {
            // content length has to be divisible by 2. (16bit width)
            if (request->content_length & 0x1)
            {
                state->code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
                return state->code;
            }

            // initialise the checksum functions
            mbedtls_md_init(&ctx);
            if (0 != mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0))
            {
                state->code = HTTP_NOREPLY_INTERNAL_SERVER_ERROR;
                return state->code;
            }
            if (0 != mbedtls_md_starts(&ctx))
            {
                state->code = HTTP_NOREPLY_INTERNAL_SERVER_ERROR;
                return state->code;
            }
        }
        int rc = 0;

        /* when no error occurred (even at the last call) continue reading from the current packet */
        while (state->status == kStatus_QMC_Ok)
        {
        	int retry_count=WEBSERVICE_FIRMWARE_UPLOAD_WRITE_RETRIES;
            unsigned char *p = s_plug_qmc_fw_update_buffer + (sizeof(s_plug_qmc_fw_update_buffer) - state->buffer_free);
            rc               = fread(p, 1, state->buffer_free, body);
            if (rc <= 0)
            {
                break;
            }
            state->buffer_free -= rc;
            size_t len = sizeof(s_plug_qmc_fw_update_buffer) - state->buffer_free;
            //write if the buffer is full or it's contents complete the request
            if ((state->buffer_free == 0) || (state->offset + len >= request->content_length))
            {
                if (mbedtls_md_update(&ctx, s_plug_qmc_fw_update_buffer, len))
                {
                    state->status = kStatus_QMC_ErrInterrupted;
                    break;
                }
                // add padding to the buffer if it's not full
                // always write a full OCTAL_FLASH_SECTOR_SIZE buffer, even if not strictly needed
                if(len < sizeof(s_plug_qmc_fw_update_buffer))
                {
                  memset(s_plug_qmc_fw_update_buffer+len,-1,sizeof(s_plug_qmc_fw_update_buffer)-len);
                }

                // retry writing on error
                while(retry_count-- >0)
                {
                	state->status = CONFIG_WriteFwUpdateChunk(state->offset, (uint8_t *)s_plug_qmc_fw_update_buffer, sizeof(s_plug_qmc_fw_update_buffer));
                	state->sector_write_count++;
                	if(state->status == kStatus_QMC_Ok) {
                		break;
                	}
                	// count up on retry. counts the retries in case of eventual success
					state->sector_error_count++;
                }

                state->buffer_free = sizeof(s_plug_qmc_fw_update_buffer);
                state->offset += len; /* add buffer contents to offset */
                switch (state->status)
                {
                    case kStatus_QMC_ErrRange:
                        state->code = HTTP_NOREPLY_PAYLOAD_TOO_LARGE;
                        break;
                    case kStatus_QMC_Ok:
                        break;
                    case kStatus_QMC_ErrArgInvalid:
                        /*fallthrough*/
                    default:
                        state->code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
                        break;
                }
            }
        }

        /* check if all bytes have been written */
        if (state->offset >= request->content_length)
        {
            STATIC_ASSERT(sizeof(response->buffer) >= 32, "response->buffer to small");

            /* finalize the hash value */
            if (mbedtls_md_finish(&ctx, (unsigned char *)response->buffer))
            {
                state->status = kStatus_QMC_ErrInterrupted;
            }
            mbedtls_md_free(&ctx);
            if (state->status == kStatus_QMC_Ok)
            {
                int i = 0, h = 0, l = 0;
                *s_plug_qmc_fw_update_buffer = '\0';
                FILE *fd = fmemopen(s_plug_qmc_fw_update_buffer, sizeof(s_plug_qmc_fw_update_buffer), "w");
                if (fd)
                {
                    if (webservice_check_error(state->status, fd))
                    {
                        (void)fprintf(fd, "{\"bytes\":%u,\"sha256\":\"", state->offset);
                        for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
                        {
                            h = ((int)response->buffer[i] & 0xf0) >> 4;
                            l = ((int)response->buffer[i] & 0x0f);
                            (void)fputc(((h < 10) ? '0' : 'A' - 10) + h, fd);
                            (void)fputc(((l < 10) ? '0' : 'A' - 10) + l, fd);
                        }
                        (void)fprintf(fd, "\", \"sector_writes\":%u, \"sector_retry_count\":%u}\n",
                        		state->sector_write_count,state->sector_error_count);
                        fclose(fd);
                        state->code = HTTP_REPLY_OK;
                    }
                }
            }
        }
    }
    else
    {
        /* consume/drop body if there was an error */
        /* the callback would be called again otherwise */
        fseek(body, 0, SEEK_END);
    }
    return state->code;
}

/**
 * @brief generate upload reply, setting content-type
 */
http_status_t qmc_fw_upload_on_reply(plug_state_p state,
                                     const plug_rule_t *rule,
                                     plug_request_t *request,
                                     plug_response_t *response,
                                     http_status_t status_code)
{
    usrmgmt_session_t *user_session = request->session;
    if (user_session && user_session->role == kUSRMGMT_RoleMaintenance)
    {
        /* if the request is valid */
        if (request->accept_json)
        {
            response->content_type   = "application/json";
            response->content_length = state->code == HTTP_REPLY_OK ? strlen((char *)s_plug_qmc_fw_update_buffer) : 0;
            status_code              = state->code;
        }
        else
        {
            /* no json response not accepted, upload was not written */
            status_code = HTTP_NOREPLY_NOT_ACCEPTABLE;
        }
        if (status_code == HTTP_REPLY_NO_CONTENT)
        {
            status_code = HTTP_NOREPLY_UNPROCESSABLE_ENTITY;
        }
    }
    else
    {
        status_code = HTTP_NOREPLY_UNAUTHORIZED;
    }
    return status_code;
}

/**
 * @brief response body generation, create a json message
 *
 * called if HTTP_REPLY_OK was returned from the on_request callback
 */
void qmc_fw_upload_write_body_data(plug_state_p state,
                                   const plug_rule_t *rule,
                                   plug_request_t *request,
                                   const plug_response_t *response,
                                   http_status_t status_code,
                                   FILE *body,
                                   u16_t sndbuf)
{
    (void)fputs((char *)s_plug_qmc_fw_update_buffer, body);
}

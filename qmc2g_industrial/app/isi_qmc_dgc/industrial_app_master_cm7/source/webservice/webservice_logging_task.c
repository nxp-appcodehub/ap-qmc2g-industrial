/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "webservice_logging_task.h"
#include "constants.h"
#include "api_configuration.h"
#include "qmc_features_config.h"
#include "api_logging.h"
#include <cmsis_compiler.h>
#include <stdint.h>
#include <sys/_types.h>

#define ELEMENTS(A)  (sizeof(A) / sizeof((A)[0]))

TaskHandle_t g_webservice_logging_task_handle;

typedef union
{
    struct
    {
        uint16_t count;
        uint16_t uid;
    } value;
    uint32_t atomic;
} webservice_counter;


/* number of error counters for the status code ranges */
#define LOG_ENTRIES_4xx  (HTTP_REPLY_REQUEST_HEADER_FIELDS_TOO_LARGE - HTTP_REPLY_BAD_REQUEST)
#define LOG_ENTRIES_5xx  (HTTP_REPLY_NETWORK_AUTHENTICATION_REQUIRED - HTTP_REPLY_INTERNAL_SERVER_ERROR)

/* status code range error counters */
static webservice_counter g_webservice_500_errors[LOG_ENTRIES_5xx];
static webservice_counter g_webservice_400_errors[LOG_ENTRIES_4xx];

/**
 * @brief atomic increment a value
 *
 * @param val pointer to uint16_t to increment
 */
static void atomic_inc(uint16_t *val)
{
    uint16_t tmp;
    __DMB();
    do
    {
        tmp = __LDREXH(val);
        tmp++;
    } while (__STREXH(tmp, val));
    __DMB();
}

/**
 * @brief atomic counter reset
 *
 * @param val pointer to value to retrieve and reset
 *
 * return current value and sets it to 0;
 *
 * @return pre reset value
 */
static webservice_counter atomic_reset(webservice_counter *val)
{
    webservice_counter tmp;
    __DMB();
    do
    {
        tmp.atomic = __LDREXW(&val->atomic);
        if (tmp.atomic == 0)
        {
            break;
        };
    } while (__STREXW(0, &val->atomic));
    __DMB();
    return tmp;
}

/**
 * @brief atomic store if not set
 *
 * @param val value to optionally store
 * @param addr pointer to addess to store to if not zero
 *
 * return current value, or 0 if stored
 */
static uint16_t atomic_store_if_zero(uint16_t val, uint16_t *addr)
{
    uint16_t tmp;
    __DMB();
    do
    {
        tmp = __LDREXH(addr);
        if (tmp != 0)
        {
            break;
        };
    } while (__STREXH(val, &tmp));
    __DMB();
    return tmp;
}

/**
 * @brief increment 4xx-5xx status code error counters
 *
 * @param status_code HTTP status code
 *
 * will ignore other status codes
 */
void Webservice_IncrementErrorCounter(int status_code, config_id_t user)
{
    status_code=abs(status_code);
    /* default to 500 error */ 
    webservice_counter  *counter=&g_webservice_500_errors[0];
    
    /* if this is no error just return */
    if(status_code< 400) { 
        return;
    }

    /* 4xx error codes */
    if(status_code < 500 && status_code < 400+ELEMENTS(g_webservice_400_errors))
    {
        counter=&g_webservice_400_errors[status_code - 400];
    }

    /* 5xx error codes*/
    else if(status_code < 600 && status_code >= 500 && status_code < 500+ELEMENTS(g_webservice_500_errors))
    {
        counter=&g_webservice_500_errors[status_code - 500];
    }

    /* increment of the error counter */
    atomic_inc(&counter->value.count);
    
    /* associate a user if none is set yet*/
    if (user != kCONFIG_KeyNone)
    {
        atomic_store_if_zero(user, &counter->value.uid);
    }
}

/*!
 * @brief The Webservice Service Task logs accumulated webserver errors.
 *
 * @param pvParameters Unused.
 */

void WebserviceLoggingTask(void *pvParameters)
{
    log_record_t entry = {
        .type                   = kLOG_ErrorCount,
        .data.errorCount.source = LOG_SRC_Webservice,
    };
    for (;;)
    {
        // start with delaying the task
        vTaskDelay(pdMS_TO_TICKS(1000 * WEBSERVICE_HTTPD_ERROR_LOG_INTERVAL));

        unsigned int i;

        for (i = 0; i < ELEMENTS(g_webservice_500_errors);  i++)
        {
            webservice_counter count = atomic_reset(&g_webservice_500_errors[i]);
            if (count.value.count)
            {
                entry.data.errorCount.category = LOG_CAT_Fault, 
                entry.data.errorCount.errorCode = 500 + i;
                entry.data.errorCount.count = count.value.count;
                entry.data.errorCount.user  = count.value.uid;
                LOG_QueueLogEntry(&entry, false);
            }
        }
        for (i = 0; i < ELEMENTS(g_webservice_400_errors); i++)
        {
            webservice_counter count = atomic_reset(&g_webservice_400_errors[i]);
            if (count.value.count)
            {
                entry.data.errorCount.category =
                    (400+i == 401 || 400+i == 403) ? LOG_CAT_Authentication : LOG_CAT_General,
                entry.data.errorCount.errorCode = 400 + i;
                entry.data.errorCount.count = count.value.count;
                entry.data.errorCount.user  = count.value.uid;
                LOG_QueueLogEntry(&entry, false);
            }
        }
    }
}

/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*
 * A Considerable IoT HTTPd
 *
 * configuration options
 *
 * */

#ifndef CIT_HTTPD_OPT_H
#define CIT_HTTPD_OPT_H

/* RULE PATTERN SUPPORT */
#define ENABLE_RULE_MATCH_URI         0
#define ENABLE_RULE_MATCH_PORT        0
#define ENABLE_RULE_MATCH_INTERFACE   0
#define ENABLE_RULE_MATCH_QUERYSTRING 0
#define ENABLE_RULE_MATCH_FRAGMENT    0

/*
 * use coreJSON to validate json requests
 */

/* include file  */
#define PLUG_JSON_INCLUDE_FILE <core_json.h>

/* validation expression */
#define PLUG_JSON_VALIDATE(buffer, len) (JSONSuccess == JSON_Validate(buffer, len))

/* server defines */

#ifndef CIT_HTTPD_SERVER_HEADER
#define CIT_HTTPD_SERVER_HEADER "Server: cit_httpd/1.0"
#endif

#ifndef CIT_HTTPD_MAX_ACTIVE_RULES
#define CIT_HTTPD_MAX_ACTIVE_RULES 6
#endif

#ifndef CIT_HTTPD_MAX_SESSIONS
#define CIT_HTTPD_MAX_SESSIONS 8
#endif

/* TCP_TMR_INTERVAL; */
/*  the poll interval, in 0.5sec steps */
/*  10 is every 5 seconds */
#ifndef CIT_HTTPD_POLL_INTERVAL
#define CIT_HTTPD_POLL_INTERVAL 5
#endif

/* timeout counter for states in accepting state */
#ifndef CIT_HTTPD_KEEP_ALIVE_TIMEOUT
#define CIT_HTTPD_KEEP_ALIVE_TIMEOUT 60
#endif

#ifndef CIT_HTTPD_SESSION_TIMEOUT
#define CIT_HTTPD_SESSION_TIMEOUT 60
#endif

#ifndef CIT_HTTPD_MAX_REQ_SIZE
#define CIT_HTTPD_MAX_REQ_SIZE (1024 * 8)
#endif

/* maximum internal path length */
#ifndef CIT_HTTPD_MAX_URI_LEN
#define CIT_HTTPD_MAX_URI_LEN 128
#endif
/* maximum internal path length */
#ifndef CIT_HTTPD_MAX_PATH_LEN
#define CIT_HTTPD_MAX_PATH_LEN 64
#endif

/* size of the response scratch buffer */
#ifndef CIT_HTTPD_SCRATCH_BUFFER_SIZE
#define CIT_HTTPD_SCRATCH_BUFFER_SIZE 128
#endif

#ifndef CIT_HTTPD_MAX_FRAGMENT_LEN
#define CIT_HTTPD_MAX_FRAGMENT_LEN 24
#endif

#ifndef CIT_HTTPD_MAX_QUERY_STRING_LEN
#define CIT_HTTPD_MAX_QUERY_STRING_LEN 24
#endif

#ifndef CIT_HTTPD_MAX_FIELD_LENGTH
#define CIT_HTTPD_MAX_QUERY_STRING_LEN 24
#endif

#ifndef CIT_IOBUF_POOL_SIZE
#define CIT_IOBUF_POOL_SIZE 32
#endif

#ifndef CIT_HTTPD_ARENA_BUFFER_SIZE
#define CIT_HTTPD_ARENA_BUFFER_SIZE (TCP_MSS / 2)
#endif

#ifndef CIT_HTTPD_TCP_PRIO
#define CIT_HTTPD_TCP_PRIO TCP_PRIO_MIN
#endif

#ifndef PLUG_ZIP_FS_FATFS
#define PLUG_ZIP_FS_FATFS 0
#endif

#ifndef PLUG_ZIP_FILE_COUNT
#if PLUG_ZIP_FS_FATFS
#define PLUG_ZIP_FILE_COUNT 2
#else
#define PLUG_ZIP_FILE_COUNT 1
#endif
#endif

#ifndef PLUG_STATE_SIZE
#define PLUG_STATE_SIZE 16
#endif

#ifndef PLUG_TMP_BUFFER_SECTION
#define PLUG_TMP_BUFFER_SECTION __attribute__((section(".bss.$SRAM_OC1")))
#endif

#ifndef PLUG_JSON_VALIDATE
#define PLUG_JSON_VALIDATE(buffer, len)
#warning PLUGJSON_VALIDATE plug_json will not validate input!
#endif

#ifndef PLUG_ZIP_JSON_VALIDATE
#undef PLUG_ZIP_JSON_INCLUDE
#endif

#endif /* CITSRV_H */

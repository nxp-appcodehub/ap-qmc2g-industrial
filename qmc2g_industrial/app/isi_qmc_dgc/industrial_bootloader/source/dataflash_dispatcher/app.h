/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
 
#ifndef _APP_H_
#define _APP_H_

#include "qmc2_boot_cfg.h"
#include "fsl_flexspi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/


//LogRecorder / log_records are stored here
#define RECORDER_REC_DATALOGGER_AREABEGIN  (FLASH_RECORDER_ORIGIN)
#define RECORDER_REC_DATALOGGER_AREALENGTH (OCTAL_FLASH_SECTOR_SIZE * FLASH_RECORDER_SECTORS)

//InfRecorder / log_infos are stored here
#define RECORDER_REC_INF_DATALOGGER_AREABEGIN  (RECORDER_REC_DATALOGGER_AREABEGIN + RECORDER_REC_DATALOGGER_AREALENGTH)
#define RECORDER_REC_INF_DATALOGGER_AREALENGTH (OCTAL_FLASH_SECTOR_SIZE * 2)

//Configuration
#define RECORDER_REC_CONFIG_AREABEGIN  (RECORDER_REC_INF_DATALOGGER_AREABEGIN + RECORDER_REC_INF_DATALOGGER_AREALENGTH)
#define RECORDER_REC_CONFIG_AREALENGTH (OCTAL_FLASH_SECTOR_SIZE * FLASH_CONFIG_SECTORS)

//End of data
#define RECORDER_REC_END_DATA (RECORDER_REC_CONFIG_AREABEGIN + RECORDER_REC_CONFIG_AREALENGTH)

#define MAKE_EVEN( size) ( ( (size)&1) == 1 ? (size+1) : (size) )
#define MAKE_NUMBER_ALIGN( value, size) ( value % size ? value + size - value % size : value )

#ifdef FEATURE_DATALOGGER_RECORDER_DBG_PRINT
#define dbgRecPRINTF(...) PRINTF(__VA_ARGS__)
#else
#define dbgRecPRINTF(...)
#endif

#ifdef FEATURE_DATALOGGER_DISPATCHER_DBG_PRINT
#define dbgDispPRINTF(...) PRINTF(__VA_ARGS__)
#else
#define dbgDispPRINTF(...)
#endif

#ifdef FEATURE_DATALOGGER_SDCARD_DBG_PRINT
#define dbgSDcPRINTF(...) PRINTF(__VA_ARGS__)
#else
#define dbgSDcPRINTF(...)
#endif

#ifdef FEATURE_CONFIG_DBG_PRINT
#define dbgCnfPRINTF(...) PRINTF(__VA_ARGS__)
#else
#define dbgCnfPRINTF(...)
#endif

#ifdef FEATURE_LCRYPT_DBG_PRINT
#define dbgLcryptPRINTF(...) PRINTF(__VA_ARGS__)
#else
#define dbgLcryptPRINTF(...)
#endif

#endif /* _APP_H_ */

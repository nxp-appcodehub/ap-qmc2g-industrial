/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */
 
#ifndef _APP_H_
#define _APP_H_

#include "qmc_features_config.h"
#include "fsl_flexspi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define OCTAL_FLASH_FLEXSPI FLEXSPI1
#define OCTAL_RAM_FLEXSPI FLEXSPI1
#define OCTAL_FLASH_SIZE_A1 0x10000
#define OCTAL_RAM_SIZE_A2 0x8000
#define OCTAL_FLASH_SIZE (OCTAL_FLASH_SIZE_A1*1024U)
#define OCTAL_FLASH_PAGE_SIZE 256U
#define	OCTAL_FLASH_SECTOR_SIZE 4096U

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

//LUT sequences
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA 		0
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READID 			1
#define OCTALRAM_CMD_LUT_SEQ_IDX_READDATA 			2
#define OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA 			3
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS		4
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2			6
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2		8
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2_SPI	13
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE		7
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE_SPI	10
#define OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA 		11
#define OCTALFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR 		12
#define OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG			9

#define CUSTOM_LUT_LENGTH 64

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

#endif /* _APP_H_ */

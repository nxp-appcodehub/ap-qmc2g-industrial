/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "fsl_flexspi.h"
#include "app.h"
#include "fsl_debug_console.h"
#include "fsl_cache.h"

#include "pin_mux.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "dispatcher.h"
#include "api_logging.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

//void flexspi_octal_flash_init(void);
void flexspi1_octal_bus_init();

status_t flexspi_nor_octalflash_read_status(FLEXSPI_Type *base, uint32_t *buffer);
status_t flexspi_nor_octalflash_read_data(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer, size_t size);
status_t flexspi_nor_octalflash_write_data(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer, size_t size);
status_t flexspi_nor_octalflash_erase_sector(FLEXSPI_Type *base, uint32_t addr);
status_t flexspi_nor_octalflash_read_ID(FLEXSPI_Type *base, uint32_t *buffer);
status_t flexspi_nor_octalflash_read_config2(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer);
status_t flexspi_nor_octalflash_write_config2(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer);
status_t flexspi_nor_octalflash_write_config2_spi(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer);
status_t flexspi_nor_octalflash_read_config(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
flexspi_device_config_t g_deviceA1config = {
    .flexspiRootClk       = 0,
    .isSck2Enabled        = false,
    .flashSize            = OCTAL_FLASH_SIZE_A1,
    .CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval           = 2,
    .CSHoldTime           = 3,
    .CSSetupTime          = 3,
    .dataValidTime        = 3,
    .columnspace          = 0,
    .enableWordAddress    = false,
    .enableWriteMask      = false,
    .AWRSeqIndex          = OCTALFLASH_CMD_LUT_SEQ_IDX_READID,
    .AWRSeqNumber         = 1,
    .ARDSeqIndex          = OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA,
    .ARDSeqNumber         = 1,
    .AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 20,
};

flexspi_device_config_t g_deviceA2config = {
    .flexspiRootClk       = 0,
    .isSck2Enabled        = false,
    .flashSize            = OCTAL_RAM_SIZE_A2,
    .CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle,
    .CSInterval           = 2,
    .CSHoldTime           = 2,
    .CSSetupTime          = 3,
    .dataValidTime        = 3,
    .columnspace          = 4,
    .enableWordAddress    = false,
	.enableWriteMask      = true,
    .AWRSeqIndex          = OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA,
    .AWRSeqNumber         = 1,
    .ARDSeqIndex          = OCTALRAM_CMD_LUT_SEQ_IDX_READDATA,
    .ARDSeqNumber         = 1,
    .AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
    .AHBWriteWaitInterval = 20,
};

const uint32_t g_customLUT[CUSTOM_LUT_LENGTH] = {
/* Read Data RAM */
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_READDATA + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x0),
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_READDATA + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 22, kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 8),
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_READDATA + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 8, kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04),
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_READDATA + 3] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, 0, 0, kFLEXSPI_Command_STOP, 0, 0),

/* Write Data RAM */
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x0),
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 22, kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 8),
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 8, kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x04),
[4 * OCTALRAM_CMD_LUT_SEQ_IDX_WRITEDATA + 3] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, 0, 0, kFLEXSPI_Command_STOP, 0, 0),

/* Read data flash DTR-OPI */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xEE, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x11),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_8PAD, 16),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x4, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

/* Erase Sector flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x21, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xDE),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),

/* Write Data flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x12, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xED),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x4),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, 0, 0, kFLEXSPI_Command_STOP, 0, 0),

/* Read ID flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READID + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x9F, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x60),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READID + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_8PAD, 0x04),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READID + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

/* Read status data flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xFA),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_8PAD, 0x4),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x2, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

/* Write enbale Octal DDR flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x06, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xF9),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),

/* Write enbale Octal DDR flash SPI mode */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE_SPI + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x06, kFLEXSPI_Command_STOP, 0, 0),

/* Write Config2 Octal DDR flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2 + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x72, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x8D),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2 + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x01),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2 + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

/* Write Config2 SPI flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2_SPI + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x72, kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x20),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2_SPI + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

/* Read Config2 Octal DDR flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2 + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x71, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x8E),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2 + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_8PAD, 0x04),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2 + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x01, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

/* Read Config Octal DDR flash */
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG + 0] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x15, kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xEA),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG + 1] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x20, kFLEXSPI_Command_DUMMY_DDR, kFLEXSPI_8PAD, 0x04),
[4 * OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG + 2] = FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x01, kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

};

void FlashPrintStatus( recorder_status_t *rstat)
{
	dbgRecPRINTF("Idr:%x\n\r",rstat->Idr);
	dbgRecPRINTF("Pt:%x\n\r",rstat->Pt);
	dbgRecPRINTF("AreaBegin:%x\n\r",rstat->AreaBegin);
	dbgRecPRINTF("AreaLength:%x\n\r",rstat->AreaLength);
	dbgRecPRINTF("PageSize:%x\n\r",rstat->PageSize);
	dbgRecPRINTF("RecordSize:%x\n\r",rstat->RecordSize);

	dbgRecPRINTF("cnt:%x\n\r",rstat->cnt);
	dbgRecPRINTF("timestamp_s:%x\n\r",rstat->ts.seconds);
	dbgRecPRINTF("timestamp_ms:%x\n\r",rstat->ts.milliseconds);
}

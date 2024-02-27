/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/**
 * @file    sbl_flash.c
 * @brief   Flash API.
 */
#include "fsl_common.h"
#include <qmc2_boot_cfg.h>
#include <qmc2_flash.h>
#include "fsl_romapi.h"
#include "fsl_cache.h"
#include "board.h"

/*! @brief FLEXSPI NOR flash driver Structure */
static flexspi_nor_config_t flexspi1NorCfg;
static flexspi_nor_config_t flexspi2NorCfg;

/*! @brief For Normal Octal, eg. MX25UM51245G  */
static serial_nor_config_option_t option = {
    .option0.U = 0xc0403037,
    .option1.U = 0U,
};

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

static void flexspi1_octal_bus_init_with_rxSC( flexspi_read_sample_clock_t rxSampleClock);
static void allowCm7ReadWriteToFlexSPIs(void);
static void allowCm7ReadToFlexSPIs(void);
/**************************************************************************************
 * 									Private functions								  *
 **************************************************************************************/
/* Allow CM7 read/write access to FlexSPI peripherals. */
static void allowCm7ReadWriteToFlexSPIs(void)
{
	RDC->MR[8].MRC = 0x400000F3U;
	RDC->MR[16].MRC = 0x400000F3U;
	RDC->PDAP[3] = 0x0000000BU;
	RDC->PDAP[4] = 0x0000000BU;

   	/* Data Synchronization Barrier, Instruction Synchronization Barrier, and Data Memory Barrier. */
    __DSB();
    __ISB();
    __DMB();
}
/* Allow CM7 read access to FlexSPI peripherals. */
static void allowCm7ReadToFlexSPIs(void)
{
	RDC->MR[8].MRC = 0x400000F2U;
	RDC->MR[16].MRC = 0x400000F2U;
	RDC->PDAP[3] = 0x0000000AU;
	RDC->PDAP[4] = 0x0000000AU;

   	/* Data Synchronization Barrier, Instruction Synchronization Barrier, and Data Memory Barrier. */
    __DSB();
    __ISB();
    __DMB();
}

void flexspi1_octal_bus_init_with_rxSC( flexspi_read_sample_clock_t rxSampleClock)
{
    flexspi_config_t config;
    flexspi_config_t config2;

    /* Reset peripheral before configuring it. */
	FLEXSPI1->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;
	FLEXSPI_SoftwareReset(FLEXSPI1);

	clock_root_config_t rootCfg = {0};
    rootCfg.mux = kCLOCK_FLEXSPI1_ClockRoot_MuxSysPll2Out; // 528MHz
	if( rxSampleClock == kFLEXSPI_ReadSampleClkExternalInputFromDqsPad )
	{
	    rootCfg.div = 2;	//cca 133MHz DDR
	}
	else
	{
	    rootCfg.div = 10;	//cca 50MHz SDR
	}
    CLOCK_SetRootClock(kCLOCK_Root_Flexspi1, &rootCfg);

    g_deviceA1config.flexspiRootClk = CLOCK_GetRootClockFreq(kCLOCK_Root_Flexspi1);
    g_deviceA2config.flexspiRootClk = CLOCK_GetRootClockFreq(kCLOCK_Root_Flexspi1);

    /*Get FLEXSPI default settings and configure the flexspi. */
    FLEXSPI_GetDefaultConfig(&config);

    /*Set AHB buffer size for reading data through AHB bus. */
    config.ahbConfig.enableAHBPrefetch = true;
    /*Allow AHB read start address do not follow the alignment requirement. */
    config.ahbConfig.enableReadAddressOpt = true;
    config.ahbConfig.enableAHBBufferable  = true;
    config.ahbConfig.enableAHBCachable    = true;
    /* enable diff clock and DQS */
    config.enableSckBDiffOpt = false;
    config.rxSampleClock     = rxSampleClock;
    config.enableCombination = true;
    config.enableDoze = false;

    FLEXSPI_Init(OCTAL_FLASH_FLEXSPI, &config);


    /*Get FLEXSPI default settings and configure the flexspi. */
    FLEXSPI_GetDefaultConfig(&config2);

    /*Set AHB buffer size for reading data through AHB bus. */
    config2.ahbConfig.enableAHBPrefetch = false;
    /*Allow AHB read start address do not follow the alignment requirement. */
    config2.ahbConfig.enableReadAddressOpt = true;
    config2.ahbConfig.enableAHBBufferable  = false;
    config2.ahbConfig.enableAHBCachable    = false;
    /* enable diff clock and DQS */
    config2.enableSckBDiffOpt = false;
    config2.rxSampleClock     = rxSampleClock;
    config2.enableCombination = true;
    config2.enableDoze = false;

    FLEXSPI_Init(OCTAL_RAM_FLEXSPI, &config2);

    /* Configure flash settings according to serial flash feature. */
    FLEXSPI_SetFlashConfig(OCTAL_FLASH_FLEXSPI, &g_deviceA1config, kFLEXSPI_PortA1);
    FLEXSPI_SetFlashConfig(OCTAL_RAM_FLEXSPI, &g_deviceA2config, kFLEXSPI_PortA2);

    /* Update LUT table. */
    FLEXSPI_UpdateLUT(OCTAL_FLASH_FLEXSPI, 0, g_customLUT, CUSTOM_LUT_LENGTH);

    /* Do software reset. */
    FLEXSPI_SoftwareReset(OCTAL_FLASH_FLEXSPI);
}

/**************************************************************************************
 * 									Public functions								  *
 **************************************************************************************/
sss_status_t QMC2_FLASH_OctalRamInit(void)
{
	/* Allow CM7 read/write access to FlexSPI peripherals. */
	allowCm7ReadWriteToFlexSPIs();
	flexspi1_octal_bus_init_with_rxSC( kFLEXSPI_ReadSampleClkExternalInputFromDqsPad);
	/* Allow CM7 read access to FlexSPI peripherals. */
	allowCm7ReadToFlexSPIs();
	return kStatus_SSS_Success;
}

sss_status_t QMC2_FLASH_DriverInit(void)
{
	status_t status = kStatus_Fail;

	/* Allow CM7 read/write access to FlexSPI peripherals. */
	allowCm7ReadWriteToFlexSPIs();

    ROM_API_Init();

  	memcpy((void*) &flexspi2NorCfg, (void*) FLEXSPI2_FCB_ADDRESS,	sizeof(flexspi_nor_config_t));

    /* Setup FLEXSPI NOR Configuration Block */
    status = ROM_FLEXSPI_NorFlash_GetConfig(FLEXSPI1_INSTANCE, &flexspi1NorCfg, &option);
    if (status != kStatus_Success)
    {
    	PRINTF("\r\nERROR: Get FLEXSPI NOR configuration block failure!\r\n");
    	goto cleanup;
    }

    status = kStatus_Fail;
	/* Init FlexSPI1 because the FlexSPI2 was initialized by ROM */
	status = ROM_FLEXSPI_NorFlash_Init(FLEXSPI1_INSTANCE, &flexspi1NorCfg);
    if (status != kStatus_Success)
    {
    	PRINTF("\r\nERROR: ROM Flash Init failed!\r\n");
    	goto cleanup;
    }

    flexspi2NorCfg.memConfig.lookupTable[20] = 0x8200421;
	flexspi2NorCfg.memConfig.lookupTable[32] = 0x82004dc;
	flexspi2NorCfg.memConfig.lookupTable[36] = 0x8200434;
	flexspi2NorCfg.memConfig.lookupTable[37] = 0x2204;
	flexspi2NorCfg.memConfig.lookupTable[44] = 0x460;

cleanup:

	allowCm7ReadToFlexSPIs();

	if (status == kStatus_Success)
	{
		return kStatus_SSS_Success;
	}

	return status;
}


/* Clean global varialbes. */
void QMC2_FLASH_CleanFlexSpiData(void)
{
	memset(&flexspi1NorCfg, 0, sizeof(flexspi_nor_config_t));
	memset(&flexspi2NorCfg, 0, sizeof(flexspi_nor_config_t));
	memset(&option, 0, sizeof(serial_nor_config_option_t));
}

sss_status_t QMC2_FLASH_Erase(uint32_t dst, uint32_t length)
{
	status_t status = kStatus_Fail;
	sss_status_t sssStatus = kStatus_SSS_Fail;
	uint32_t flexspiInstance = 0;
	flexspi_nor_config_t *norConfig = NULL;
	uint32_t primask = 0;
	uint32_t len = length;

	if(!((dst & EXAMPLE_FLEXSPI1_AMBA_BASE) || (dst & EXAMPLE_FLEXSPI2_AMBA_BASE)))
	{
		return kStatus_SSS_InvalidArgument;
	}

	if(length == 0)
	{
		return kStatus_SSS_InvalidArgument;
	}

	if((dst & EXAMPLE_FLEXSPI1_AMBA_BASE) == EXAMPLE_FLEXSPI1_AMBA_BASE)
	{
		dst -= EXAMPLE_FLEXSPI1_AMBA_BASE;
		flexspiInstance = FLEXSPI1_INSTANCE;
		norConfig = &flexspi1NorCfg;
	}
	else
	{
		dst -= EXAMPLE_FLEXSPI2_AMBA_BASE;
		flexspiInstance = FLEXSPI2_INSTANCE;
		norConfig = &flexspi2NorCfg;
	}

	/* Disable interrupts */
	primask = DisableGlobalIRQ();

	/* Allow CM7 read/write access to FlexSPI peripherals. */
	allowCm7ReadWriteToFlexSPIs();

	USER_LED4_ON();
	while((len >= QSPI_FLASH_ERASE_SECTOR_SIZE))
	{
		status = ROM_FLEXSPI_NorFlash_Erase(flexspiInstance, norConfig, dst, QSPI_FLASH_ERASE_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\nERROR: Erase failed!\r\n");
			goto cleanup;
		}

		dst += QSPI_FLASH_ERASE_SECTOR_SIZE;
		len -= QSPI_FLASH_ERASE_SECTOR_SIZE;

		USER_LED4_TOGGLE();
	}

	if(len)
	{
		status = ROM_FLEXSPI_NorFlash_Erase(flexspiInstance, norConfig, dst, QSPI_FLASH_ERASE_SECTOR_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\nERROR: Erase failed!\r\n");
			goto cleanup;
		}

		USER_LED4_TOGGLE();
	}


	sssStatus = kStatus_SSS_Success;

cleanup:

	USER_LED4_OFF();
	/* Invalidate Data cache. */
	DCACHE_InvalidateByRange(dst, length);
	/* Allow CM7 write access to FlexSPI peripherals. */
	allowCm7ReadToFlexSPIs();
    /* Enable interrupts */
    EnableGlobalIRQ(primask);

	return sssStatus;
}

sss_status_t QMC2_FLASH_ProgramPages(uint32_t dst, const void *src, uint32_t length)
{
	status_t status = kStatus_Fail;
	sss_status_t sssStatus = kStatus_SSS_Fail;

	uint32_t flexspiInstance = 0;
	flexspi_nor_config_t *norConfig = NULL;
	uint32_t primask = 0;
	uint8_t pgmBuffer[PGM_PAGE_SIZE];
	uint32_t srcAddr = (uint32_t)src;
	uint32_t dstAddr = 0;
	uint32_t len = length;

	bool isSrcAddrEncryptedXip = false;
	if(src == NULL)
	{
		return kStatus_SSS_InvalidArgument;
	}

	if(!((dst & EXAMPLE_FLEXSPI1_AMBA_BASE) || (dst & EXAMPLE_FLEXSPI2_AMBA_BASE)))
	{
		return kStatus_SSS_InvalidArgument;
	}

	if(length == 0)
	{
		return kStatus_SSS_InvalidArgument;
	}

	if((dst & EXAMPLE_FLEXSPI1_AMBA_BASE) == EXAMPLE_FLEXSPI1_AMBA_BASE)
	{
		dstAddr = dst - EXAMPLE_FLEXSPI1_AMBA_BASE;
		flexspiInstance = FLEXSPI1_INSTANCE;
		norConfig = &flexspi1NorCfg;
	}
	else
	{
		dstAddr = dst - EXAMPLE_FLEXSPI2_AMBA_BASE;
		flexspiInstance = FLEXSPI2_INSTANCE;
		norConfig = &flexspi2NorCfg;
	}

	/* Only in case we are going to copy from encrypted region. */
	if((srcAddr >= SBL_MAIN_FW_ADDRESS) && (srcAddr < (SBL_MAIN_FW_ADDRESS+SBL_MAIN_FW_SIZE)))
	{
		isSrcAddrEncryptedXip = true;

		srcAddr -= ENCRYPTED_XIP_FLEXSPI_BASE;
		PRINTF("\r\nINFO: Flexspi Base address is  0x%08X \r\n", (uint32_t)ENCRYPTED_XIP_FLEXSPI_BASE);
	}

	/* Disable interrupts */
	primask = DisableGlobalIRQ();

	/* Allow CM7 read/write access to FlexSPI peripherals. */
	allowCm7ReadWriteToFlexSPIs();

	USER_LED3_ON();

	while((len >= PGM_PAGE_SIZE))
	{
		if(isSrcAddrEncryptedXip)
		{
			status = ROM_FLEXSPI_NorFlash_Read(flexspiInstance, norConfig, (uint32_t *)pgmBuffer, srcAddr, PGM_PAGE_SIZE);
			if (status != kStatus_Success)
			{
				PRINTF("\r\nERROR: Read from address 0x%08X via ROM API Read command failed!\r\n", srcAddr);
				goto cleanup;
			}
		}
		else
		{
			memcpy((void *)pgmBuffer, (void *)srcAddr, PGM_PAGE_SIZE);
		}

		status = ROM_FLEXSPI_NorFlash_ProgramPage(flexspiInstance, norConfig, dstAddr, (const uint32_t *)pgmBuffer);
		if (status != kStatus_Success)
		{
			PRINTF("\r\nERROR: Programming failed on address 0x%08X!\r\n", dstAddr);
			goto cleanup;
		}

		dstAddr += PGM_PAGE_SIZE;
		srcAddr += PGM_PAGE_SIZE;
		len -= PGM_PAGE_SIZE;

		USER_LED3_TOGGLE();
	}

	if (len)
	{
		memset((void *)pgmBuffer, 0xff, PGM_PAGE_SIZE);

		if(isSrcAddrEncryptedXip)
		{
			status = ROM_FLEXSPI_NorFlash_Read(flexspiInstance, norConfig, (uint32_t *)pgmBuffer, srcAddr, len);
			if (status != kStatus_Success)
			{
				PRINTF("\r\nERROR: Read from address 0x%08X via ROM API Read command failed!\r\n", srcAddr);
				goto cleanup;
			}
		}
		else
		{
			memcpy((void *)pgmBuffer, (void *)srcAddr, len);
		}
		status = ROM_FLEXSPI_NorFlash_ProgramPage(flexspiInstance, norConfig, dstAddr, (const uint32_t *)pgmBuffer);
		if (status != kStatus_Success)
		{
			PRINTF("\r\nERROR: Programming of the unaligned chunk failed!\r\n");
			goto cleanup;
		}

		USER_LED3_TOGGLE();
	}

	sssStatus = kStatus_SSS_Success;

cleanup:

	USER_LED3_OFF();
	/* Invalidate Data cache. */
	DCACHE_InvalidateByRange(dst, length);
	/* Allow CM7 read access to FlexSPI peripherals. */
	allowCm7ReadToFlexSPIs();
    /* Enable interrupts */
    EnableGlobalIRQ(primask);

	return sssStatus;
}

sss_status_t QMC2_FLASH_encXipMemcmp(uint32_t encXipAddress, uint32_t address, uint32_t length)
{
	sss_status_t sssStatus = kStatus_SSS_Fail;
	status_t status = kStatus_Fail;
    uint8_t ramBuffer[RAM_BUFFER_SIZE] = {0};
    uint8_t *pAddress = (uint8_t *)address;
    uint32_t primask = 0;
    uint32_t encXipAddr = (uint32_t)encXipAddress;

    if(length > SBL_MAIN_FW_SIZE)
    {
    	return kStatus_SSS_InvalidArgument;
    }
    /* If both pointer are the same. */
    if(encXipAddress == address)
    {
        return kStatus_SSS_InvalidArgument;
    }

	/* Only in case we are going to copy from encrypted region. */
	if(!((encXipAddr >= SBL_MAIN_FW_ADDRESS) && (encXipAddr < (SBL_MAIN_FW_ADDRESS+SBL_MAIN_FW_SIZE))))
	{
		return kStatus_SSS_InvalidArgument;
	}

	encXipAddr -= ENCRYPTED_XIP_FLEXSPI_BASE;

	/* Disable interrupts */
	primask = DisableGlobalIRQ();

	/* Allow CM7 read/write access to FlexSPI peripherals. */
	allowCm7ReadWriteToFlexSPIs();

	while(length >= RAM_BUFFER_SIZE)
	{
		status = ROM_FLEXSPI_NorFlash_Read(FLEXSPI2_INSTANCE, &flexspi2NorCfg, (uint32_t *)ramBuffer, encXipAddr, RAM_BUFFER_SIZE);
		if (status != kStatus_Success)
		{
			PRINTF("\r\nERROR: Read from address 0x%08X via ROM API Read command failed!\r\n", encXipAddr);
			goto cleanup;
		}

		for(uint32_t i=0; i<RAM_BUFFER_SIZE; i++)
		{
			if (ramBuffer[i] != *pAddress)
			{
				PRINTF("\r\nERROR: Memcpy failed!\r\n");
				goto cleanup;
			}
			pAddress++;
		}

		length -= RAM_BUFFER_SIZE;
		encXipAddr += RAM_BUFFER_SIZE;
	}

	if(length)
	{
		status = ROM_FLEXSPI_NorFlash_Read(FLEXSPI2_INSTANCE, &flexspi2NorCfg, (uint32_t *)ramBuffer, encXipAddr, length);
		if (status != kStatus_Success)
		{
			PRINTF("\r\nERROR: Read from address 0x%08X via ROM API Read command failed!\r\n", encXipAddr);
			goto cleanup;
		}

		for(uint32_t i=0; i<length; i++)
		{
			if (ramBuffer[i] != *pAddress)
			{
				PRINTF("\r\nERROR: Memcpy failed!\r\n");
				goto cleanup;
			}
			pAddress++;
		}
	}

	sssStatus = kStatus_SSS_Success;

cleanup:

		/* Allow CM7 read access to FlexSPI peripherals. */
		allowCm7ReadToFlexSPIs();

		/* Enable interrupts */
	    EnableGlobalIRQ(primask);

		return sssStatus;
	}

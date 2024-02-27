/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_flexspi.h"
#include "app.h"
#include "fsl_debug_console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define millisecToTicks(millisec) (((millisec)*configTICK_RATE_HZ + 999U) / 1000U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void flexspi1_octal_bus_init();
void flexspi1_octal_bus_init_with_rxSC( flexspi_read_sample_clock_t rxSampleClock);
status_t flexspi_nor_octalflash_write_config2_spi(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer);

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern flexspi_device_config_t g_deviceA1config;
extern flexspi_device_config_t g_deviceA2config;
extern const uint32_t g_customLUT[CUSTOM_LUT_LENGTH];

/*******************************************************************************
 * Code
 ******************************************************************************/

void flexspi1_octal_bus_init()
{
    status_t status;

    uint32_t reg = 0;
    flexspi1_octal_bus_init_with_rxSC( kFLEXSPI_ReadSampleClkLoopbackInternally);
    dbgDispPRINTF("Rootclock:%dMHz\n\r", CLOCK_GetRootClockFreq(kCLOCK_Root_Flexspi1)/1000000U);

	reg = 0x02; //16dummy for 166MHz DTR
	status = flexspi_nor_octalflash_write_config2_spi( FLEXSPI1, 0x300, &reg);
	if (status != kStatus_Success)
	{
		dbgDispPRINTF("IP Command read flash fail\n\r");
	}
	reg = 0x02; //DTR OPI mode
	status = flexspi_nor_octalflash_write_config2_spi( FLEXSPI1, 0, &reg);
	if (status != kStatus_Success)
	{
		dbgDispPRINTF("IP Command read flash fail\n\r");
	}

	flexspi1_octal_bus_init_with_rxSC( kFLEXSPI_ReadSampleClkExternalInputFromDqsPad);
    dbgDispPRINTF("Rootclock:%dMHz\n\r", CLOCK_GetRootClockFreq(kCLOCK_Root_Flexspi1)/1000000U);
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

status_t flexspi_nor_octalflash_read_status(FLEXSPI_Type *base, uint32_t *buffer)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = 0x0;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Read;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_READSTATUS;
    flashXfer.data          = buffer;
    flashXfer.dataSize      = 2;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

status_t flexspi_nor_octalflash_write_enable(FLEXSPI_Type *base)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = 0x0;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Command;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE;
    flashXfer.data          = NULL;
    flashXfer.dataSize      = 0;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);
    return status;
}

status_t flexspi_nor_octalflash_write_enable_spi_mode(FLEXSPI_Type *base)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = 0x0;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Command;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEENABLE_SPI;
    flashXfer.data          = NULL;
    flashXfer.dataSize      = 0;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

status_t flexspi_nor_wait_bus_busy(FLEXSPI_Type *base, TickType_t ticks )
{
    /* Wait status ready. */
    uint32_t readValue;
    status_t status;

    do
    {
        status = flexspi_nor_octalflash_read_status( base, &readValue);
        if (status != kStatus_Success)
        {
        	break;
        }
        vTaskDelay( ticks);
    } while(readValue & 1U);

    return status;
}

status_t flexspi_nor_octalflash_read_data(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer, size_t size)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = addr;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Read;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_READDATA;
    flashXfer.data          = buffer;
    flashXfer.dataSize      = size;

    status  = FLEXSPI_TransferBlocking(base, &flashXfer);
    return status;
}

status_t flexspi_nor_octalflash_write_data(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer, size_t size)
{
    flexspi_transfer_t flashXfer;
    status_t status1, status = kStatus_InvalidArgument;

	while( size)
	{
		status = flexspi_nor_octalflash_write_enable( base);
		if( status != kStatus_Success)
			return MAKE_STATUS( 180, status);

		//Max pgm size is 256 (OCTAL_FLASH_PAGE_SIZE)
		size_t siz_pgm = size>OCTAL_FLASH_PAGE_SIZE?OCTAL_FLASH_PAGE_SIZE:size;

		flashXfer.deviceAddress = addr;
		flashXfer.port          = kFLEXSPI_PortA1;
		flashXfer.cmdType       = kFLEXSPI_Write;
		flashXfer.SeqNumber     = 1;
		flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_WRITEDATA;
		flashXfer.data          = buffer;
		flashXfer.dataSize      = siz_pgm;

		status  = FLEXSPI_TransferBlocking(base, &flashXfer);

		SCB_InvalidateDCache_by_Addr ( (void*)(FlexSPI1_AMBA_BASE + MAKE_EVEN(addr)), size);

		/* Do software reset or clear AHB buffer directly. */
		base->AHBCR |= FLEXSPI_AHBCR_CLRAHBRXBUF_MASK;
		base->AHBCR &= ~FLEXSPI_AHBCR_CLRAHBRXBUF_MASK;

		if( status != kStatus_Success)
		{
			return MAKE_STATUS( 181, status);
		}

		status1 = flexspi_nor_wait_bus_busy( base, millisecToTicks(1));

		if( status1 != kStatus_Success)
		{
			return MAKE_STATUS( 182, status1);
		}

		size -= siz_pgm;
	}
    return status;
}

//Erases 4K sector
status_t flexspi_nor_octalflash_erase_sector(FLEXSPI_Type *base, uint32_t addr)
{
    flexspi_transfer_t flashXfer;
    status_t status, status1;

    status = flexspi_nor_octalflash_write_enable( base);
    if( status != kStatus_Success)
    	return MAKE_STATUS( 183, status);

    flashXfer.deviceAddress = addr;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Command;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_ERASESECTOR;

    status  = FLEXSPI_TransferBlocking(base, &flashXfer);

	SCB_InvalidateDCache_by_Addr ( (void*)(FlexSPI1_AMBA_BASE + MAKE_EVEN(addr)), 4096);

	/* Do software reset or clear AHB buffer directly. */
    base->AHBCR |= FLEXSPI_AHBCR_CLRAHBRXBUF_MASK;
    base->AHBCR &= ~FLEXSPI_AHBCR_CLRAHBRXBUF_MASK;

    if( status != kStatus_Success)
    {
    	return MAKE_STATUS( 184, status);
    }

	status1 = flexspi_nor_wait_bus_busy( base, millisecToTicks(30));

    if( status1 != kStatus_Success)
    {
    	return MAKE_STATUS( 185, status1);
    }

    return status;
}

status_t flexspi_nor_octalflash_read_ID(FLEXSPI_Type *base, uint32_t *buffer)
{
    flexspi_transfer_t flashXfer;
    status_t status;
    uint32_t lbuf[2];

    flashXfer.deviceAddress = 0x00;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Read;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_READID;
    flashXfer.data          = lbuf;
    flashXfer.dataSize      = 6;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);

    uint8_t *bbuf = (uint8_t*)lbuf;
    *buffer = (*buffer<<8) | bbuf[1];
    *buffer = (*buffer<<8) | bbuf[3];
    *buffer = (*buffer<<8) | bbuf[5];

    return status;
}

status_t flexspi_nor_octalflash_read_config2(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = addr;
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Read;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG2;
    flashXfer.data          = buffer;
    flashXfer.dataSize      = 1;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

status_t flexspi_nor_octalflash_write_config2(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = addr & (OCTAL_FLASH_SIZE-1);	 //Mask address to stay in flash SS0_B chipselect range
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Write;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2;
    flashXfer.data          = buffer;
    flashXfer.dataSize      = 1;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

status_t flexspi_nor_octalflash_read_config(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    flashXfer.deviceAddress = addr & (OCTAL_FLASH_SIZE-1);	 //Mask address to stay in flash SS0_B chipselect range
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Read;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_READCFG;
    flashXfer.data          = buffer;
    flashXfer.dataSize      = 1;
    status                  = FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

status_t flexspi_nor_octalflash_write_config2_spi(FLEXSPI_Type *base, uint32_t addr, uint32_t *buffer)
{
    flexspi_transfer_t flashXfer;
    status_t status;

    status = flexspi_nor_octalflash_write_enable_spi_mode( base);
    if( status != kStatus_Success)
    {
    	return status;
    }

    flashXfer.deviceAddress = addr & (OCTAL_FLASH_SIZE-1);	 //Mask address to stay in flash SS0_B chipselect range
    flashXfer.port          = kFLEXSPI_PortA1;
    flashXfer.cmdType       = kFLEXSPI_Config;
    flashXfer.SeqNumber     = 1;
    flashXfer.seqIndex      = OCTALFLASH_CMD_LUT_SEQ_IDX_WRITECFG2_SPI;
    flashXfer.data          = buffer;
    flashXfer.dataSize      = 1;
    status					= FLEXSPI_TransferBlocking(base, &flashXfer);

    return status;
}

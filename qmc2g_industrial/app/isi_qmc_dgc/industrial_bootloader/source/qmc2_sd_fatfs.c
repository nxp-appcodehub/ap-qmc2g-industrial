/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include <qmc2_sd_fatfs.h>
#include <stdio.h>
#include <string.h>
#include "fsl_sd.h"
#include "fsl_debug_console.h"
#include "ff.h"
#include "diskio.h"
#include "fsl_sd_disk.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "sdmmc_config.h"
#include "fsl_usdhc.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* buffer size (in byte) for read/write operations */
#define BUFFER_SIZE (4096)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief wait card insert function.
 */
/*******************************************************************************
 * Variables
 ******************************************************************************/
static FATFS g_fileSystem; /* File system object */
static FIL g_fileObject;   /* File object */

/* @brief decription about the read/write buffer
 * The size of the read/write buffer should be a multiple of 512, since SDHC/SDXC card uses 512-byte fixed
 * block length and this driver example is enabled with a SDHC/SDXC card.If you are using a SDSC card, you
 * can define the block length by yourself if the card supports partial access.
 * The address of the read/write buffer should align to the specific DMA data buffer address align value if
 * DMA transfer is used, otherwise the buffer address is not important.
 * At the same time buffer address/size should be aligned to the cache line size if cache is supported.
 */
/*! @brief Data written to the card */
SDK_ALIGN(uint8_t g_bufferWrite[BUFFER_SIZE], BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);
/*! @brief Data read from the card */
SDK_ALIGN(uint8_t g_bufferRead[BUFFER_SIZE], BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);
/*******************************************************************************
 * Code
 ******************************************************************************/

FRESULT error;
DIR directory; /* Directory object */
FILINFO fileInformation;
UINT bytesWritten;
UINT bytesRead;
const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};
volatile bool failedFlag           = false;
char ch                            = '0';
BYTE work[FF_MAX_SS];

static sd_card_t *card = &g_sd;
static FIL *fileObj = &g_fileObject;

static sss_status_t SDCARD_FATFS_isCardInserted(sd_card_t *card, uint32_t status);

static sss_status_t SDCARD_FATFS_isCardInserted(sd_card_t *card, uint32_t status)
{
    assert(card != NULL);
    assert(card->usrParam.cd != NULL);
    uint8_t attempts = 10;

    if (card->usrParam.cd->type == kSD_DetectCardByGpioCD)
    {
        if (card->usrParam.cd->cardDetected == NULL)
        {
            return kStatus_SSS_Fail;
        }

        do
        {
            if ((card->usrParam.cd->cardDetected() == true) && (status == (uint32_t)kSD_Inserted))
            {
                SDMMC_OSADelay(card->usrParam.cd->cdDebounce_ms);
                if (card->usrParam.cd->cardDetected() == true)
                {
                    return kStatus_SSS_Success;
                }
            }

            if ((card->usrParam.cd->cardDetected() == false) && (status == (uint32_t)kSD_Removed))
            {
                break;
            }
        } while (attempts--);
    }
    else
    {
        if (card->isHostReady == false)
        {
            return kStatus_SSS_Fail;
        }

        if (SDMMCHOST_PollingCardDetectStatus(card->host, status, ~0U) != kStatus_SSS_Success)
        {
            return kStatus_SSS_Fail;
        }
    }

    return kStatus_SSS_Fail;
}

/* Delete file or folder */
sss_status_t QMC2_SD_FATFS_Delete(const char* path)
{
	FRESULT error = FR_NO_FILE;

	assert(path != NULL);

	// Pointer to the file or directory path
	error =  f_unlink (_T(path));
    if (error)
    {
		PRINTF("Delete op of the file failed.\r\n");
		return kStatus_SSS_Fail;
    }

    return kStatus_SSS_Success;
}

sss_status_t QMC2_SD_FATFS_Open(const char* path)
{
	FRESULT error = FR_NO_FILE;

	assert(path != NULL);

    error = f_open(fileObj, _T(path), (FA_READ | FA_OPEN_EXISTING));
    if (error)
    {
        if (error == FR_EXIST)
        {
            PRINTF("File exists.\r\n");
        }
        else
        {
            PRINTF("Open file failed.\r\n");
            return kStatus_SSS_Fail;
        }
    }

    return kStatus_SSS_Success;
}

sss_status_t QMC2_SD_FATFS_Read(uint8_t *buffer, uint32_t length, uint32_t offset)
{
	FRESULT error = FR_NO_FILE;
	UINT bytesRead = 0;

	assert(buffer != NULL);
	assert(length <= PGM_PAGE_SIZE);

	/* Move the file pointer */
	if (f_lseek(fileObj, offset))
	{
		PRINTF("Set file pointer position failed. \r\n");
		return kStatus_SSS_Fail;
	}

    error = f_read(fileObj, (void *)buffer, (UINT)length, &bytesRead);
    if ((error) || (bytesRead != length))
    {
        PRINTF("Read file failed. \r\n");
        return kStatus_SSS_Fail;
    }

    return kStatus_SSS_Success;

}

sss_status_t QMC2_SD_FATFS_Close(void)
{
    if (f_close(fileObj))
    {
        PRINTF("\r\nClose file failed.\r\n");
        return kStatus_SSS_Fail;
    }

    return kStatus_SSS_Success;
}

sss_status_t QMC2_SD_FATFS_Init(bool *isSdCardInserted)
{
	status_t status = kStatus_Fail;

	*isSdCardInserted = false;
	 //InstallIRQHandler(Reserved177_IRQn, (uint32_t)SoftwareHandler);
	BOARD_SD_Config(card, NULL, BOARD_SDMMC_SD_HOST_IRQ_PRIORITY, NULL);

    /* SD host init function */
	status = SD_HostInit(card);
    if (status != kStatus_Success)
    {
        PRINTF("\r\nSD host init fail\r\n");
        return kStatus_SSS_Fail;
    }

    /* wait card insert */
    if (SDCARD_FATFS_isCardInserted(card, kSD_Inserted) != kStatus_SSS_Success)
    {
        /* host deinitialize */
        SD_HostDeinit(card);
        /* Return success. The card is not inserted. */
    	return kStatus_SSS_Success;
    }
    PRINTF("\r\nInfo: SDCARD is inserted!\r\n");

    /* power off card */
    SD_SetCardPower(card, false);
    /* power on the card */
    SD_SetCardPower(card, true);

    status = f_mount(&g_fileSystem, driverNumberBuffer, 0U);
    if (status != kStatus_Success)
    {
		PRINTF("Mount volume failed.\r\n");
		return kStatus_SSS_Fail;
	}

#if (FF_FS_RPATH >= 2U)
    error = f_chdrive((char const *)&driverNumberBuffer[0U]);
    if (error)
    {
        PRINTF("Change drive failed.\r\n");
        return kStatus_SSS_Fail;
    }
#endif

	*isSdCardInserted = true;
    return kStatus_SSS_Success;
}

sss_status_t QMC2_SD_FATFS_DeInit(void)
{
    /* Card deinitialize */
    SD_CardDeinit(card);
    /* Host deinitialize */
    SD_HostDeinit(card);

    GPIO_PortDisableInterrupts(BOARD_SDMMC_SD_CD_GPIO_BASE, 1U << BOARD_SDMMC_SD_CD_GPIO_PIN);
    GPIO_PortClearInterruptFlags(BOARD_SDMMC_SD_CD_GPIO_BASE, ~0);
    /* Open card detection pin NVIC. */
    DisableIRQ(BOARD_SDMMC_SD_CD_IRQ);

    return kStatus_SSS_Success;
}


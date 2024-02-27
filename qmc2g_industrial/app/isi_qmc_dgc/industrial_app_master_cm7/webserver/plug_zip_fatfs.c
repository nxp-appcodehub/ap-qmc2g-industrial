/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#if PLUG_ZIP_FS_FATFS
#include <stdio.h>
#include "ff.h"

/**
 * @brief filesystem io handler for read
 */
int fatfs_read_handler(void *fd, char *buf, int nbytes)
{
    FIL *fp = fd;
    FRESULT rc;
    UINT br = 0;
    rc      = f_read(fp, buf, nbytes, &br);
    return (rc == FR_OK) ? br : -1;
}
/**
 * @brief filesystem io handler for write
 */
int fatfs_write_handler(void *fd, const char *buf, int nbytes)
{
    FIL *fp = fd;
    FRESULT rc;
    UINT bw = 0;
    rc      = f_write(fp, buf, nbytes, &bw);
    return (rc == FR_OK) ? bw : -1;
}
/**
 * @brief filesystem io handler for seek
 */
fpos_t fatfs_seek_handler(void *fd, off_t offset, int whence)
{
    FIL *fp = fd;
    FRESULT rc;
    FSIZE_t pos;
    rc = FR_INT_ERR;

    switch (whence)
    {
        case SEEK_SET:
            rc = f_lseek(fp, offset);
            break;
        case SEEK_CUR:
            pos = f_tell(fp);
            rc  = f_lseek(fp, pos + offset);
            break;
        case SEEK_END:
            pos = f_size(fp);
            rc  = f_lseek(fp, pos + offset);
            break;
    }
    return (rc == FR_OK) ? f_tell(fp) : -1;
}

/**
 * @brief filesystem io handler for close
 */
static int fatfs_close_handler(void *fd)
{
    FIL *fp = fd;
    FRESULT rc;
    rc = f_close(fp);
    return rc == FR_OK ? 0 : -1;
}


/**
 * @brief open a fatfs file, and return an handler based  FILE object
 *
 * @param fp     fatfs FIL pointer to use
 * @param path   path to open
 *
 * @return  FILE pointer to the opened file using the callbacks of this file
 */
FILE *plug_zip_open_fatfs(FIL *fp, const char *path)
{
    FRESULT rc;
    FILE *fd = NULL;
    rc       = f_open(fp, path, FA_READ);
    if (rc == FR_OK)
    {
        fd = funopen(fp, fatfs_read_handler, fatfs_write_handler, fatfs_seek_handler, fatfs_close_handler);
    }
    return fd;
};

#endif

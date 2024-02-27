/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CIT_HTTPD_PLUG_ZIP_FS_H
#define CIT_HTTPD_PLUG_ZIP_FS_H

#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "contrib/incbin.h"
#include "plug.h"

#ifndef PLUG_ZIP_FS_FATFS
#define PLUG_ZIP_FS_FATFS 0
#endif

PLUG_EXTENSION_PROTOTYPE(plug_zip_fs);


/**
 * @brief data/size tuple passed into the plug_zip rules
 */
struct plug_fs_info
{
    const unsigned char *data;
    unsigned int size;
};

#define PLUG_ZIP_FS_FILE(name, filename)                                              \
    INCBIN(name, filename);                                                           \
    struct plug_fs_info static name(void)                                             \
    {                                                                                 \
        return (struct plug_fs_info){.data = g##name##_data, .size = g##name##_size}; \
    }                                                                                 \
    //

#if PLUG_ZIP_FS_FATFS
#define PLUG_ZIP_FS_FATFS_PATH(name, path)                                            \
    struct plug_fs_info static name(void)                                             \
    {                                                                                 \
        return (struct plug_fs_info){.data = (const unsigned char *)path, .size = 0}; \
    }                                                                                 \
//
#endif

#endif

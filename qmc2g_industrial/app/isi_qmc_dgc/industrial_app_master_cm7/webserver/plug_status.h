/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CIT_HTTPD_PLUG_STATUS_H
#define CIT_HTTPD_PLUG_STATUS_H
#pragma once
#include "plug.h"


/**
 * @brief response format (based on mime-type)
 */
enum plug_status_mode
{
    TEXT_STATUS = 0,
    HTML_STATUS,
    JSON_STATUS,
};

PLUG_EXTENSION_PROTOTYPE(plug_status);
#endif

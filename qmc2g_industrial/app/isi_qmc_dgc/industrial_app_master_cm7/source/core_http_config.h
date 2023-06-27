/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @file core_http_config.h
 * @brief Overrides the default HTTP client configuration in core_http_config_defaults.h
 *
 */

#ifndef CORE_HTTP_CONFIG_H
#define CORE_HTTP_CONFIG_H

#define HTTP_USER_AGENT_VALUE      "QMC2G"
#define HTTP_RECV_RETRY_TIMEOUT_MS (1500U)

#endif

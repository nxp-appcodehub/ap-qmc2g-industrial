/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */
#ifndef __qmc2_log_h_
#define __qmc2_log_h_

//-----------------------------------------------------------------------
// Linker macros placing include file
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "qmc2_types.h"
#include "api_logging.h"
//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------

//
//------------------------------------------------------------------------------
// Enum
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Typedef
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------
/* Create log entry */
sss_status_t QMC2_LOG_CreateLogEntry(log_event_code_t *log);
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
#endif /* __qmc2_log_h_ */

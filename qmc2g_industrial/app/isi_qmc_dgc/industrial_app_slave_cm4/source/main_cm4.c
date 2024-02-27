/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

/*!
 * @mainpage QMC 2G M4 Cyber Resiliency Environment
 *
 * @section intro_sec Introduction
 *
 * The M4 part of the QMC 2G code runs isolated on the M4 core, and its main responsibility is implementing the cyber
 * resilience feature. The most important part of the cyber resilience feature is the authenticated watchdog timer. The
 * authenticated watchdog timer must be serviced with signed tickets issued by a genuine server. Should such a
 * ticket not arrive before the authenticated watchdog expires, the system is reset and goes into restricted
 * mode. The state of the AWDG is periodically saved to a secure non-volatile storage so that it can be restored after
 * unexpected or malicious system resets. The implementation of a compatible server can be found in the folder `qmc_awdt_server`. 
 * If the cyber resiliency feature is used, do not forget to set the corresponding server host name in @ref qmc_features_config.h of 
 * the "qmc2g_industrial_M7MASTER" project.
 *
 * Apart from the cyber resiliency feature, the M4 code implements certain non-security features that are needed by the
 * main application running on the M7 core. These include:
 *  - A remote procedure call module allowing the main application on the M7 to communicate with the M4.
 *  - A configurable number of functional watchdogs the main application can use.
 *  - An interface allowing the main application to set user outputs and get notified about user input changes (GPIO13).
 *  - An interface allowing the main application to get and set the current time (real-time clock).
 *  - An interface allowing the main application to write and read the firmware update status and reset cause to and from a
 * battery-backed register.
 *  - An interface allowing the main application to perform system resets.
 *
 * @section configuration_sec Configuration
 *
 * Common configuration options shared between M4 and M7 code are in the file @ref qmc_features_config.h. The only relevant
 * options for the M4 are:
 *  - `FEATURE_SECURE_WATCHDOG`: Enables (1) or disables (0) the authenticated watchdog timer, hence cyber
 * resiliency.
 *
 * Additionally, the following definitions influence the build flow:
 *  - `NO_SBL`: If defined, it is assumed no SBL exists. Note that this also leads to using a static key and seed
 * for the authenticated watchdog timer defined in the file @ref qmc_cm4_features_config.h. **DO NOT USE IN PRODUCTION!**
 *  - `ENABLE_M4_DEBUG_CONSOLE`: If defined, the M4 UART and debug console are initialized. Use this flag only for M4
 * debugging, debug output will mix with debug output from M7!
 *
 * All other M4-specific configuration options are in the file @ref qmc_cm4_features_config.h.
 *
 * @section module_sec Modules
 *
 * The following list provides a short overview of the M4 modules:
 *  - @ref main_cm4.c Main entry file for M4 execution.
 *  - @ref awdg Implementation of the authenticated watchdog timer essential for the cyber resiliency feature.
 *  - @ref hal Hardware abstraction layer for the IMX RT1176.
 *  - @ref lwdg Implementation of the functional watchdog timers used standalone and for the authenticated watchdog
 * timer.
 *  - @ref qmc_cm4 Main implementation of the M4 functionality (initialization, high-level feature implementation, glue
 * logic to lower-level modules, interrupt handlers).
 *  - @ref rpc Remote procedure call module allowing the communication between M4 and M7.
 *  - @ref utils Various helper functions.
 */

/*!
 * @file main_cm4.c
 * @brief Main entry file of the QMC2G M4 core code.
 *
 * To debug or run together with the M7 software and without SBL follow these steps:
 *
 *  1) Compile the CM4 project with the "NO_SBL" define set
 *      - This will use a static RNG and PK as replacement for the data received from the SBL
 *          - !!! only for testing or debugging, do not set this define for production use !!!
 *  2) Start debugging M7 and run until the instruction after BOARD_InitBootPeripherals() in main()
 *  3) Start debugging M4 and let it run
 *  4) Continue M7 core
 *
 *  Note that for debugging or testing purposes the AWDG timeout is set to 24 hours, if the sum of your
 *  debugging times exceeds this timeout without the AWDG being kicked, then the system is reboot once.
 *  This timeout and other CM4 implementation specific options can be changed in the file @ref qmc_cm4_features.h.
 */

#include "hal/hal.h"
#include "qmc_cm4/qmc_cm4_api.h"
#include "utils/debug_log_levels.h"
#ifndef DEBUG_LOG_LEVEL
#define DEBUG_LOG_LEVEL DEBUG_LOG_LEVEL_SILENT
#endif
#include "utils/debug.h"
#include "utils/testing.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main entry point of the CM4 software (cyber resiliency implementation).
 *
 * @startuml
 * start
 *   if (QMC_CM4_Init()) then (failed)
 *     :QMC_CM4_HandleHardwareWatchdogISR(); 
 *     stop
 *   endif
 *   while (forever)
 *     :QMC_CM4_SyncSnvsRpcStateMain();  
 *   endwhile
 *   -[hidden]->
 *   detach
 * @enduml
 * 
 * Initializes the hardware and software. If initialization fails, reboot into
 * recovery mode (acting as if the hardware watchdog expired).
 * 
 * After successful initialization, continuously synchronizes system state to
 * SNVS and processes RPCs. 
 */
int main(void)
{
    qmc_status_t ret = QMC_CM4_Init();
    if (kStatus_QMC_Ok != ret)
    {
        DEBUG_LOG_E(DEBUG_M4_TAG "Initialization failed.\r\n");
        HAL_EnterCriticalSectionNonISR();
        /* act like the hardware watchdog expired to trigger recovery mode */
        QMC_CM4_HandleHardwareWatchdogISR();
        HAL_ExitCriticalSectionNonISR();
    }

    /* welcome message */
    DEBUG_LOG_I(DEBUG_M4_TAG "QMC2G code SLAVE core started.\r\n");

    /* now communication should be up and running */

    /* main loop:
     *  - syncs SNVS software mirrors with hardware register
     *  - processes delayed RPC calls
     */
    while (FOREVER())
    {
        /* QMC CM4 non-interrupt processing function: 
         * - synchs SNVS data
         * - processes delayed RPC calls */
        QMC_CM4_SyncSnvsRpcStateMain();
    }

    return 0;
}

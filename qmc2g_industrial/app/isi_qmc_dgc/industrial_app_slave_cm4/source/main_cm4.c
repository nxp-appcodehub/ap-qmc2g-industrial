/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

/*!
 * @file main_cm4.c
 * @brief Main entry file of the QMC2G M4 core code.
 *
 * To debug / run together with the M7 software and without SBL follow these steps:
 *
 *  1) Compile the CM4 project with the "NO_SBL" define set
 *      - This will use a static RNG and PK as replacement for the data received from the SBL
 *          - !!! only for testing / debugging, do not set this define for production use !!!
 *  2) Start debugging M7 and run until the instruction after BOARD_InitBootPeripherals() in main()
 *  3) Start debugging M4 and let it run
 *  4) Continue M7 core
 *
 *  Note that for debugging / testing purposes the AWDG timeout is set to 24 hours, if the sum of your
 *  debugging times exceeds this timeout without the AWDG being kicked then the system is reboot once.
 *  This timeout and other CM4 implementation specific options can be changed in the file "qmc_cm4_features.h".
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
int main(void)
{
    qmc_status_t ret = kStatus_QMC_Err;
    ret = QMC_CM4_Init();
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
         * - synches SNVS data
         * - processes delayed RPC calls */
        QMC_CM4_SyncSnvsRpcStateMain();
    }

    return 0;
}

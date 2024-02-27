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
 * @file testing.h
 * @brief Macros to aid testing.
 *
 */

#ifndef _TESTING_H_
#define _TESTING_H_

#include <stdint.h>
#include <assert.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Macro to make static variable visible for testing. */
#ifndef TESTING
#define STATIC_TEST_VISIBLE static
#else
#define STATIC_TEST_VISIBLE
#endif

/*! @brief Macro to bypass endless loops for testing. */
#ifndef TESTING
#define FOREVER() 1
#else 
int FOREVER(void);
#endif

#endif /* _TESTING_H_ */

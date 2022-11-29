/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html. The production use license in Section 2.3 is expressly granted for this software.
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

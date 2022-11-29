/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#ifndef SECURE_ELEMENT_H_
#define SECURE_ELEMENT_H_

#include "api_qmc_common.h"
#include "fsl_sss_api.h"

/*!
 * @brief Init SE Hostlib and open SE session
 *
 * Must be called after the scheduler has started
 */
qmc_status_t SE_Initialize(void);

/*!
 * @brief Deinit SE Hostlib and close SE session
 */
void SE_Deinitialize(void);

/*!
 * @brief Checks the status of the SE session
 *
 * @return True if session open else False
 */
bool SE_IsInitialized(void);

/*!
 * @brief Get pointer to SE session
 */
sss_session_t* SE_GetSession(void);

/*!
 * @brief Get pointer to SE key store
 */
sss_key_store_t* SE_GetKeystore(void);

/*!
 * @brief Get pointer to HOST session
 */
sss_session_t* SE_GetHostSession(void);

/*!
 * @brief Get pointer to HOST key store
 */
sss_key_store_t* SE_GetHostKeystore(void);

/*!
 * @brief Get the unique ID of the SE
 */
const char* SE_GetUid(void);

#endif /* SECURE_ELEMENT_H_ */

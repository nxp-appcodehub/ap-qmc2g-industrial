/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef _API_BOARD_H_
#define _API_BOARD_H_

#include "api_qmc_common.h"


/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initialize the BoardAPI. Scheduler must be already running.
 */
qmc_status_t BOARD_Init();

/*!
 * @brief Get the current temperature in degree Celsius from the temperature sensor on the Digital Board.
 *
 * @param[out] temperature Pointer to write the temperature to
 */
qmc_status_t BOARD_GetDbTemperature(float* temperature);

 /*!
 * @brief Set alarm parameters for the temperature sensor on the digital board.
 *
 * The temperature sensor on the Digital Board can set a pin, if the temperature exceeds the value indicated by threshold.
 * If the temperature drops below the value indicated by hysteresis, the pin is cleared again. This hardware signal may be
 * used as an interrupt source. However, it is not connected to the system by default.
 *
 * @param[in] threshold Above this temperature the alarm is triggered
 * @param[in] hysteresis Below this temperature the alarm is turned off
 */
qmc_status_t BOARD_SetDbTemperatureAlarm(float threshold, float hysteresis);

/*!
 * @brief Get the temperature in degree Celsius from the selected temperature sensor on one of the Power Stage Boards.
 *
 * This function may return cached values read by the Board Service task instead of reading the sensor directly.
 *
 * @param[out] temperature Pointer to write the temperature to
 * @param[in]  psb Sensor to read the temperature from
 */
qmc_status_t BOARD_GetPsbTemperature(double* temperature, qmc_psb_temperature_id_t psb);

 /*!
 * @brief Sets the output state of the given user output pin.
 *
 * For slow output pins on the SNVS domain, the call is forwarded to the internal function BOARD_SetSnvsOutput(mode : qmc_output_cmd_t, pin : qmc_output_id_t) : qmc_status_t.
 *
 * @param[in] mode State to set the pin to
 * @param[in] pin Pin to apply the state to
 */
qmc_status_t BOARD_SetDigitalOutput(qmc_output_cmd_t mode, qmc_output_id_t pin);

 /*!
 * @brief Sets the LED state indicated by qmc_led_cmd_t of the given user LED.
 *
 * @param[in] mode State to set the LED to
 * @param[in] led LED to apply the state to
 */
void BOARD_SetUserLed(qmc_led_cmd_t mode, qmc_led_id_t led);

 /*!
 * @brief Convert a qmc_timestamp_t to a qmc_datetime_t structure.
 *
 * @param[in]  timestamp Pointer to the timestamp to convert
 * @param[out] dt Pointer to write the conversion result to
 */
qmc_status_t BOARD_ConvertTimestamp2Datetime(const qmc_timestamp_t* timestamp, qmc_datetime_t* dt);

 /*!
 * @brief Convert a qmc_datetime_t structure to a qmc_timestamp_t.
 *
 * @param[in]  dt Pointer to the datetime structure to convert
 * @param[out] timestamp Pointer to write the conversion result to
 */
qmc_status_t BOARD_ConvertDatetime2Timestamp(const qmc_datetime_t* dt, qmc_timestamp_t* timestamp);

 /*!
 * @brief Get a timestamp derived from the internal systick counter.
 *
 * The function shall add the offset to the wall-clock time retrieved by BOARD_GetTimeFromRTCSync(timestamp : qmc_timestamp_t*) : qmc_status_t during system start-up.
 *
 * @param[out] timestamp Pointer to write the retrieved timestamp to
 */
qmc_status_t BOARD_GetTime(qmc_timestamp_t* timestamp);

/*!
* @brief Set a callback function that shall be called whenever RPC_SetTimeToRTC(timestamp : qmc_timestamp_t*) : qmc_status_t successfully changed the system time
*
* @param[in] callback Pointer the callback function; passing NULL deactivates the callback
*/
void BOARD_SetTimeChangedCallback(void (*callback)(void));

/*!
* @brief Set the lifecycle of the QMC system
*
* The function shall set the system event bits so that only the specified lifecycle is set and all others are clear.
*
* @param lc the lifecycle to set
*/
qmc_status_t BOARD_SetLifecycle(qmc_lifecycle_t lc);

/*!
* @brief Get the lifecycle of the QMC system
*
* @return The current lifecycle.
*/
qmc_lifecycle_t BOARD_GetLifecycle(void);

/*!
* @brief Get the version of the currently running firmware. It is retrieved from the firmware manifest section in flash.
*/
qmc_fw_version_t BOARD_GetFwVersion();

/*!
* @brief Get a unique identifier for the system.
*
* @return A const char* pointer to the zero-terminated identifier string, or NULL if the identifier cannot be retrieved.
*/
const char* BOARD_GetDeviceIdentifier();


#endif /* _API_BOARD_H_ */

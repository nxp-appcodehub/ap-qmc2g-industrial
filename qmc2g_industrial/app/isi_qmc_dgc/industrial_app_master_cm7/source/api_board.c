/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "qmc_features_config.h"
#include "api_board.h"
#include "api_rpc.h"
#include "api_rpc_internal.h"
#include "fsl_gpio.h"
#include "fsl_lpi2c.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "board.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SECONDS_PER_MINUTE      (60)
#define SECONDS_PER_HOUR        (SECONDS_PER_MINUTE * 60)
#define SECONDS_PER_DAY         (SECONDS_PER_HOUR * 24)
#define DAYS_PER_YEAR           (365)
#define DAYS_PER_4_YEARS        (DAYS_PER_YEAR * 4 + 1)
#define DAYS_PER_100_YEARS      (DAYS_PER_4_YEARS * 25 - 1)
#define DAYS_PER_400_YEARS      (DAYS_PER_100_YEARS * 4 + 1)
#define LEAP_YEARS_BEFORE_EPOCH (477)
#define DAYS_BEFORE_EPOCH       (1970 * DAYS_PER_YEAR + LEAP_YEARS_BEFORE_EPOCH )

#define LEAP_YEARS_SINCE_YEAR(y)  (((y) >> 2)  - ((y) / 100) + ((y) / 400))
#define LEAP_YEARS_SINCE_EPOCH(y) (LEAP_YEARS_SINCE_YEAR(y) - LEAP_YEARS_BEFORE_EPOCH)

#define TEMP_REG            	0x00		/* Temperature register */
#define CONF_REG				0x01		/* Configuration register */
#define THYST_REG           	0x02		/* Thyst register to set the hysteresis limit */
#define TOS_REG             	0x03		/* Tos register to set overtemperature shutdown limit */
#define TIDLE_REG				0x04		/* Temperature conversion cycle register */

#define PCT2075_I2C_ADDRESS		0x49		/* A2 = A1 = 0, A0 = 1 */

/*******************************************************************************
 * Variables
 ******************************************************************************/

const static uint16_t gs_daysInYearPerMonth[]        = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
const static uint8_t  gs_modularDaysInYearPerMonth[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4}; /* t[] for Sakamoto's method */

bool g_needsRefresh_getTime = false;

static bool              gs_isInitialized_getTime = false;
static TimerHandle_t     gs_getTime_OverflowProtectionTimerHandle;
static StaticTimer_t     gs_getTime_OverflowProtectionTimer;
static SemaphoreHandle_t gs_getTimeMutexHandle;
static StaticSemaphore_t gs_getTimeMutex;
static SemaphoreHandle_t gs_SnvsAccessMutexHandle;
static StaticSemaphore_t gs_SnvsAccessMutex;
static qmc_timestamp_t   gs_getTimeOffset;
static TickType_t        gs_getTimeLastRefresh;

extern SemaphoreHandle_t gSmComlock;
extern double g_PSBTemps[8];
extern EventGroupHandle_t g_systemStatusEventGroupHandle;


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static bool          isLeapYear(uint16_t year);
static qmc_weekday_t computeDayOfWeek(qmc_datetime_t *dt);
static void getTime_OverflowProtectionTimerCallback(TimerHandle_t xTimer);

/*******************************************************************************
 * Code
 ******************************************************************************/

qmc_status_t BOARD_Init()
{
	/* initialize mutexes */
	gs_getTimeMutexHandle    = xSemaphoreCreateMutexStatic(&gs_getTimeMutex);
	gs_SnvsAccessMutexHandle = xSemaphoreCreateMutexStatic(&gs_SnvsAccessMutex);
    if( (NULL == gs_getTimeMutexHandle) || (NULL == gs_SnvsAccessMutexHandle) )
    	return kStatus_QMC_Err;


    /* initialize timer */
	gs_getTime_OverflowProtectionTimerHandle = xTimerCreateStatic("getTime ovf.", /* one day before overflow */
			                                                      UINT32_MAX - pdMS_TO_TICKS(SECONDS_PER_DAY * 1000),
																  pdFALSE, NULL,
																  getTime_OverflowProtectionTimerCallback,
																  &gs_getTime_OverflowProtectionTimer);
	if( NULL == gs_getTime_OverflowProtectionTimerHandle )
	    return kStatus_QMC_Err;

	/* get current time / tick count */
	if(kStatus_QMC_Ok != RPC_GetTimeFromRTC(&gs_getTimeOffset))
		return kStatus_QMC_Err;
	gs_getTimeLastRefresh = xTaskGetTickCount();
	xTimerStart(gs_getTime_OverflowProtectionTimerHandle, 0);

	gs_isInitialized_getTime = true;

	return kStatus_QMC_Ok;
}

qmc_status_t BOARD_GetDbTemperature(float* temperature)
{
	status_t retVal = kStatus_Fail;
	uint8_t TemperatureData[2] = {0};
	int16_t Temperature_11_bit = 0;

	/* input sanitation and limit checks */
	if(NULL == temperature)
		return kStatus_QMC_ErrArgInvalid;

	/* take mutex */
	xSemaphoreTake(gSmComlock, portMAX_DELAY);

	retVal = LPI2C_MasterStart(BOARD_GENERAL_I2C_BASEADDR, PCT2075_I2C_ADDRESS, kLPI2C_Write);
	if (retVal != kStatus_Success)
	{
		/* Failed to start I2C master */
		goto i2c_error;
	}

	retVal = LPI2C_MasterSend(BOARD_GENERAL_I2C_BASEADDR, &(uint8_t){TEMP_REG}, 1);
	if (retVal != kStatus_Success)
	{
		/* Failed to send data to I2C slave */
		if (retVal == kStatus_LPI2C_Nak)
		{
			LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
		}
		goto i2c_error;
	}

	retVal = LPI2C_MasterRepeatedStart(BOARD_GENERAL_I2C_BASEADDR, PCT2075_I2C_ADDRESS, kLPI2C_Read);
	if (retVal != kStatus_Success)
	{
		/* Failed to start I2C master */
		if (retVal == kStatus_LPI2C_Nak)
		{
			LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
		}
		goto i2c_error;
	}

	retVal = LPI2C_MasterReceive(BOARD_GENERAL_I2C_BASEADDR, TemperatureData, 2);
	if (retVal != kStatus_Success)
	{
		/* Failed to receive data from I2C slave */
		if (retVal == kStatus_LPI2C_Nak)
		{
			LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
		}
		goto i2c_error;
	}

	retVal = LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
	if (retVal != kStatus_Success)
	{
		/* Failed to stop I2C master */
		goto i2c_error;
	}

	/* return mutex */
	xSemaphoreGive(gSmComlock);

	/* Compute 11-bit temperature value */
	Temperature_11_bit = ((int16_t) (TemperatureData[0] << 8 | TemperatureData[1])) >> 5;

	/* Compute temperature in °C */
	*temperature = Temperature_11_bit * 0.125;
	return kStatus_QMC_Ok;

i2c_error:
	/* return mutex */
	xSemaphoreGive(gSmComlock);
	return kStatus_QMC_Err;
}

qmc_status_t BOARD_SetDbTemperatureAlarm(float threshold, float hysteresis)
{
	status_t retVal = kStatus_Fail;
	uint8_t hysteresisData[3], thresholdData[3];
	int16_t temperature_9_bit;

	/* input sanitation and limit checks */
	if(threshold <= hysteresis)
		return kStatus_QMC_ErrArgInvalid;
	if(  (hysteresis > 127.0f)
	   ||(hysteresis < -55.0f)
	   ||(threshold  > 127.0f)
	   ||(threshold  < -55.0f) )
		return kStatus_QMC_ErrRange;

	/* convert to fixed point and generate commands */
	temperature_9_bit = hysteresis / 0.5f;
	hysteresisData[2]  = (temperature_9_bit << 7) & 0xff; /* LSB */
	hysteresisData[1]  = (temperature_9_bit >> 1) & 0xff; /* MSB */
	hysteresisData[0]  = THYST_REG;                       /* Register */
	temperature_9_bit = threshold / 0.5f;
	thresholdData[2]   = (temperature_9_bit << 7) & 0xff; /* LSB */
	thresholdData[1]   = (temperature_9_bit >> 1) & 0xff; /* MSB */
	thresholdData[0]   = TOS_REG;                         /* Register */

	/* take mutex */
	xSemaphoreTake(gSmComlock, portMAX_DELAY);

	/* set hysteresis value */
	retVal = LPI2C_MasterStart(BOARD_GENERAL_I2C_BASEADDR, PCT2075_I2C_ADDRESS, kLPI2C_Write);
	if (retVal != kStatus_Success)
	{
		/* Failed to start I2C master */
		goto i2c_error;
	}

	retVal = LPI2C_MasterSend(BOARD_GENERAL_I2C_BASEADDR, hysteresisData, 3);
	if (retVal != kStatus_Success)
	{
		/* Failed to send data to I2C slave */
		if (retVal == kStatus_LPI2C_Nak)
		{
			LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
		}
		goto i2c_error;
	}

	retVal = LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
	if (retVal != kStatus_Success)
	{
		/* Failed to stop I2C master */
		goto i2c_error;
	}

	/* set threshold value */
	retVal = LPI2C_MasterStart(BOARD_GENERAL_I2C_BASEADDR, PCT2075_I2C_ADDRESS, kLPI2C_Write);
	if (retVal != kStatus_Success)
	{
		/* Failed to start I2C master */
		goto i2c_error;
	}

	retVal = LPI2C_MasterSend(BOARD_GENERAL_I2C_BASEADDR, thresholdData, 3);
	if (retVal != kStatus_Success)
	{
		/* Failed to send data to I2C slave */
		if (retVal == kStatus_LPI2C_Nak)
		{
			LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
		}
		goto i2c_error;
	}

	retVal = LPI2C_MasterStop(BOARD_GENERAL_I2C_BASEADDR);
	if (retVal != kStatus_Success)
	{
		/* Failed to stop I2C master */
		goto i2c_error;
	}

	/* return mutex */
	xSemaphoreGive(gSmComlock);

	return kStatus_QMC_Ok;

i2c_error:
	/* return mutex */
	xSemaphoreGive(gSmComlock);
	return kStatus_QMC_Err;
}

/*!
 * @brief Get the temperature in degree Celsius from the selected temperature sensor on one of the Power Stage Boards.
 *
 * This function may return cached values read by the Board Service task instead of reading the sensor directly.
 *
 * @param temperature Pointer to write the temperature to
 * @param psb Sensor to read the temperature from
 */
qmc_status_t BOARD_GetPsbTemperature(float* temperature, qmc_psb_temperature_id_t psb)
{
	if (temperature == NULL)
	{
		return kStatus_QMC_Err;
	}

	switch (psb)
	{
		case kQMC_Psb1Sensor1:
			*temperature = g_PSBTemps[0];
			break;
		case kQMC_Psb1Sensor2:
			*temperature = g_PSBTemps[1];
			break;
		case kQMC_Psb2Sensor1:
			*temperature = g_PSBTemps[2];
			break;
		case kQMC_Psb2Sensor2:
			*temperature = g_PSBTemps[3];
			break;
		case kQMC_Psb3Sensor1:
			*temperature = g_PSBTemps[4];
			break;
		case kQMC_Psb3Sensor2:
			*temperature = g_PSBTemps[5];
			break;
		case kQMC_Psb4Sensor1:
			*temperature = g_PSBTemps[6];
			break;
		case kQMC_Psb4Sensor2:
			*temperature = g_PSBTemps[7];
			break;
		default:
			return kStatus_QMC_Err;
	}
	return kStatus_QMC_Ok;
}

qmc_status_t BOARD_SetDigitalOutput(qmc_output_cmd_t mode, qmc_output_id_t pin)
{
	uint16_t pins_disp_b2  = pin & (kQMC_UserOutput1 | kQMC_UserOutput2 | kQMC_UserOutput3 | kQMC_UserOutput4);
	uint16_t pins_snvs     = pin & (kQMC_UserOutput5 | kQMC_UserOutput6 | kQMC_UserOutput7 | kQMC_UserOutput8);
	uint8_t  snvsLastStateBackup;
	static uint8_t snvsLastState /*= kQMC_CM4_SnvsUserOutputsInitState*/=0; //TODO: fix initialization once RPC implementation is available

    qmc_status_t retval = kStatus_QMC_Ok;

	/* input sanitation and limit checks */
	if(pin != (pins_disp_b2 | pins_snvs))
		return kStatus_QMC_ErrArgInvalid;

	/* qmc_output_id_t needs a shift to be used as mask */
	pins_disp_b2 <<= 1;

	/* if SNVS pins are accessed, snvsLastState must be protected */
	if(pins_snvs)
		xSemaphoreTake(gs_SnvsAccessMutexHandle, portMAX_DELAY);

    snvsLastStateBackup = snvsLastState;
    switch(mode)
    {
    case kQMC_CmdOutputClear:
    	if(pins_disp_b2)
    		GPIO_PortClear(GPIO5, pins_disp_b2);
    	if(pins_snvs)
    	{
    		retval = RPC_SetSnvsOutput(pins_snvs);
    		snvsLastState &= ~(pins_snvs >> 4);
    	}
    	break;
    case kQMC_CmdOutputSet:
    	if(pins_disp_b2)
    		GPIO_PortSet(GPIO5, pins_disp_b2);
    	if(pins_snvs)
    	{
    		retval = RPC_SetSnvsOutput(pins_snvs | (pins_snvs >> 4));
    		snvsLastState |= (pins_snvs >> 4);
    	}
    	break;
    case kQMC_CmdOutputToggle:
    	if(pins_disp_b2)
    		GPIO_PortToggle(GPIO5, pins_disp_b2);
    	if(pins_snvs)
    	{
    		snvsLastState ^= (pins_snvs >> 4);
    		retval = RPC_SetSnvsOutput(snvsLastState | (pins_snvs >> 4));
    	}
    	break;
    default:
    	if(pins_snvs)
    		xSemaphoreGive(gs_SnvsAccessMutexHandle);
    	return kStatus_QMC_ErrArgInvalid;
    }

    if(pins_snvs && (kStatus_QMC_Ok != retval))
    	snvsLastState = snvsLastStateBackup;
    if(pins_snvs)
    	xSemaphoreGive(gs_SnvsAccessMutexHandle);
    return retval;
}

void BOARD_SetUserLed(qmc_led_cmd_t mode, qmc_led_id_t led)
{
    /* restrict led to valid input values */
	led &= (kQMC_Led1 | kQMC_Led2 | kQMC_Led3 | kQMC_Led4);

	/* Definition of qmc_led_id_t resembles the pin numbers and can directly be used as mask */
	switch(mode)
	{
	case kQMC_CmdLedOff:    GPIO_PortClear(GPIO5, led);  break;
	case kQMC_CmdLedOn:     GPIO_PortSet(GPIO5, led);    break;
	case kQMC_CmdLedToggle: GPIO_PortToggle(GPIO5, led); break;
	default:                break;
	}
}

qmc_status_t BOARD_GetTime(qmc_timestamp_t* timestamp)
{
    TickType_t ticks;

	/* input sanitation and limit checks */
	if(NULL == timestamp)
	    return kStatus_QMC_ErrArgInvalid;
	if(false == gs_isInitialized_getTime)
		return kStatus_QMC_Err;

	/* take mutex */
	xSemaphoreTake(gs_getTimeMutexHandle, portMAX_DELAY);

	/* get current ticks count */
	ticks = xTaskGetTickCount();

	/* refresh gs_getTimeOffset */
    if((((ticks-gs_getTimeLastRefresh)/configTICK_RATE_HZ) > BOARD_GETTIME_REFRESH_INTERVAL_S) || g_needsRefresh_getTime )
    {
		if(kStatus_QMC_Ok != RPC_GetTimeFromRTC(&gs_getTimeOffset))
		{
			xSemaphoreGive(gs_getTimeMutexHandle);
			return kStatus_QMC_Err;
		}
		gs_getTimeLastRefresh   = xTaskGetTickCount();
		ticks                   = gs_getTimeLastRefresh;
		g_needsRefresh_getTime  = false;
		xTimerReset(gs_getTime_OverflowProtectionTimerHandle, 0);
    }

    /* compute current time */
    *timestamp               = gs_getTimeOffset;
    timestamp->seconds      += (ticks-gs_getTimeLastRefresh) / configTICK_RATE_HZ;
    timestamp->milliseconds += ((ticks-gs_getTimeLastRefresh) % configTICK_RATE_HZ) * 1000 / configTICK_RATE_HZ;
    if(timestamp->milliseconds >= 1000) /* handle milliseconds overflow */
    {
    	timestamp->milliseconds -= 1000;
    	timestamp->seconds++;
    }

    /* return mutex */
    xSemaphoreGive(gs_getTimeMutexHandle);

    return kStatus_QMC_Ok;
}

/*!
* @brief Set the lifecycle of the QMC system
*
* The function shall set the system event bits so that only the specified lifecycle is set and all others are clear.
*
* @param lc the lifecycle to set
*/
qmc_status_t BOARD_SetLifecycle(qmc_lifecycle_t lc)
{
	uint32_t systemStatus = xEventGroupGetBits(g_systemStatusEventGroupHandle);
	qmc_lifecycle_t currentLc;
	int detectedLifecycleStates = 0;

	if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Operational)
	{
		currentLc = kQMC_LcOperational;
		detectedLifecycleStates++;
	}

	if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Maintenance)
	{
		currentLc = kQMC_LcMaintenance;
		detectedLifecycleStates++;
	}

	if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Commissioning)
	{
		currentLc = kQMC_LcCommissioning;
		detectedLifecycleStates++;
	}

	if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Decommissioning)
	{
		currentLc = kQMC_LcDecommissioning;
		detectedLifecycleStates++;
	}

	if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Error)
	{
		currentLc = kQMC_LcError;
		detectedLifecycleStates++;
	}

	if (detectedLifecycleStates != 1)
	{
		/* More than one lifecycle bit is set. */
		xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Operational | QMC_SYSEVENT_LIFECYCLE_Maintenance\
				| QMC_SYSEVENT_LIFECYCLE_Commissioning | QMC_SYSEVENT_LIFECYCLE_Decommissioning | QMC_SYSEVENT_LIFECYCLE_Error);
		xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Error);

		return kStatus_QMC_Err;
	}

	switch (lc)
	{
	case kQMC_LcOperational:
		if (currentLc == kQMC_LcMaintenance || currentLc == kQMC_LcError)
		{
			xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Maintenance | QMC_SYSEVENT_LIFECYCLE_Error);
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Operational);
			return kStatus_QMC_Ok;
		}
		break;

	case kQMC_LcMaintenance:
		if (currentLc == kQMC_LcOperational || currentLc == kQMC_LcError || currentLc == kQMC_LcCommissioning)
		{
			xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Operational || QMC_SYSEVENT_LIFECYCLE_Error\
					|| QMC_SYSEVENT_LIFECYCLE_Commissioning);
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Maintenance);
			return kStatus_QMC_Ok;
		}
		break;

	case kQMC_LcCommissioning:
		if (currentLc == kQMC_LcDecommissioning)
		{
			xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Decommissioning);
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Commissioning);
			return kStatus_QMC_Ok;
		}
		break;

	case kQMC_LcDecommissioning:
		if (currentLc == kQMC_LcMaintenance)
		{
			xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Maintenance);
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Decommissioning);
			return kStatus_QMC_Ok;
		}
		break;

	case kQMC_LcError:
		if (currentLc == kQMC_LcOperational)
		{
			xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Operational);
			xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Error);
			return kStatus_QMC_Ok;
		}
		break;

	default:
		break;
	}

	/* Either a wrong value was given as the lc argument or a forbidden transition was requested */
	xEventGroupClearBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Operational | QMC_SYSEVENT_LIFECYCLE_Maintenance\
			| QMC_SYSEVENT_LIFECYCLE_Commissioning | QMC_SYSEVENT_LIFECYCLE_Decommissioning | QMC_SYSEVENT_LIFECYCLE_Error);

	xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Error);

	return kStatus_QMC_Err;
}

qmc_lifecycle_t BOARD_GetLifecycle(void)
{
	uint32_t systemStatus = xEventGroupGetBits(g_systemStatusEventGroupHandle);

	if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Operational)
	{
		return kQMC_LcOperational;
	}
	else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Maintenance)
	{
		return kQMC_LcMaintenance;
	}
	else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Commissioning)
	{
		return kQMC_LcCommissioning;
	}
	else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Decommissioning)
	{
		return kQMC_LcDecommissioning;
	}
	else if (systemStatus & QMC_SYSEVENT_LIFECYCLE_Error)
	{
		return kQMC_LcError;
	}
	else
	{
		/* No lifecycle bit is set. */
		xEventGroupSetBits(g_systemStatusEventGroupHandle, QMC_SYSEVENT_LIFECYCLE_Error);
		return kQMC_LcError;
	}
}

qmc_status_t BOARD_ConvertTimestamp2Datetime(const qmc_timestamp_t* timestamp, qmc_datetime_t* dt)
{
	int      i;
	uint32_t days, seconds, units;
	bool     isLeap;

	/* input sanitation and limit checks */
	if((NULL == dt) || (NULL == timestamp) )
	    return kStatus_QMC_ErrArgInvalid;
	if(timestamp->milliseconds > 999 )
        return kStatus_QMC_ErrRange;

	/* split timestamp into full days and remaining seconds */
    days    = (timestamp->seconds / SECONDS_PER_DAY);
    seconds = timestamp->seconds - days*SECONDS_PER_DAY;
    days   += DAYS_BEFORE_EPOCH;

    /* split days into full years (observing leap years) and remaining days */
	units     = (days / DAYS_PER_400_YEARS);
    days     -= DAYS_PER_400_YEARS * units;
    dt->year  = units * 400;
    units     = (days / DAYS_PER_100_YEARS);
    days     -= DAYS_PER_100_YEARS * units;
    dt->year += units * 100;
    units     = (days / DAYS_PER_4_YEARS);
    days     -= DAYS_PER_4_YEARS * units;
    dt->year += units << 2 /* *4 */;
    units     = (days / DAYS_PER_YEAR);
    days     -= DAYS_PER_YEAR * units;
    dt->year += units;

    /* check whether the year is a leap year */
    isLeap = isLeapYear(dt->year);

    /* in leap years the code above subtracts one day too much */
    if(isLeap)
    	days++;  //TODO: why?

    /* for further processing, add the incomplete day to the remaining days */
	days++;

    /* split days into full months and remaining days */
    for(i = 1; i < (sizeof(gs_daysInYearPerMonth)/sizeof(uint16_t)); i++) {
    	if(days < ((isLeap && (i>1)) ? gs_daysInYearPerMonth[i]+1 : gs_daysInYearPerMonth[i]) )
    		break;
    }
    dt->month = i;
    dt->day   = days - gs_daysInYearPerMonth[dt->month - 1];
    if(isLeap && (dt->month > 2))
    	dt->day--;

    /* split seconds into full hours, minutes and remaining seconds */
    dt->hour   = seconds / SECONDS_PER_HOUR;
    seconds   -= dt->hour * SECONDS_PER_HOUR;
    dt->minute = seconds / SECONDS_PER_MINUTE;
    seconds   -= dt->minute * SECONDS_PER_MINUTE;
    dt->second = seconds;

    /* copy over the milliseconds and calculate the day of the week*/
    dt->millisecond = timestamp->milliseconds;
    dt->dayOfWeek   = computeDayOfWeek(dt);

    return kStatus_QMC_Ok;
}

qmc_status_t BOARD_ConvertDatetime2Timestamp(const qmc_datetime_t* dt, qmc_timestamp_t* timestamp)
{
	uint32_t daysSinceEpoch;
	bool     isLeap;

	/* input sanitation and limit checks */
	if((NULL == dt) || (NULL == timestamp) )
	    return kStatus_QMC_ErrArgInvalid;
	if( (dt->hour > 23) || (dt->minute > 59) || (dt->second > 59) || (dt->millisecond > 999) || (dt->dayOfWeek < 0) || (dt->dayOfWeek > 6) )
        return kStatus_QMC_ErrRange;
	isLeap = isLeapYear(dt->year);
    switch(dt->month)
    {
    case  2: if( (!isLeap && (dt->day > 28)) || (isLeap && (dt->day > 29)) )
    	         return kStatus_QMC_ErrRange;
    case  4:
    case  6:
    case  9:
    case 11: if(dt->day > 30 )
    	         return kStatus_QMC_ErrRange;
    case  1:
    case  3:
    case  5:
    case  7:
    case  8:
    case 10:
    case 12: if(dt->day > 31 )
                 return kStatus_QMC_ErrRange;
             break;
    default: return kStatus_QMC_ErrRange;
    }

    /* compute days since the epoch (01.01.1970 00:00:00.000) */
	daysSinceEpoch = (dt->year - 1970)*DAYS_PER_YEAR + LEAP_YEARS_SINCE_EPOCH(dt->year) + gs_daysInYearPerMonth[dt->month-1] + dt->day-1;
	if(isLeap && (dt->month < 3))
		daysSinceEpoch--;

	/* compute seconds since the epoch */
	timestamp->seconds = (daysSinceEpoch*SECONDS_PER_DAY)+ dt->hour*SECONDS_PER_HOUR + dt->minute * SECONDS_PER_MINUTE + dt->second;
	timestamp->milliseconds = dt->millisecond;

	return kStatus_QMC_Ok;
}

static bool isLeapYear(uint16_t year)
{
	if(0 != (year & 0x0003U) /*(year % 4)*/)
		return false;
	if(0 == year % 400)
		return true;
	if(0 == year % 100)
		return false;
	return true;
}

static qmc_weekday_t computeDayOfWeek(qmc_datetime_t *dt)
{
	uint16_t year = dt->year;

	/* Sakamoto's method */
	if(dt->month < 3)
		year--;
	return (year + LEAP_YEARS_SINCE_YEAR(year) + gs_modularDaysInYearPerMonth[dt->month-1] + dt->day) % 7;
}

static void getTime_OverflowProtectionTimerCallback(TimerHandle_t xTimer)
{
	g_needsRefresh_getTime = true;
}

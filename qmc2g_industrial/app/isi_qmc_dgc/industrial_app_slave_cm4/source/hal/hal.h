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
 * @file hal.h
 * @brief Hardware abstraction layer for the QMC 2G CM4 project.
 *
 * IMX RT1176 specific source code.
 */
#ifndef _HAL_H_
#define _HAL_H_

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "qmc_features_config.h"

#include "board.h"
#include "fsl_debug_console.h"

#include "api_qmc_common.h"
#include "api_rpc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEBUG_LOG_PRINT_FUNC PRINTF

/* debouncing time > (HAL_DEBOUNCING_TIMEOUT_RELOAD - 1) * HAL_SYSTICK_PERIOD_MS */
#define HAL_DEBOUNCING_TIMEOUT_RELOAD (2) /*!< reload value of debouncing timeout */

/* 2 ^ HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP = 1024Hz */
#define HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP (10U) /*!< SNVS RTC periodic interrupt frequency exponent */
/* (HAL_WDG_TIMEOUT_VALUE + 1) * 0.5s = 1s */
#define HAL_WDG_TIMEOUT_VALUE (1U) /*!< Hardware watchdog timeout value */
/* (HAL_WDG_INT_BEFORE_TIMEOUT_VALUE * 0.5s) = 0.5s */
#define HAL_WDG_INT_BEFORE_TIMEOUT_VALUE                \
    (1U) /*!< Hardware watchdog time before timeout for \
              triggering ISR */

#define HAL_SYSTICK_PERIOD_MS (10U) /*!< period of the SysTick exception */

/*!
 * @brief Pin values for the SNVS GPIO13 port
 *
 */
typedef enum
{
    kHAL_SnvsUserInput0 =
        BOARD_SLOW_DIG_IN4_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P7. */
    kHAL_SnvsUserInput1 =
        BOARD_SLOW_DIG_IN5_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P8. */
    kHAL_SnvsUserInput2 =
        BOARD_SLOW_DIG_IN6_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P9. */
    kHAL_SnvsUserInput3 =
        BOARD_SLOW_DIG_IN7_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P10. */
    kHAL_SnvsUserOutput0 =
        BOARD_SLOW_DIG_OUT_4_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P3. */
    kHAL_SnvsUserOutput1 =
        BOARD_SLOW_DIG_OUT_5_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P4. */
    kHAL_SnvsUserOutput2 =
        BOARD_SLOW_DIG_OUT_6_GPIO_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P5. */
    kHAL_SnvsUserOutput3 =
        BOARD_SLOW_DIG_OUT_7_GPIO_PIN,           /*!< According to latest pinmux configuration mapped to GPIO13 P6. */
    kHAL_SnvsSpiCs0 = BOARD_SPI_DEVICE_SEL0_PIN, /*!< According to latest pinmux configuration mapped to GPIO13 P11. */
    kHAL_SnvsSpiCs1 = BOARD_SPI_DEVICE_SEL1_PIN  /*!< According to latest pinmux configuration mapped to GPIO13 P12. */
} hal_snvs_gpio_pin_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

extern volatile rpc_shm_t g_rpcSHM;             /*!< shared memory for inter-core communication */
extern rpc_secwd_init_data_t g_awdgInitDataSHM; /*!< shared memory for AWDG init data */

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Performs the early board initialization.
 *
 * Configures the MPU and optionally the debug UART pins.
 *
 * Does not set up any peripherals, nor enables any interrupts.
 */
void HAL_InitBoard(void);

/*!
 * @brief Performs GPIO13 peripheral initialization.
 *
 * Sets up the GPIO13 peripheral and pin change interrupt interrupt (but does not enable it in the NVIC).
 *
 * The GPIO13 is configured as specified in the QMC2G documentation.
 *
 * @param initState Initial state for the outputs (for pin values see hal_snvs_gpio_pin_t).
 */
void HAL_SnvsGpioInit(uint32_t initState);

/*!
 * @brief Performs SysTick initialization.
 *
 * Sets up the SysTick interrupt (10ms period), used for debouncing.
 * As the SysTick is an exception (not handled by NVIC),
 * the handler will start to be immediately called after running this function.
 * 
 * @return qmc_status_t A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully, system tick timer is running.
 * @retval kStatus_QMC_ErrRange
 * The internally calculated tick reload value is invalid, system tick timer not running!
 */
qmc_status_t HAL_InitSysTick(void);

/*!
 * @brief Performs SRTC peripheral initialization.
 *
 * Initializes the SNVS LP domain (also needed for accessing the SNVS LPGPR registers!).
 * Sets up the SNVS SRTC peripheral (just starts it).
 * The value of the SRTC is only reset to zero if the SRTC was not running
 * already.
 */
void HAL_InitSrtc(void);

/*!
 * @brief Performs RTC peripheral initialization.
 *
 * Initializes the SNVS HP domain.
 * Sets up the RTC peripheral and periodic interrupt (but does not enable it in the NVIC).
 * The periodic interrupt frequency is set to 1024Hz (approximately 1ms).
 */
void HAL_InitRtc(void);

/*!
 * @brief Configure and enable all needed interrupts.
 *
 * We do not use interrupt nesting, but we still assign subpriorities to
 * specify which interrupt should be processed first when multiple ones are
 * pending (all time measurements refer to IMXRT1176 CM4):
 * 
 *  1. WDOG1_IRQn (only in case hardware watchdog is near expiration == code does not work)
 *      w.c. 359593 cycles 899us (n = 100000)
 *  2. SNVS_HP_NON_TZ_IRQ (ticks functional + authenticated watchdog, every 1ms)
 *      w.c. 71910 cycles 180us (n = 100000; 10 functional watchdogs)
 *  3. GPIO13_Combined_0_31_IRQ (triggered when an input state changes, call pattern unknown)
 *      w.c. 83891 cycles 210us (n = 100000)
 *  4. SysTick_IRQ (used for debouncing, every 10ms)
 *      w.c. 47935 cycles 120us (n = 100000)
 *  5. GPR_IRQ_IRQn (triggered in case of communication, call pattern unknown)
 *     - w.c. 2813 cycles 165us (n = 100000; without reset)
 *     - w.c. 395559 cycles 1211us (n = 100000; with reset)
 * 
 *     The communication implementation "polls" the CM7 until it acknowledges
 *     the receival. Therefore, we assign GPR_IRQ_IRQn the lowest priority so that no
 *     other interrupts are blocked. This should be fine as the other interrupts are more
 *     predictable and should not block the system.
 */
void HAL_ConfigureEnableInterrupts(void);

/*!
 * @brief Resets the SoC.
 */
void HAL_ResetSystem(void);

/*!
 * @brief Inits the hardware watchdog and enables the interrupt before timeout.
 *
 * The timeout is set to 1s. The interrupt happens 0.5s before the timeout.
 */
void HAL_InitHWWatchdog(void);

/*!
 * @brief Kick hardware watchdog.
 */
void HAL_KickHWWatchdog(void);

/*!
 * @brief Writes the specified value to GPIO13 outputs.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @param value New output pins state (for pin values see hal_snvs_gpio_pin_t).
 */
void HAL_SetSnvsGpio13Outputs(uint32_t value);

/*!
 * @brief Returns the value of the GPIO13 outputs.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @return uint32_t Encodes the GPIO13 outputs state (for pin values see hal_snvs_gpio_pin_t).
 */
uint32_t HAL_GetSnvsGpio13Outputs(void);

/*!
 * @brief Gets the debounced input state of GPIO13.
 *
 * @return uint32_t Debounced GPIO13 state (for pin values see hal_snvs_gpio_pin_t).
 */
uint32_t HAL_GetSnvsGpio13(void);

/*!
 * @brief Writes the WD timer backup to the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @param value New WD timer backup value.
 */
void HAL_SetWdTimerBackup(uint16_t value);

/*!
 * @brief Writes the watchdog status to the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @param status WD status: running (!=0) or stopped (0).
 */
void HAL_SetWdStatus(uint8_t status);

/*!
 * @brief Writes the FW update status to the battery-backed SNVS GPR.
 *
 * Hint: The type qmc_fw_update_state_t is not used as enums are always 32 bits wide.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @param status New firmware update status according to qmc_fw_update_state_t.
 */
void HAL_SetFwuStatus(uint8_t status);

/*!
 * @brief Writes the SRTC offset to the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @param offset The SRTC offset value.
 */
void HAL_SetSrtcOffset(int64_t offset);

/*!
 * @brief Writes the reset cause to the battery-backed SNVS GPR.
 *
 * Hint: The type qmc_reset_cause_id_t is not used as enums are always 32 bits wide.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @param cause The reset cause value according to qmc_reset_cause_id_t.
 */
void HAL_SetResetCause(uint8_t cause);

/*!
 * @brief Reads the timer backup value from the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @return uint16_t Timer backup value.
 */
uint16_t HAL_GetWdTimerBackup(void);

/*!
 * @brief Reads the WD status from the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @return uint8_t WD status: running (!=0) or stopped (0).
 */
uint8_t HAL_GetWdStatus(void);

/*!
 * @brief Reads the FW update status from the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @return uint8_t The FW update status according to qmc_fw_update_state_t.
 */
uint8_t HAL_GetFwuStatus(void);

/*!
 * @brief Reads the reset cause from the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @return uint8_t The reset cause according to qmc_reset_cause_id_t.
 */
uint8_t HAL_GetResetCause(void);

/*!
 * @brief Reads the SRTC offset from the battery-backed SNVS GPR.
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @return int64_t The SRTC offset as stored in SNVS.
 */
int64_t HAL_GetSrtcOffset(void);

/*!
 * @brief Gets the value of the SNVS SRTC real-time clock hardware counter.
 *
 * The reading is implemented according to the platform reference manual.
 * The corresponding API function offered by the SDKs SRTC driver
 * (SNVS_LP_SRTC_GetDatetime) was not used as it does not fulfill our time
 * granularity requirements (only seconds).
 *
 * Accesses the slow SNVSMIX domain.
 *
 * @startuml
 * start
 * :uint8_t tries = 0
 * uint64_t srtcValue1 = 0
 * uint64_t srtcValue2 = 0;
 * if () then (NULL == pRtcVal)
 *   :return kStatus_QMC_ErrArgInvalid;
 *   stop
 * endif
 * repeat
 *   :srtcValue1 = ((uint64_t)SNVS->LPSRTCMR << 32) | SNVS->LPSRTCLR
 *   srtcValue2 = ((uint64_t)SNVS->LPSRTCMR << 32) | SNVS->LPSRTCLR
 *   tries++;
 * repeat while ((tries < 3) && (srtcValue1 != srtcValue2))
 * if () then (srtcValue1 == srtcValue2)
 *   :~*pRtcVal = (int64_t)srtcValue1
 *   return kStatus_QMC_Ok;
 * else (else)
 *   :return kStatus_QMC_Timeout;
 * endif
 * stop
 * @enduml
 *
 * @param pRtcVal Pointer to a variable where the SNVS SRTC hardware counter value
 * is stored. The counter value is always positive and fits into int64_t as it has a bit width of 47bit.
 * @return qmc_status_t A qmc_status_t status code.
 * @retval kStatus_QMC_Ok
 * The operation did complete successfully, the result has been written to *pRtcVal.
 * @retval kStatus_QMC_ErrArgInvalid
 * The given pRtcVal pointer was invalid (NULL).
 * @retval kStatus_QMC_Timeout
 * The result was not stable after 3 tries, it has not been written. Try again.
 */
qmc_status_t HAL_GetSrtcCount(int64_t *pRtcVal);

/*!
 * @brief Disables the inter-core interrupt.
 */
void HAL_DisableInterCoreIRQ(void);

/*!
 * @brief Enables the inter-core interrupt.
 */
void HAL_EnableInterCoreIRQ(void);

/*!
 * @brief Executes a data memory barrier to ensure all previous memory accesses retire.
 */
void HAL_DataMemoryBarrier(void);

/*!
 * @brief Executes a data synchronization barrier.
 */
void HAL_DataSynchronizationBarrier(void);

/*!
 * @brief Notifies the CM7 about pending messages / events using the GPR SW interrupt.
 *
 * Before calling this function the inter-core IRQ has to be disabled!
 * 
 * Includes a barrier that ensures all memory accesses retire before the 
 * interrupted is triggered.
 *
 * This function is not reentrant and must not be executed in parallel.
 */
void HAL_TriggerInterCoreIRQWhileIRQDisabled(void);

/*!
 * @brief Basic critical section implementation. Do not call from an ISR!
 *
 * Just disables interrupts and keeps track of nested critical sections.
 */
void HAL_EnterCriticalSectionNonISR(void);

/*!
 * @brief Basic critical section implementation. Do not call from an ISR!
 *
 * Just disables interrupts and keeps track of nested critical sections.
 * Always enables interrupts globally, regardless what the previous
 * state was!
 */
void HAL_ExitCriticalSectionNonISR(void);

/*!
 * @brief Handler to call when the GPIO interrupt is triggered.
 *
 * This function should be called from the GPIO13 interrupt handler.
 * Part of input debouncing.
 */
void HAL_GpioInterruptHandler(void);

/*!
 * @brief Handler to call when a periodic interrupt is triggered.
 *
 * This function should be called from the SysTick interrupt handler.
 * Part of input debouncing.
 */
void HAL_GpioTimerHandler(void);

/*!
 * @brief Initializes the MCU temperature sensor.
 *
 * Must be called before calling HAL_GetMcuTemperature().
 */
void HAL_InitMcuTemperatureSensor(void);

/*!
 * @brief Gets the current MCU temperature.
 *
 * HAL_InitMcuTemperatureSensor() must be called before this function.
 */
float HAL_GetMcuTemperature(void);

#endif /* _HAL_H_ */

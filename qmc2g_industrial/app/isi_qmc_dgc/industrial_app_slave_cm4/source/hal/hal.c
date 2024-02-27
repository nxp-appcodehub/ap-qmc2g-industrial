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
 * @file hal.c
 * @brief Hardware abstraction layer for the QMC 2G CM4 project.
 *
 * IMX RT1176 specific source code.
 */
#include <stdatomic.h>

#include "hal.h"
#include "qmc_cm4_features_config.h"
#include "board.h"
#include "ksdk_mbedtls.h"
#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_snvs_hp.h"
#include "fsl_snvs_lp.h"
#include "fsl_wdog.h"
#include "fsl_tempsensor.h"

#include "utils/testing.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PRIGROUP_ONLY_SUB_PRI (7U) /*!< priority group which only allows subpriorities */

/*
 * The macros that end on INDEX select which SNVS GPR register is used to write the value to
 * The macros that end on MASK mask off the bits of interest, used in the read-modify-write operations
 * The macros that end on POS shift the value into the wanted position in the SNVS GPR register
 */
#define WDTIMER_SNVS_INDEX (0U)             /*!< which SVNS GPR index is used for the AWDG timer backup */
#define WDTIMER_SNVS_MASK  UINT32_C(0xFFFF) /*!< used bits mask for AWDG timer backup in SNVS GPR */
#define WDTIMER_SNVS_POS   (0U)             /*!< shift position for AWDG timer backup in SNVS GPR */

#define WDSTATUS_SNVS_INDEX (0U)               /*!< which SVNS GPR index is used for the AWDG status */
#define WDSTATUS_SNVS_MASK  UINT32_C(0xFF0000) /*!< used bits mask for AWDG status in SNVS GPR */
#define WDSTATUS_SNVS_POS   (16U)              /*!< shift position for AWDG status in SNVS GPR */

#define FWUSTATUS_SNVS_INDEX (0U)                 /*!< which SVNS GPR index is used for the firmware update status */
#define FWUSTATUS_SNVS_MASK  UINT32_C(0xFF000000) /*!< used bits mask for firmware update status in SNVS GPR */
#define FWUSTATUS_SNVS_POS   (24U)                /*!< shift position for firmware update status in SNVS GPR */

#define RTCLOW_SNVS_INDEX (1U)                 /*!< which SNVS GPR index is used for the low part of the RTC offset */
#define RTCLOW_SNVS_MASK  UINT32_C(0xFFFFFFFF) /*!< used bits mask for the low part of the RTC offset in SNVS GPR */
#define RTCLOW_SNVS_POS   (0U)                 /*!< shift position for the low part of the RTC offset in SNVS GPR */

#define RTCHIGH_SNVS_INDEX (2U)                 /*!< which SNVS GPR index is used for the high part of the RTC offset */
#define RTCHIGH_SNVS_MASK  UINT32_C(0xFFFFFFFF) /*!< used bits mask for the high part of the RTC offset in SNVS GPR */
#define RTCHIGH_SNVS_POS   (0U)                 /*!< shift position for the high part of the RTC offset in SNVS GPR */

#define RESETREASON_SNVS_INDEX (3U)           /*!< which SNVS GPR index is used for the resetCause */
#define RESETREASON_SNVS_MASK  UINT32_C(0xFF) /*!< used bits mask for resetCause in SNVS GPR */
#define RESETREASON_SNVS_POS   (0U)           /*!< shift position for resetCause in SNVS GPR */

#define SNVS_GET_SRTC_COUNT_RETRIES (3U) /*!< how often should we try getting a valid SRTC count */

#define GPIO13_OUTPUT_MASK                                                           \
    ((UINT32_C(1) << kHAL_SnvsUserOutput0) | (UINT32_C(1) << kHAL_SnvsUserOutput1) | \
     (UINT32_C(1) << kHAL_SnvsUserOutput2) | (UINT32_C(1) << kHAL_SnvsUserOutput3) | \
     (UINT32_C(1) << kHAL_SnvsSpiCs0) | (UINT32_C(1) << kHAL_SnvsSpiCs1)) /*!< GPIO13 bits used as outputs */
#define GPIO13_INPUT_MASK                                                          \
    ((UINT32_C(1) << kHAL_SnvsUserInput0) | (UINT32_C(1) << kHAL_SnvsUserInput1) | \
     (UINT32_C(1) << kHAL_SnvsUserInput2) | (UINT32_C(1) << kHAL_SnvsUserInput3)) /*!< GPIO13 bits used as inputs */

#define DEBOUNCING_PINS_PER_PORT   (16U) /*!< how many pins per port do we sense? */
#define DEBOUNCING_TIMEOUT_EXPIRED (1)   /*!< debouncing time expired value */
#define DEBOUNCING_TIMEOUT_IDLE    (0)   /*!< debouncing is idle */

/* see IMXRT1176 reference manual */
#if HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP > 15U
#error "HAL: Chosen SNVS RTC periodic interrupt frequency is not supported by hardware!"
#else
#define SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY (15U - HAL_SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY_EXP)
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* cross-core shared memory
 * address received from linker
 */
#if defined(__GNUC__)
volatile rpc_shm_t g_rpcSHM
    __attribute__((section(".noinit.$" RPC_SHM_SECTION_NAME))); /*!< shared memory section for RPC */
#else
#error "g_rpcSHM: Please provide your definition of g_rpcSHM!"
#endif

/* AWDG init data shared memory
 * address received from linker
 * if no SBL is available use static key and seed for testing and debugging
 */
#if defined(NO_SBL)
rpc_secwd_init_data_t g_awdgInitDataSHM = {
    .rngSeed    = QMC_CM4_TEST_AWDG_RNG_SEED,
    .rngSeedLen = QMC_CM4_TEST_AWDG_RNG_SEED_SIZE,
    .pk         = QMC_CM4_TEST_AWDG_PK,
    .pkLen      = QMC_CM4_TEST_AWDG_PK_SIZE}; /*!< shared memory section for AWDG init data*/
#else
#if defined(__GNUC__)
rpc_secwd_init_data_t g_awdgInitDataSHM __attribute__((
    section(".noinit.$" RPC_SECWD_INIT_DATA_SECTION_NAME))); /*!< shared memory section for AWDG init data */
#else
#error "g_awdgInitDataSHM: Please provide your definition of g_awdgInitDataSHM!"
#endif
#endif

static uint32_t gs_criticalSectionNestingLevel = 0U; /*!< nesting level for critical section */
/* _Atomic makes sure data is properly aligned for atomic access */
static volatile _Atomic int8_t gs_gpioChanges[DEBOUNCING_PINS_PER_PORT] = {0U}; /*!< debouncing timer per pin */
STATIC_TEST_VISIBLE volatile _Atomic uint32_t gs_gpioState = 0U;   /*!< GPIO input shadow register */

/*******************************************************************************
 * Code
 ******************************************************************************/

void HAL_GpioInterruptHandler(void)
{
    /* store which pins caused the interrupts */
    uint32_t isrRegister = GPIO13->ISR & GPIO13_INPUT_MASK;
    uint32_t mask        = 0x1U;

    for (size_t i = 0U; i < DEBOUNCING_PINS_PER_PORT; i++)
    {
        if ((mask & isrRegister) != 0U)
        {
            if (gs_gpioChanges[i] == DEBOUNCING_TIMEOUT_IDLE)
            {
                gs_gpioChanges[i] = HAL_DEBOUNCING_TIMEOUT_RELOAD;
            }
        }
        /* inspect next bit */
        mask = mask << 1U;
    }

    /* clear the bits that caused the interrupt by writing a one to them */
    GPIO13->ISR = isrRegister;
}

void HAL_GpioTimerHandler(void)
{
    uint32_t mask      = 0x1U;
    bool validShadowDr = false;
    uint32_t shadowDr;

    for (size_t i = 0U; i < DEBOUNCING_PINS_PER_PORT; i++)
    {
        if (gs_gpioChanges[i] > DEBOUNCING_TIMEOUT_EXPIRED)
        {
            gs_gpioChanges[i]--;
        }
        else if (gs_gpioChanges[i] == DEBOUNCING_TIMEOUT_EXPIRED)
        {
            gs_gpioChanges[i] = DEBOUNCING_TIMEOUT_IDLE;

            /* optimization to reduce register reads to a minimum
             * (access is slow) */
            if (validShadowDr == false)
            {
                shadowDr      = GPIO13->DR & GPIO13_INPUT_MASK;
                validShadowDr = true;
            }

            /* bitwise operations to modify gpio state without disturbing the rest
             * these sequence contains two atomic operations (could be interrupted),
             * but this is OK as it is executed in an interrupt and we have no nesting */
            gs_gpioState = (gs_gpioState & ~mask) | (shadowDr & mask);
        }
        else
        {
            /* no action required, "else" added for compliance */
            ;
        }
        mask = mask << 1U;
    }
}

void HAL_InitBoard(void)
{
    BOARD_ConfigMPU();
    /* clocks are configured in SBL (M7), just get core clock frequency */
    SystemCoreClock = CLOCK_GetRootClockFreq(kCLOCK_Root_M4);
#if !defined(BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL) && defined(ENABLE_M4_DEBUG_CONSOLE)
    /* NOTE only if debug console is wanted on CM4, remove if not needed anymore */
    BOARD_InitDebugConsole();
#endif
}

void HAL_SnvsGpioInit(uint32_t initState)
{
    /* SNVS GPIO output configuration */
    const gpio_pin_config_t kRdcOutputConfig = {
        kGPIO_DigitalOutput,
        0U,
        kGPIO_NoIntmode,
    };
    /* SNVS GPIO input configuration */
    const gpio_pin_config_t kRdcInputConfig = {
        kGPIO_DigitalInput,
        0U,
        kGPIO_IntRisingOrFallingEdge,
    };

    /* setup physical pin connections with GPIO13 */
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_00_DIG_GPIO13_IO03, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_01_DIG_GPIO13_IO04, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_02_DIG_GPIO13_IO05, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_03_DIG_GPIO13_IO06, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_04_DIG_GPIO13_IO07, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_05_DIG_GPIO13_IO08, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_06_DIG_GPIO13_IO09, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_07_DIG_GPIO13_IO10, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_08_DIG_GPIO13_IO11, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SNVS_09_DIG_GPIO13_IO12, 0U);

    /* setup pins as defined by api_qmc */
    GPIO_PinInit(GPIO13, kHAL_SnvsUserInput0, &kRdcInputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsUserInput1, &kRdcInputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsUserInput2, &kRdcInputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsUserInput3, &kRdcInputConfig);

    GPIO_PinInit(GPIO13, kHAL_SnvsUserOutput0, &kRdcOutputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsUserOutput1, &kRdcOutputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsUserOutput2, &kRdcOutputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsUserOutput3, &kRdcOutputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsSpiCs0, &kRdcOutputConfig);
    GPIO_PinInit(GPIO13, kHAL_SnvsSpiCs1, &kRdcOutputConfig);

    /* write initial output state */
    GPIO13->DR = initState & GPIO13_OUTPUT_MASK;

    /* get inital input state */
    gs_gpioState = GPIO13_INPUT_MASK & GPIO13->DR;

    /* clear pending interrupts and enable them */
    GPIO13->ISR = 0xFFFFFFFFU;
    /* does not enable interrupt in NVIC */
    GPIO_PortEnableInterrupts(GPIO13, GPIO13_INPUT_MASK);
}

qmc_status_t HAL_InitSysTick(void)
{
    uint32_t ticks = 0U;

    /* copied and modified from ARM CMSIS code
     * as we want to change the interrupt priority ourselves */

    /* check for overflow */
    assert(HAL_SYSTICK_PERIOD_MS <= UINT32_MAX / SystemCoreClock);
    if (SystemCoreClock > (UINT32_MAX / HAL_SYSTICK_PERIOD_MS))
    {
        return kStatus_QMC_ErrRange;
    }
    ticks = SystemCoreClock * HAL_SYSTICK_PERIOD_MS / 1000U;
    

    /* unsupported */
    assert((ticks > 0U) && ((ticks - 1U) <= SysTick_LOAD_RELOAD_Msk));
    /* ticks value of 0 is not allowed */
    if (0U == ticks)
    {
        return kStatus_QMC_ErrRange;
    }
    else if ((ticks - 1U) > SysTick_LOAD_RELOAD_Msk)
    {
       return kStatus_QMC_ErrRange;
    }
    /* set reload register */
    SysTick->LOAD = (uint32_t)(ticks - 1U);
    /* load the SysTick counter value */
    SysTick->VAL = 0U;
    /* enable SysTick exception and SysTick timer */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    return kStatus_QMC_Ok;
}

void HAL_InitSrtc(void)
{
    /* SNVS LP SRTC configuration */
    const snvs_lp_srtc_config_t kSnvsLpConfig = {.srtcCalEnable = false, .srtcCalValue = 0U};

    /* setup low-power SNVS */
    SNVS_LP_Init(SNVS);
    SNVS_LP_SRTC_Init(SNVS, &kSnvsLpConfig);

    /* SRTC not running yet? */
    if (SNVS_LPCR_SRTC_ENV(1U) != (SNVS->LPCR & SNVS_LPCR_SRTC_ENV(1U)))
    {
        /* reset SRTC counts */
        SNVS->LPSRTCMR = 0x0U;
        SNVS->LPSRTCLR = 0x0U;
        /* start the SRTC */
        SNVS_LP_SRTC_StartTimer(SNVS);
    }
    /* lock write access to SRTC, can not replace isolation from CM7! */
    SNVS->LPLR |= SNVS_LPLR_SRTC_HL(1U);
    SNVS->LPLR |= SNVS_LPLR_LPCALB_HL(1U);
}

void HAL_InitRtc(void)
{
    /* SNVS HP RTC configuration */
    const snvs_hp_rtc_config_t kSnvsHpConfig = {
        .rtcCalEnable = false, .rtcCalValue = 0U, .periodicInterruptFreq = SNVS_RTC_PERIODIC_INTERRUPT_FREQUENCY};

    /* setup high-power SNVS */
    SNVS_HP_Init(SNVS);
    SNVS_HP_RTC_Init(SNVS, &kSnvsHpConfig);
    /* enable periodic interrupt and start RTC */
    SNVS_HP_RTC_EnableInterrupts(SNVS, kSNVS_RTC_PeriodicInterrupt);
    SNVS_HP_RTC_StartTimer(SNVS);
}

void HAL_ConfigureEnableInterrupts(void)
{
    uint32_t priority = 0U;

    /* configure interrupts
     * www.ocfreaks.com/interrupt-priority-grouping-arm-cortex-m-nvic/
     * we do not use preemption (no nested interrupts) but still want to choose
     * which interrupt is executed first if multiple interrupts are pending:
     *  1) WDOG1_IRQn (only in case hardware watchdog is near expiration)
     *  2) SNVS_HP_NON_TZ_IRQ (ticks functional + authenticated watchdog, every 1ms)
     *  3) GPIO13_Combined_0_31_IRQ (triggered when an input state changes, call pattern unknown)
     *  4) SysTick_IRQ (used for debouncing, every 10ms)
     *  5) GPR_IRQ_IRQn (triggered in case of communication, call pattern unknown)
     *     The communication implementation "polls" the CM7 until it acknowledges
     *     the receival. Therefore, we assign GPR_IRQ_IRQn the lowest priority so that no
     *     other interrupts are blocked. This should be fine as the other interrupts are more
     *     predictable and should not block the system.
     */

    /* only subpriorities -> no ISR nesting */
    NVIC_SetPriorityGrouping(PRIGROUP_ONLY_SUB_PRI);

    /* highest priority if multiple interrupts are pending */
    priority = NVIC_EncodePriority(PRIGROUP_ONLY_SUB_PRI, 0U, 0U);
    NVIC_SetPriority(WDOG1_IRQn, priority);

    priority = NVIC_EncodePriority(PRIGROUP_ONLY_SUB_PRI, 0U, 1U);
    NVIC_SetPriority(SNVS_HP_NON_TZ_IRQn, priority);

    priority = NVIC_EncodePriority(PRIGROUP_ONLY_SUB_PRI, 0U, 2U);
    NVIC_SetPriority(GPIO13_Combined_0_31_IRQn, priority);

    /* SysTick is an exception, not handled by NVIC!
     * priority can still be set with the NVIC functions */
    priority = NVIC_EncodePriority(PRIGROUP_ONLY_SUB_PRI, 0U, 3U);
    NVIC_SetPriority(SysTick_IRQn, priority);

    /* lowest priority if multiple interrupts are pending */
    priority = NVIC_EncodePriority(PRIGROUP_ONLY_SUB_PRI, 0U, 4U);
    NVIC_SetPriority(GPR_IRQ_IRQn, priority);

    /* enable interrupts */
    NVIC_EnableIRQ(WDOG1_IRQn);
    NVIC_EnableIRQ(SNVS_HP_NON_TZ_IRQn);
    NVIC_EnableIRQ(GPIO13_Combined_0_31_IRQn);
    /* SysTick is an exception, not handled by NVIC, always on
     * NVIC_EnableIRQ(SysTick_IRQn); */
    NVIC_EnableIRQ(GPR_IRQ_IRQn);
}

void HAL_ResetSystem(void)
{
    NVIC_SystemReset();
}

void HAL_DisableInterCoreIRQ(void)
{
    NVIC_DisableIRQ(GPR_IRQ_IRQn);
    /* barrier for interrupt disabling
     * avoids that a pending IRQ handler is executed after the disabling instruction; to avoid side effects
     * documentation-service.arm.com/static/5efefb97dbdee951c1cd5aaf?token= */
    __DSB();
    __ISB();
}

void HAL_EnableInterCoreIRQ(void)
{
    NVIC_EnableIRQ(GPR_IRQ_IRQn);
    /* barrier for interrupt enabling
     * avoids that following instructions are executed before calling the ISR; to avoid side effects
     * documentation-service.arm.com/static/5efefb97dbdee951c1cd5aaf?token= */
    __DSB();
    __ISB();
}

void HAL_DataMemoryBarrier(void)
{
    __DMB();
}

void HAL_DataSynchronizationBarrier(void)
{
    __DSB();
}

void HAL_TriggerInterCoreIRQWhileIRQDisabled(void)
{
    /* ensures memory accesses are retired and visible by the other core before the ISR is triggered */
    __DSB();
    IOMUXC_GPR->GPR7 &= ~IOMUXC_GPR_GPR7_GINT(1U);
    IOMUXC_GPR->GPR7 |= IOMUXC_GPR_GPR7_GINT(1U);
    IOMUXC_GPR->GPR7 &= ~IOMUXC_GPR_GPR7_GINT(1U);
    /* do NOT clear pending IRQ here as M7 might have send another message!
     * NVIC_ClearPendingIRQ(GPR_IRQ_IRQn)*/
}

void HAL_EnterCriticalSectionNonISR(void)
{
    /* disable interrupts */
    __disable_irq();
    /* nesting level should never reach such high values */
    assert(gs_criticalSectionNestingLevel < UINT32_MAX);
    /* increase nesting level counter */
    gs_criticalSectionNestingLevel++;
}

void HAL_ExitCriticalSectionNonISR(void)
{
    /* must be greater one else we never entered a critical section before
     * (programming error) */
    assert(gs_criticalSectionNestingLevel > 0U);
    if (0U == gs_criticalSectionNestingLevel)
    {
        return;
    }

    /* decrease nesting level counter, reenable interrupts if it is zero */
    gs_criticalSectionNestingLevel--;
    if (0U == gs_criticalSectionNestingLevel)
    {
        __enable_irq();
        /* barrier for interrupt enabling
         * avoids that following instructions are executed before calling the ISR to avoid side effects
         * documentation-service.arm.com/static/5efefb97dbdee951c1cd5aaf?token= */
        __ISB();
    }
}

void HAL_SetWdTimerBackup(uint16_t value)
{
    uint32_t snvsStorage            = SNVS->LPGPR[WDTIMER_SNVS_INDEX];
    snvsStorage                     = snvsStorage & ~WDTIMER_SNVS_MASK;
    snvsStorage                     = snvsStorage | ((uint32_t)value << WDTIMER_SNVS_POS);
    SNVS->LPGPR[WDTIMER_SNVS_INDEX] = snvsStorage;
}

uint16_t HAL_GetWdTimerBackup(void)
{
    uint32_t snvsStorage = SNVS->LPGPR[WDTIMER_SNVS_INDEX];
    return (uint16_t)((snvsStorage & WDTIMER_SNVS_MASK) >> WDTIMER_SNVS_POS);
}

void HAL_SetWdStatus(uint8_t status)
{
    uint32_t snvsStorage             = SNVS->LPGPR[WDSTATUS_SNVS_INDEX];
    snvsStorage                      = snvsStorage & ~WDSTATUS_SNVS_MASK;
    snvsStorage                      = snvsStorage | ((uint32_t)status << WDSTATUS_SNVS_POS);
    SNVS->LPGPR[WDSTATUS_SNVS_INDEX] = snvsStorage;
}

uint8_t HAL_GetWdStatus(void)
{
    uint32_t snvsStorage = SNVS->LPGPR[WDSTATUS_SNVS_INDEX];
    return (uint8_t)((snvsStorage & WDSTATUS_SNVS_MASK) >> WDSTATUS_SNVS_POS);
}

void HAL_SetFwuStatus(uint8_t status)
{
    uint32_t snvsStorage              = SNVS->LPGPR[FWUSTATUS_SNVS_INDEX];
    snvsStorage                       = snvsStorage & ~FWUSTATUS_SNVS_MASK;
    snvsStorage                       = snvsStorage | ((uint32_t)status << FWUSTATUS_SNVS_POS);
    SNVS->LPGPR[FWUSTATUS_SNVS_INDEX] = snvsStorage;
}

uint8_t HAL_GetFwuStatus(void)
{
    uint32_t snvsStorage = SNVS->LPGPR[FWUSTATUS_SNVS_INDEX];
    return (uint8_t)((snvsStorage & FWUSTATUS_SNVS_MASK) >> FWUSTATUS_SNVS_POS);
}

void HAL_SetSrtcOffset(int64_t offset)
{
    SNVS->LPGPR[RTCLOW_SNVS_INDEX]  = (uint32_t)(((uint64_t)offset) & 0xFFFFFFFFU);
    SNVS->LPGPR[RTCHIGH_SNVS_INDEX] = (uint32_t)(((uint64_t)offset) >> 32U);
}

int64_t HAL_GetSrtcOffset(void)
{
    return (int64_t)((uint64_t)SNVS->LPGPR[RTCLOW_SNVS_INDEX] | (((uint64_t)SNVS->LPGPR[RTCHIGH_SNVS_INDEX]) << 32U));
}

void HAL_SetResetCause(uint8_t cause)
{
    uint32_t snvsStorage                = SNVS->LPGPR[RESETREASON_SNVS_INDEX];
    snvsStorage                         = snvsStorage & ~RESETREASON_SNVS_MASK;
    snvsStorage                         = snvsStorage | ((uint32_t)cause << RESETREASON_SNVS_POS);
    SNVS->LPGPR[RESETREASON_SNVS_INDEX] = snvsStorage;
}

uint8_t HAL_GetResetCause(void)
{
    uint32_t snvsStorage = SNVS->LPGPR[RESETREASON_SNVS_INDEX];
    return (uint8_t)((snvsStorage & RESETREASON_SNVS_MASK) >> RESETREASON_SNVS_POS);
}

void HAL_SetSnvsGpio13Outputs(uint32_t value)
{
    GPIO13->DR = value & GPIO13_OUTPUT_MASK;
}

uint32_t HAL_GetSnvsGpio13Outputs(void)
{
    return GPIO13->DR & GPIO13_OUTPUT_MASK;
}

uint32_t HAL_GetSnvsGpio13(void)
{
    return gs_gpioState & GPIO13_INPUT_MASK;
}

qmc_status_t HAL_GetSrtcCount(int64_t *pRtcVal)
{
    uint8_t tries      = 0U;
    uint64_t srtcValue1 = 0;
    uint64_t srtcValue2 = 0;

    if (NULL == pRtcVal)
    {
        return kStatus_QMC_ErrArgInvalid;
    }

    do
    {
        /* read two consecutive times to make sure we have a stable value 
         * see reference manual */
        srtcValue1 = ((uint64_t)SNVS->LPSRTCMR << 32U) | SNVS->LPSRTCLR;
        srtcValue2 = ((uint64_t)SNVS->LPSRTCMR << 32U) | SNVS->LPSRTCLR;
        tries++;
    } while ((tries < SNVS_GET_SRTC_COUNT_RETRIES) && (srtcValue1 != srtcValue2));

    if (srtcValue1 == srtcValue2)
    {
        /* 17 most-significant bits are always zero, so converting to signed is OK */
        *pRtcVal = (int64_t)srtcValue1;
        return kStatus_QMC_Ok;
    }
    else
    {
        return kStatus_QMC_Timeout;
    }
}

void HAL_InitHWWatchdog(void)
{
    /* WDOG1 configuration */
    const wdog_config_t kWdog1Config = {
        .enableWdog             = true,
        .workMode.enableWait    = false,
        .workMode.enableStop    = false,
        .workMode.enableDebug   = true, /* pause watchdog when debugging */
        .enableInterrupt        = true,
        .timeoutValue           = HAL_WDG_TIMEOUT_VALUE,
        .interruptTimeValue     = HAL_WDG_INT_BEFORE_TIMEOUT_VALUE,
        .softwareResetExtension = false,
        .enablePowerDown        = false,
        .enableTimeOutAssert    = false,
    };

    WDOG_Init(WDOG1, &kWdog1Config);
    /* we switch the interrupt priority level later in HAL_ConfigureEnableInterrupts() */
}

void HAL_KickHWWatchdog(void)
{
    WDOG_Refresh(WDOG1);
}

void HAL_InitMcuTemperatureSensor(void)
{
    tmpsns_config_t config = {0};
	TMPSNS_GetDefaultConfig(&config);
	TMPSNS_Init(TMPSNS, &config);
    /* disable interrupts */
    TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_LowTempInterruptStatusEnable);
	TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_HighTempInterruptStatusEnable);
	TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_PanicTempInterruptStatusEnable);
    TMPSNS_DisableInterrupt(TMPSNS, kTEMPSENSOR_FinishInterruptStatusEnable);
}

float HAL_GetMcuTemperature(void)
{
	float MCUTemp = 0;
	TMPSNS_StartMeasure(TMPSNS);
	MCUTemp = TMPSNS_GetCurrentTemperature(TMPSNS);
	TMPSNS_StopMeasure(TMPSNS);
    return MCUTemp;
}

/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2022-2023 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 /**************************************************************************//*!
 * @file      interrupts.c
 * @author    
 * @version   0.0.1.0
 * @brief     
 ******************************************************************************/
 
// Standard C Included Files
#include <stdint.h>
#include <stdbool.h>
#include "main_cm7.h"
#include "api_qmc_common.h"
#include "api_rpc_internal.h"
#include "api_mem_fault_info.h"
#include "qmc_features_config.h"
#include "task.h"

/* module definitions                                                         */

/* module function prototypes                                                 */

/* module variables                                                           */
extern TaskHandle_t g_datalogger_task_handle;
   
/* module Function definitions                                                */

/* module IRQ Handlers                                                        */
   
/***************************************************************************//*!
 * @brief   NMI exception handler
 * @details It will stop application in debug mode with software breakpoint
 *          when hard fault event occur.
 ******************************************************************************/
void NMI_Handler(void)
{
#ifdef DEBUG
  __asm("BKPT #0x01");
#endif
  NVIC_SystemReset();
}

/***************************************************************************//*!
 * @brief   Hardfault exception handler
 * @details It will stop application in debug mode with software breakpoint
 *          when hard fault event occur.
 ******************************************************************************/
#ifdef __SEMIHOST_HARDFAULT_DISABLE

void HardFault_Handler(void)
{
#ifdef DEBUG
  __asm("BKPT #0x02");
#endif
  NVIC_SystemReset();
}
#endif

/***************************************************************************//*!
 * @brief   Extended MemManage exception handler
 * @details Forwards write requests to the CCM or ANADIG module to the M4.
 * 			Otherwise, it will stop application in debug mode with software 
 *          breakpoint when hard fault event occur.
 ******************************************************************************/
#if !defined(NO_SBL) || defined(DEBUG_M4_WRITE_FORWARDING)
/* no optimization */
__attribute__((optimize("O0")))
void MemManage_HandlerFaultStackFrame(mem_fault_info_stack_frame_t *const pFaultStackFrame)
{
	/* check that fault address register is valid, a data access caused the fault
	 * and that the fault address lied within CCM or ANADIG range */
	if((SCB->CFSR & SCB_CFSR_MMARVALID_Msk) &&
	   (SCB->CFSR & SCB_CFSR_DACCVIOL_Msk) &&
	      /* CCM */
	   (((SCB->MMFAR >= 0x40CC0000U) && (SCB->MMFAR <= 0x40CC7FFFU)) ||
          /* ANADIG */
	    ((SCB->MMFAR >= 0x40C84000U) && (SCB->MMFAR <= 0x40C87FFFU))))
	{
		mem_fault_info_t memFaultInfo = {0U};

		/* get information about memory access fault */
		if(!MEM_FAULT_INFO_GetWriteAccessFaultInformation(pFaultStackFrame, (uintptr_t)SCB->MMFAR, &memFaultInfo))
		{
			/* unsupported write access -> halt or reboot */
			#ifdef DEBUG
			  __asm("BKPT #0x03");
			#endif
			  NVIC_SystemReset();
		}

		/* forward request to M4 which will either succeed or the system will be reset */
		qmc_mem_write_t write = {
			.baseAddress = memFaultInfo.address,
			.dataWords =  (memFaultInfo.dataWords <= QMC_MEM_WRITE_MAX_DATA_WORDS) ?
				  		   memFaultInfo.dataWords : QMC_MEM_WRITE_MAX_DATA_WORDS,
			.accessSize = memFaultInfo.accessSize
		};
		memcpy(write.data, memFaultInfo.data, ((uint32_t)write.dataWords) * sizeof(uint32_t));
		qmc_status_t ret = RPC_MemoryWriteIntsDisabled(&write);
		(void) ret;
		/* can not fail if pointer is valid */
		assert(kStatus_QMC_Ok == ret);

		/* we performed the access using the M4, skip the instruction that caused the fault */
		pFaultStackFrame->pc += memFaultInfo.insWidth;
		/* simulate writeback if necessary */
		if(NULL != memFaultInfo.pWritebackGpr)
		{
			*memFaultInfo.pWritebackGpr = memFaultInfo.writebackValue;
		}

		/* clear active MMFSR faults */
		SCB->CFSR |= SCB->CFSR & (SCB_CFSR_MMARVALID_Msk | SCB_CFSR_MLSPERR_Msk | SCB_CFSR_MSTKERR_Msk |
								  SCB_CFSR_MUNSTKERR_Msk | SCB_CFSR_DACCVIOL_Msk | SCB_CFSR_IACCVIOL_Msk);
    }
 	else
	{
		#ifdef DEBUG
		  __asm("BKPT #0x03");
		#endif
		  NVIC_SystemReset();
	}
}
#endif

/***************************************************************************//*!
 * @brief   MemManage exception handler
 * @details Extends the fault stack frame and calls 
 *          MemManage_HandlerFaultStackFrame if the SBL is used.
 ******************************************************************************/
#if defined(NO_SBL) && !defined(DEBUG_M4_WRITE_FORWARDING)
void MemManage_Handler(void)
{
	#ifdef DEBUG
		__asm("BKPT #0x03");
	#endif
		NVIC_SystemReset();
}
#else
/* naked as we use our own custom prologue and epilogue 
 * should have highest priority (0) */
__attribute__((naked))
void MemManage_Handler(void)
{
	/* push remaining GPRs and LR which were not pushed by hardware on stack with fault stack frame,
	 * move active stack pointer into argument 0 register and call extended handler,
	 * return after execution of extended handler */
	__asm volatile
	(
		/* if bit 2 in lr is set, then psp was active prior to exception */
		"tst lr, #4 \n"
		"beq 1f \n"
		/* psp was active, now msp is active */
		"mrs r0, psp \n"
		"stmdb r0!, {r4, r5, r6, r7, r8, r9, r10, r11, lr} \n"
		/* r4 is callee saved */
		"mov r4, r0 \n"
		"bl MemManage_HandlerFaultStackFrame \n"
		"ldm r4, {r4, r5, r6, r7, r8, r9, r10, r11, pc} \n"
		/* msp was active and is still active */
		"1: \n"
		"push {r4, r5, r6, r7, r8, r9, r10, r11, lr} \n"
		"mov r0, sp \n"
		"bl MemManage_HandlerFaultStackFrame \n"
		"pop {r4, r5, r6, r7, r8, r9, r10, r11, pc} \n"
	);
}
#endif

/***************************************************************************//*!
 * @brief   BusFault exception handler
 * @details It will stop application in debug mode with software breakpoint
 *          when hard fault event occur.
 ******************************************************************************/
void BusFault_Handler(void)
{
#ifdef DEBUG
  __asm("BKPT #0x03");
#endif
  NVIC_SystemReset();
}

/***************************************************************************//*!
 * @brief   UsageFault exception handler
 * @details It will stop application in debug mode with software breakpoint
 *          when usage fault event occur.
 ******************************************************************************/
void UsageFault_Handler(void)
{
#ifdef DEBUG
  __asm("BKPT #0x03");
#endif
  NVIC_SystemReset();
}

/***************************************************************************//*!
 * @brief   User button interrupt handler
 * @details .
 ******************************************************************************/
void BOARD_USER_BUTTON1_IRQ_HANDLER(void)
{
	EventBits_t clearBits=0, setBits=0;
	TickType_t currentTicks = xTaskGetTickCount();
	static TickType_t buttonPressedTime[4] = { 0, 0, 0, 0 };

	/* Button 1 state */
    if(GPIO_GetPinsInterruptFlags(BOARD_USER_BUTTON1_GPIO)&( 1U << BOARD_USER_BUTTON1_GPIO_PIN))
    {
    	if(GPIO_PinRead(BOARD_USER_BUTTON1_GPIO, BOARD_USER_BUTTON1_GPIO_PIN))
    	{
    		setBits   |= QMC_IOEVENT_BTN1_RELEASED;
    		clearBits |= QMC_IOEVENT_BTN1_PRESSED;
    		buttonPressedTime[0] = currentTicks;
    	} else {
    		if ((currentTicks - buttonPressedTime[0]) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
    		{
    			setBits   |= QMC_IOEVENT_BTN1_PRESSED;
    			clearBits |= QMC_IOEVENT_BTN1_RELEASED;
    		}
    	}
    	GPIO_PortClearInterruptFlags(BOARD_USER_BUTTON1_GPIO, 1U << BOARD_USER_BUTTON1_GPIO_PIN);
    }

    /* Button 2 state */
    if(GPIO_GetPinsInterruptFlags(BOARD_USER_BUTTON2_GPIO)&( 1U << BOARD_USER_BUTTON2_GPIO_PIN))
    {
    	if(GPIO_PinRead(BOARD_USER_BUTTON2_GPIO, BOARD_USER_BUTTON2_GPIO_PIN))
    	{
    		setBits   |= QMC_IOEVENT_BTN2_RELEASED;
    		clearBits |= QMC_IOEVENT_BTN2_PRESSED;
    		buttonPressedTime[1] = currentTicks;
    	} else {
    		if ((currentTicks - buttonPressedTime[1]) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
    		{
    			setBits   |= QMC_IOEVENT_BTN2_PRESSED;
    			clearBits |= QMC_IOEVENT_BTN2_RELEASED;
    		}
    	}
    	GPIO_PortClearInterruptFlags(BOARD_USER_BUTTON2_GPIO, 1U << BOARD_USER_BUTTON2_GPIO_PIN);
    }

    /* Button 3 state */
    if(GPIO_GetPinsInterruptFlags(BOARD_USER_BUTTON3_GPIO)&( 1U << BOARD_USER_BUTTON3_GPIO_PIN))
    {
    	if(GPIO_PinRead(BOARD_USER_BUTTON3_GPIO, BOARD_USER_BUTTON3_GPIO_PIN))
    	{
    		setBits   |= QMC_IOEVENT_BTN3_RELEASED;
    		clearBits |= QMC_IOEVENT_BTN3_PRESSED;
    		buttonPressedTime[2] = currentTicks;
    	} else {
    		if ((currentTicks - buttonPressedTime[2]) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
    		{
    			setBits   |= QMC_IOEVENT_BTN3_PRESSED;
    	    	clearBits |= QMC_IOEVENT_BTN3_RELEASED;
    		}
    	}
    	GPIO_PortClearInterruptFlags(BOARD_USER_BUTTON3_GPIO, 1U << BOARD_USER_BUTTON3_GPIO_PIN);
    }

    /* Button 4 state */
    if(GPIO_GetPinsInterruptFlags(BOARD_USER_BUTTON4_GPIO)&( 1U << BOARD_USER_BUTTON4_GPIO_PIN))
    {
    	if(GPIO_PinRead(BOARD_USER_BUTTON4_GPIO, BOARD_USER_BUTTON4_GPIO_PIN))
    	{
    		setBits   |= QMC_IOEVENT_BTN4_RELEASED;
    		clearBits |= QMC_IOEVENT_BTN4_PRESSED;
    		buttonPressedTime[3] = currentTicks;
    	} else {
    		if ((currentTicks - buttonPressedTime[3]) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
    		{
    			setBits   |= QMC_IOEVENT_BTN4_PRESSED;
    			clearBits |= QMC_IOEVENT_BTN4_RELEASED;
    		}
    	}
    	GPIO_PortClearInterruptFlags(BOARD_USER_BUTTON4_GPIO, 1U << BOARD_USER_BUTTON4_GPIO_PIN);
    }

	/* Update event group */
    xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, clearBits);
    xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, setBits, NULL);

    SDK_ISR_EXIT_BARRIER;
}

/***************************************************************************//*!
 * @brief   DIGITAL inputs 0-1 interrupt handler
 * @details .
 ******************************************************************************/
void BOARD_DIG_IN0_IRQ_HANDLER(void)
{
	EventBits_t clearBits=0, setBits=0;
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
	TickType_t currentTicks = xTaskGetTickCount();
	static TickType_t buttonPressedTime[2] = { 0, 0 };
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */

	/* User input 0 state */
	if(GPIO_GetPinsInterruptFlags(BOARD_DIG_IN0_GPIO)&( 1U << BOARD_DIG_IN0_GPIO_PIN))
	{
#if FEATURE_DETECT_POWER_LOSS
		xTaskNotifyFromISR(g_datalogger_task_handle, kDLG_SHUTDOWN_PowerLoss, eSetBits, NULL);
#endif /* FEATURE_DETECT_POWER_LOSS */

		if(GPIO_PinRead(BOARD_DIG_IN0_GPIO, BOARD_DIG_IN0_GPIO_PIN))
		{
			setBits   |= QMC_IOEVENT_INPUT0_HIGH;
			clearBits |= QMC_IOEVENT_INPUT0_LOW;
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
			buttonPressedTime[0] = currentTicks;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
		} else {
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
			if ((currentTicks - buttonPressedTime[0]) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
			{
				setBits   |= QMC_IOEVENT_INPUT0_LOW;
				clearBits |= QMC_IOEVENT_INPUT0_HIGH;
			}
#else
			setBits   |= QMC_IOEVENT_INPUT0_LOW;
			clearBits |= QMC_IOEVENT_INPUT0_HIGH;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
		}

		GPIO_PortClearInterruptFlags(BOARD_DIG_IN0_GPIO, 1U << BOARD_DIG_IN0_GPIO_PIN);
	}

	if(GPIO_GetPinsInterruptFlags(BOARD_DIG_IN1_GPIO)&( 1U << BOARD_DIG_IN1_GPIO_PIN))
	{
    	if(GPIO_PinRead(BOARD_DIG_IN1_GPIO, BOARD_DIG_IN1_GPIO_PIN))
    	{
    		setBits   |= QMC_IOEVENT_INPUT1_HIGH;
    		clearBits |= QMC_IOEVENT_INPUT1_LOW;
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
    		buttonPressedTime[1] = currentTicks;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
    	} else {
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
    		if ((currentTicks - buttonPressedTime[1]) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
			{
				setBits   |= QMC_IOEVENT_INPUT1_LOW;
				clearBits |= QMC_IOEVENT_INPUT1_HIGH;
			}
#else
			setBits   |= QMC_IOEVENT_INPUT1_LOW;
			clearBits |= QMC_IOEVENT_INPUT1_HIGH;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
    	}
		GPIO_PortClearInterruptFlags(BOARD_DIG_IN1_GPIO, 1U << BOARD_DIG_IN1_GPIO_PIN);
	}

	/* Update event group */
    xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, clearBits);
    xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, setBits, NULL);

	SDK_ISR_EXIT_BARRIER;
}

/***************************************************************************//*!
 * @brief   DIGITAL inputs 2 interrupt handler
 * @details .
 ******************************************************************************/
void BOARD_DIG_IN2_IRQ_HANDLER(void)
{
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
	TickType_t currentTicks = xTaskGetTickCount();
	static TickType_t buttonPressedTime =  0;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */

	if(GPIO_GetPinsInterruptFlags(BOARD_DIG_IN2_GPIO)&( 1U << BOARD_DIG_IN2_GPIO_PIN))
	{
    	if(GPIO_PinRead(BOARD_DIG_IN2_GPIO, BOARD_DIG_IN2_GPIO_PIN))
    	{
    		xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT2_LOW);
    		xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT2_HIGH, NULL);
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
    		buttonPressedTime = currentTicks;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
    	} else {
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
    		if ((currentTicks - buttonPressedTime) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
			{
    			xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT2_HIGH);
				xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT2_LOW, NULL);
			}
#else
    		xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT2_HIGH);
			xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT2_LOW, NULL);
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
    	}
		GPIO_PortClearInterruptFlags(BOARD_DIG_IN2_GPIO, 1U << BOARD_DIG_IN2_GPIO_PIN);
	}
	SDK_ISR_EXIT_BARRIER;
}

/***************************************************************************//*!
 * @brief   DIGITAL inputs 3 interrupt handler
 * @details .
 ******************************************************************************/
void BOARD_DIG_IN3_IRQ_HANDLER(void)
{
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
	TickType_t currentTicks = xTaskGetTickCount();
	static TickType_t buttonPressedTime =  0;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
	if(GPIO_GetPinsInterruptFlags(BOARD_DIG_IN3_GPIO)&( 1U << BOARD_DIG_IN3_GPIO_PIN))
	{
    	if(GPIO_PinRead(BOARD_DIG_IN3_GPIO, BOARD_DIG_IN3_GPIO_PIN))
    	{
    		xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT3_LOW);
    		xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT3_HIGH, NULL);
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
    		buttonPressedTime = currentTicks;
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
    	} else {
#if FEATURE_ENABLE_GPIO_SW_DEBOUNCING
    		if ((currentTicks - buttonPressedTime) >= pdMS_TO_TICKS(GPIO_SW_DEBOUNCE_MS))
			{
    			xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT3_HIGH);
				xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT3_LOW, NULL);
			}
#else
    		xEventGroupClearBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT3_HIGH);
			xEventGroupSetBitsFromISR(g_inputButtonEventGroupHandle, QMC_IOEVENT_INPUT3_LOW, NULL);
#endif /* FEATURE_ENABLE_GPIO_SW_DEBOUNCING */
    	}
		GPIO_PortClearInterruptFlags(BOARD_DIG_IN3_GPIO, 1U << BOARD_DIG_IN3_GPIO_PIN);
	}
	SDK_ISR_EXIT_BARRIER;
}

/***************************************************************************//*!
 * @brief   Handler for the GPR SW interrupt.
 * @details
 * Handler for the GPR SW interrupt. Processes incoming messages from the CM4.
 ******************************************************************************/
void GPR_IRQ_IRQHandler(void)
{
    RPC_HandleISR();
    SDK_ISR_EXIT_BARRIER;
}

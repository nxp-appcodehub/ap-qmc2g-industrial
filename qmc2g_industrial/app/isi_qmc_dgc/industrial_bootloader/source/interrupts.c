/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

 /**************************************************************************//*!
 * @file      interrupts.c
 * @author    
 * @version   0.0.1.0
 * @brief     
 ******************************************************************************/

// Standard C Included Files
#include <qmc2_boot.h>
#include <stdint.h>
#include <stdbool.h>

/* module definitions                                                         */

/* module function prototypes                                                 */

/* module variables                                                           */
   
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
/* Defined in semihost_hardfault.c */
void HardFault_Handler(void)
{
#ifdef DEBUG
  __asm("BKPT #0x02");
#endif

  NVIC_SystemReset();
}


/***************************************************************************//*!
 * @brief   MemManage exception handler
 * @details It will stop application in debug mode with software breakpoint
 *          when memory fault event occur.
 ******************************************************************************/
void MemManage_Handler(void)
{
#ifdef DEBUG
  __asm("BKPT #0x03");
#endif

  NVIC_SystemReset();
}

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


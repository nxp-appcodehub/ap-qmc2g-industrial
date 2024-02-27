/*
 * Copyright 2022-2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "api_fault.h"
#include "task.h"
#include "qmc_features_config.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define BUFFER_SIZE		20  /* Circular fault buffer size - one slot is kept empty for full/empty buffer recognition */
#define QUEUE_SIZE		20	/* Fault queue size */

#if (BUFFER_SIZE > 0xFF)
#error "BUFFER_SIZE MUST FIT WITHIN THE UINT8_T TYPE"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

static uint8_t gs_readBufferHead = 0; /* Circular fault read buffer head index */
static uint8_t gs_writeBufferHead = 0; /* Circular fault write buffer head index */
static fault_source_t gs_faultBuffer[BUFFER_SIZE] = {0}; /* Circular fault buffer */
static uint8_t gs_faultQueue[QUEUE_SIZE * sizeof(fault_source_t)] = {0}; /* Static fault queue */
static StaticQueue_t gs_staticQueue;

QueueHandle_t g_FaultQueue = NULL; /* Global fault queue variable */
fault_system_fault_t g_systemFaultStatus = kFAULT_NoFault; /* Global system fault status variable */
fault_system_fault_t g_FaultHandlingErrorFlags = 0; /* Global variable that holds fault handling error flags */
mc_fault_t g_psbFaults[4] = { 0 }; /* Global PSB faults array */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Initializes the fault queue
 *
 * @retval Returns QMC status.
 */
qmc_status_t FAULT_InitFaultQueue(void)
{
	g_FaultQueue = xQueueCreateStatic(QUEUE_SIZE, sizeof(fault_source_t), gs_faultQueue, &gs_staticQueue);

	if (g_FaultQueue == NULL)
	{
		return kStatus_QMC_Err;
	}
	else
	{
		return kStatus_QMC_Ok;
	}
}

/*!
* @brief Sends a FAULT signal to the Fault Handling service indicating the given fault_source_t. For valid fault sources see referenced enumeration type. (Not ISR safe version)
*
* @param src Source that triggered this fault
*/
void FAULT_RaiseFaultEvent(fault_source_t src)
{
    /* If the fault queue overflows, set the flag in g_FaultHandlingErrorFlags. If the error has already been reported and is marked in
     * the g_systemFaultStatus and the queue is no longer overflowed, clear the flag. This ensures that the flag won't be cleared before
     * the error is reported. */
    if (xTaskNotify(g_fault_handling_task_handle, src, eSetValueWithoutOverwrite) == pdFAIL)
    {
        if (xQueueSend(g_FaultQueue, (void *) &src, 0) == errQUEUE_FULL)
        {
            g_FaultHandlingErrorFlags |= kFAULT_FaultQueueOverflow;
        }
        else if (CONTAINS_QUEUE_OVERFLOW_FLAG(g_systemFaultStatus))
        {
            g_FaultHandlingErrorFlags &= (fault_system_fault_t) ~((uint32_t) kFAULT_FaultQueueOverflow);
        }
        else
        {
            ;
        }
    }
}

/*!
 * @brief Reads from the circular fault buffer.
 *
 * @retval Returns the stored fault. If called on an empty buffer, returns kFAULT_InvalidFaultSource.
 */
fault_source_t FAULT_ReadBuffer(void)
{
	if (FAULT_BufferEmpty())
	{
		return kFAULT_InvalidFaultSource;
	}

	uint8_t localReadBufferHead = gs_readBufferHead;
	fault_source_t retVal = gs_faultBuffer[localReadBufferHead];
	uint8_t nextReadBufferHead = 0;

	/* Checking to be compliant with CERTC-C INT31-C */
	if ((localReadBufferHead + 1) > BUFFER_SIZE)
	{
		nextReadBufferHead = BUFFER_SIZE;
	}
	else
	{
		nextReadBufferHead = localReadBufferHead + 1;

		/* Checking to be compliant with CERTC-C INT30-C */
		if (nextReadBufferHead < localReadBufferHead)
		{
			nextReadBufferHead = 0;
		}
	}

	if (nextReadBufferHead == BUFFER_SIZE)
	{
		nextReadBufferHead = 0;
	}
	gs_readBufferHead = nextReadBufferHead;
	return retVal;
}

/*!
 * @brief Writes a fault into the circular fault buffer. This should only be used by FAULT_RaiseFaultEvent_fromISR.
 *        This functions is NOT ISR SAFE.
 *
 * @param[in] fault The fault that should be written into the buffer.
 *
 * @retval Returns QMC status.
 */
qmc_status_t FAULT_WriteBuffer(fault_source_t fault)
{
	/* Local copy of gs_writeBufferHead to make writing into the buffer more interrupt-friendly (still not interrupt-safe!) */
	uint8_t localWriteBufferHead = gs_writeBufferHead;
	uint8_t nextWriteBufferHead = 0;

	/* Checking to be compliant with CERTC-C INT31-C */
	if ((localWriteBufferHead + 1) > BUFFER_SIZE)
	{
		nextWriteBufferHead = BUFFER_SIZE;
	}
	else
	{
		nextWriteBufferHead = localWriteBufferHead + 1;

		/* Checking to be compliant with CERTC-C INT30-C */
		if (nextWriteBufferHead < localWriteBufferHead)
		{
			nextWriteBufferHead = 0;
		}
	}

	if (nextWriteBufferHead == BUFFER_SIZE)
	{
		nextWriteBufferHead = 0;
	}

	if (nextWriteBufferHead == gs_readBufferHead)
	{
		/* Buffer is full. */
		return kStatus_QMC_Err;
	}

	gs_faultBuffer[localWriteBufferHead] = fault;
	gs_writeBufferHead = nextWriteBufferHead;
	return kStatus_QMC_Ok;
}

/*!
 * @brief Checks if the circular fault buffer is empty.
 *
 * @retval True if buffer is empty, false if it isn't.
 */
bool FAULT_BufferEmpty(void)
{
	return gs_readBufferHead == gs_writeBufferHead;
}

/*
 * Copyright 2022 NXP 
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in accordance with the applicable license terms found at https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html.
 */

#include "api_fault.h"
#include "task.h"
#include "qmc_features_config.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define BUFFER_SIZE		20  /* Circular fault buffer size */
#define QUEUE_SIZE		20	/* Fault queue size */

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
            g_FaultHandlingErrorFlags &= ~(kFAULT_FaultQueueOverflow);
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
 * @retval Returns the stored fault.
 */
fault_source_t FAULT_ReadBuffer(void)
{
	uint8_t localBufferHead = gs_readBufferHead;
	fault_source_t retVal = gs_faultBuffer[localBufferHead++];
	if (localBufferHead == BUFFER_SIZE)
	{
		localBufferHead = 0;
	}
	gs_readBufferHead = localBufferHead;
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
	uint8_t nextWriteBufferHead = localWriteBufferHead + 1;

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

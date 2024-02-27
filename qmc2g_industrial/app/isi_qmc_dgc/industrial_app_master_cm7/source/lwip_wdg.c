/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "lwipopts.h"

#if LWIP_USE_WATCHDOG
#include "api_rpc.h"
#include "api_fault.h"
#include "api_logging.h"

void lwip_KickFunctionalWatchdog()
{
	static bool firstWdKick = true;

	if (firstWdKick)
	{
		if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogWebService) != kStatus_QMC_Ok)
		{
			FAULT_RaiseFaultEvent(kFAULT_FunctionalWatchdogInitFail);
		}

		firstWdKick = false;
	}
	else
	{
		if (RPC_KickFunctionalWatchdog(kRPC_FunctionalWatchdogWebService) != kStatus_QMC_Ok)
		{
			log_record_t logEntryWithoutId = {
					.rhead = {
							.chksum				= 0,
							.uuid				= 0,
							.ts = {
								.seconds		= 0,
								.milliseconds	= 0
							}
					},
					.type = kLOG_SystemData,
					.data.faultDataWithoutID.source = LOG_SRC_Webservice,
					.data.faultDataWithoutID.category = LOG_CAT_General
			};

			logEntryWithoutId.data.faultDataWithoutID.eventCode = LOG_EVENT_FunctionalWatchdogKickFailed;
			LOG_QueueLogEntry(&logEntryWithoutId, false);
		}
	}
}
#endif

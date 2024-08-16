/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/rpapi.c	1.1"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/rpapi.c,v 1.1 1994/02/11 18:24:47 nick Exp $";
#endif

#include "rprinter.h"
#include "rpapi.h"
#include "inform.h"
#include "spxapi.h"
#include "entry_proto.h"
#include "upconfig_proto.h"
#include "rptdr_proto.h"
#include "prt.h"

#define RESET_COUNT				30000

#define SEND_ALWAYS				1
#define SEND_WHEN_DIFFERENT		2


static void
checkPrinterStatus(
	RPrinterInfo_t *rpEntry,
	int				whenToSend);

void
RPSendStatus(
	RPrinterInfo_t *rpEntry)
{
	int		length;
	int		more;
	uint8	sendPacket[SEND_RPRINTER_STATUS_LENGTH];

	TDRSendStatus( &rpEntry->printerStatus, sendPacket );

	rpEntry->spxHandle.dataStreamType = RP_STATUS;
	length = SEND_RPRINTER_STATUS_LENGTH;
	more = FALSE;
	if (SPXSend( &rpEntry->spxHandle, sendPacket, length, NULL, more )
	== FAILURE) {
		Inform( rpEntry, RPMSG_STATUS_NOT_SENT, MSG_DEBUG );
		InformMsg( rpEntry, SPXDisplayErrno( &rpEntry->spxHandle ), MSG_VERBOSE );

		if (rpEntry->spxHandle.errnoType == SPXET_T_DISCONNECT) {
			rpEntry->spxConnection = FALSE;
			RestartRPEntry( rpEntry );
		}
	}
}


void
RPOpenPrinter(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->hostPrinterHandle.printerID < 0) {
		if (rpEntry->problemCount >= ErrorBeforeInform()) {
			if (PRTOpenPrinter( &rpEntry->hostPrinterHandle,
			rpEntry->hostPrinterName,
			rpEntry->printerStatus.printerNumber,
			rpEntry->pserverName ) == FAILURE) {
				rpEntry->problemCount = 0;
			} else {
				rpEntry->printerStatus.troubleCode =
					(uint8) rpEntry->hostPrinterHandle.printerStatus &
					(RP_TROUBLE_CODE_OFFLINE |
					RP_TROUBLE_CODE_OUT_OF_PAPER);
				rpEntry->problemCount = RESET_COUNT;
				/* call PRTGetPrinterType */
			}
		} else {
			rpEntry->problemCount++;
		}
	}
}


void
RPClosePrinter( 
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->hostPrinterHandle.printerID >= 0) {
		if (rpEntry->entryStatus == RP_ENTRY_STATUS_PROCESSING_A_JOB &&
		(rpEntry->printJobStatus == PJ_STATUS_PROCESS_JOB ||
		rpEntry->printJobStatus == PJ_STATUS_PAUSED)) {
			PRTAbortPrintJob( &rpEntry->hostPrinterHandle );
			Inform( rpEntry, RPMSG_PJ_ABORT, MSG_DEBUG );
		}

		PRTClosePrinter( &rpEntry->hostPrinterHandle );
	}
}


void
RPGetPrinterStatus(
	RPrinterInfo_t *rpEntry)
{
	PRTGetPrinterStatus( &rpEntry->hostPrinterHandle );
	checkPrinterStatus( rpEntry, SEND_WHEN_DIFFERENT );
}


void
RPSendDataToPrintJob(
	RPrinterInfo_t *rpEntry,
	uint8			packet[],
	int				length)
{
	PRTHandle_t *printerHandle;
	PRTStartJobInfo_t startJobInfo;

	if (!length)	/* DEBUG_MSG?? */
		return;

	printerHandle = &rpEntry->hostPrinterHandle;

	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB) {
		ChangeRPEntryStatus( rpEntry,
			RP_ENTRY_STATUS_PROCESSING_A_JOB );
		rpEntry->printJobStatus = 0;	/* go to 'default' below */
	}

	TDRStartJobInfo( (uint8 *) 0, 0, &startJobInfo );

	switch (rpEntry->printJobStatus) {
		case PJ_STATUS_PROCESS_JOB:
			PRTSendDataToPrintJob( printerHandle, packet, length );
			break;
		case PJ_STATUS_PAUSED:	/* DEBUG_MSG?? */
			PRTUnpausePrintJob( printerHandle );
			PRTSendDataToPrintJob( printerHandle, packet, length );
			rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
			break;
		case PJ_STATUS_RELEASED:	/* DEBUG_MSG?? */
			PRTReclaimPrinter( printerHandle );
			PRTStartNewPrintJob( printerHandle, &startJobInfo );
			rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
			PRTSendDataToPrintJob( printerHandle, packet, length );
			break;
		default:	/* DEBUG_MSG?? */
			PRTStartNewPrintJob( printerHandle, &startJobInfo );
			rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
			PRTSendDataToPrintJob( printerHandle, packet, length );
			break;
	}

	checkPrinterStatus( rpEntry, SEND_ALWAYS );
	rpEntry->printJobSize += length;
}


void
RPAbortPrintJob(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB)
		return;		/* DEBUG_MSG?? */

	if (rpEntry->printJobStatus == PJ_STATUS_PROCESS_JOB ||
	rpEntry->printJobStatus == PJ_STATUS_PAUSED)
		PRTAbortPrintJob( &rpEntry->hostPrinterHandle );

	ChangeRPEntryStatus( rpEntry, RP_ENTRY_STATUS_WAITING_FOR_JOB );
	Inform( rpEntry, RPMSG_PJ_ABORT, MSG_DEBUG );
}


void
RPPausePrintJob(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB)
		return;		/* DEBUG_MSG?? */

	if (rpEntry->printJobStatus == PJ_STATUS_PROCESS_JOB) {
		PRTPausePrintJob( &rpEntry->hostPrinterHandle );
		rpEntry->printJobStatus = PJ_STATUS_PAUSED;
	}	/* else DEBUG_MSG?? */

	Inform( rpEntry, RPMSG_PJ_PAUSED, MSG_DEBUG );
}


void
RPUnpausePrintJob(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB)
		return;		/* DEBUG_MSG?? */

	if (rpEntry->printJobStatus == PJ_STATUS_PAUSED) {
		PRTUnpausePrintJob( &rpEntry->hostPrinterHandle );
		rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
	}	/* else DEBUG_MSG?? */

	Inform( rpEntry, RPMSG_PJ_UNPAUSED, MSG_DEBUG );
}


void
RPSendSidebandPrintJob(
	RPrinterInfo_t *rpEntry,
	uint8			packet[],
	int				length)
{
	PRTHandle_t *printerHandle;
	PRTSideband_t sidebandInfo;

	if (length < SIDEBAND_INFO_LENGTH)	/* DEBUG_MSG?? */
		return;

	printerHandle = &rpEntry->hostPrinterHandle;
	TDRSidebandInfo( packet, &sidebandInfo );

	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB) {
		ChangeRPEntryStatus( rpEntry,
			RP_ENTRY_STATUS_PROCESSING_A_JOB );
		rpEntry->printJobStatus = 0;	/* go to 'default' below */
	}

	switch (rpEntry->printJobStatus) {
		case PJ_STATUS_PROCESS_JOB:	/* DEBUG_MSG?? */
			PRTPausePrintJob( printerHandle );
			PRTSidebandPrintJob( printerHandle, &sidebandInfo );
			PRTUnpausePrintJob( printerHandle );
			break;
		case PJ_STATUS_PAUSED:
			PRTSidebandPrintJob( printerHandle, &sidebandInfo );
			break;
		case PJ_STATUS_RELEASED:	/* DEBUG_MSG?? */
			PRTReclaimPrinter( printerHandle );
			PRTSidebandPrintJob( printerHandle, &sidebandInfo );
			PRTReleasePrinter( printerHandle );
			break;
		default:
			PRTSidebandPrintJob( printerHandle,
				&sidebandInfo );
			ChangeRPEntryStatus( rpEntry,
				RP_ENTRY_STATUS_WAITING_FOR_JOB );
			break;
	}
	rpEntry->printerStatus.inSideband = 0; /* done with sideband */
	checkPrinterStatus( rpEntry, SEND_ALWAYS );
	Inform( rpEntry, RPMSG_PJ_SIDEBAND, MSG_DEBUG );
}


void
RPStartNewPrintJob(
	RPrinterInfo_t *rpEntry,
	uint8			packet[],
	int				length)
{
	PRTHandle_t *printerHandle;
	PRTStartJobInfo_t startJobInfo;

	printerHandle = &rpEntry->hostPrinterHandle;
	TDRStartJobInfo( packet, length, &startJobInfo );

	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB) {
		ChangeRPEntryStatus( rpEntry,
			RP_ENTRY_STATUS_PROCESSING_A_JOB );
		rpEntry->printJobStatus = 0;	/* go to 'default' below */
	}

	switch (rpEntry->printJobStatus) {
		case PJ_STATUS_PROCESS_JOB:
			PRTEndPrintJob( printerHandle );
			InformWithInt( rpEntry, RPMSG_PJ_END_BY_NEW,
				rpEntry->printJobSize, MSG_DEBUG );
			rpEntry->printJobSize = 0;
			PRTStartNewPrintJob( printerHandle, &startJobInfo );
			break;
		case PJ_STATUS_PAUSED:	/* DEBUG_MSG?? */
			PRTEndPrintJob( printerHandle );
			PRTStartNewPrintJob( printerHandle, &startJobInfo );
			rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
			break;
		case PJ_STATUS_RELEASED:	/* DEBUG_MSG?? */
			PRTReclaimPrinter( printerHandle );
			PRTStartNewPrintJob( printerHandle, &startJobInfo );
			rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
			break;
		default:
			PRTStartNewPrintJob( printerHandle, &startJobInfo );
			rpEntry->printJobStatus = PJ_STATUS_PROCESS_JOB;
			break;
	}

	checkPrinterStatus( rpEntry, SEND_WHEN_DIFFERENT );
	InformWithInt( rpEntry, RPMSG_PJ_NEW,
		(long) startJobInfo.tabExpansion, MSG_DEBUG );
}


void
RPEndPrintJob(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB)
		return;		/* DEBUG_MSG?? */

	if (rpEntry->printJobStatus == PJ_STATUS_PROCESS_JOB) {
		PRTEndPrintJob( &rpEntry->hostPrinterHandle );
		ChangeRPEntryStatus( rpEntry, RP_ENTRY_STATUS_WAITING_FOR_JOB );
	}	/* else DEBUG_MSG?? */

	InformWithInt( rpEntry, RPMSG_PJ_EOJ, rpEntry->printJobSize,
		MSG_DEBUG );
	rpEntry->printJobSize = 0;
}


void
RPEndPrintJobTimeout(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB)
		return;		/* DEBUG_MSG?? */

	if (rpEntry->printJobStatus == PJ_STATUS_PROCESS_JOB) {
		PRTEndPrintJob( &rpEntry->hostPrinterHandle );
		ChangeRPEntryStatus( rpEntry, RP_ENTRY_STATUS_WAITING_FOR_JOB );
	}	/* else DEBUG_MSG?? */

	InformWithInt( rpEntry, RPMSG_PJ_TIMEOUT, rpEntry->printJobSize,
		MSG_DEBUG );
	rpEntry->printJobSize = 0;
}


void
RPReleasePrinter(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB) {
		ChangeRPEntryStatus( rpEntry,
			RP_ENTRY_STATUS_PROCESSING_A_JOB );
		rpEntry->printJobStatus = 0;	/* go to 'default' below */
	}

	switch (rpEntry->printJobStatus) {
		case PJ_STATUS_PROCESS_JOB:
			PRTEndPrintJob( &rpEntry->hostPrinterHandle );
			PRTReleasePrinter( &rpEntry->hostPrinterHandle );
			rpEntry->printJobStatus = PJ_STATUS_RELEASED;
			break;
		case PJ_STATUS_PAUSED:	/* DEBUG_MSG?? */
			PRTAbortPrintJob( &rpEntry->hostPrinterHandle );
			PRTReleasePrinter( &rpEntry->hostPrinterHandle );
			rpEntry->printJobStatus = PJ_STATUS_RELEASED;
			break;
		case PJ_STATUS_RELEASED:	/* DEBUG_MSG?? */
			break;
		default:
			PRTReleasePrinter( &rpEntry->hostPrinterHandle );
			rpEntry->printJobStatus = PJ_STATUS_RELEASED;
			break;
	}
}


void
RPReclaimPrinter(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->entryStatus != RP_ENTRY_STATUS_PROCESSING_A_JOB)
		return;		/* DEBUG_MSG?? */

	if (rpEntry->printJobStatus == PJ_STATUS_RELEASED) {
		PRTReclaimPrinter( &rpEntry->hostPrinterHandle );
		ChangeRPEntryStatus( rpEntry, RP_ENTRY_STATUS_WAITING_FOR_JOB );
	}	/* else DEBUG_MSG?? */
}


static void
checkPrinterStatus(
	RPrinterInfo_t *rpEntry,
	int				whenToSend)
{
	uint8 troubleCode;

	troubleCode = (uint8) rpEntry->hostPrinterHandle.printerStatus &
		(RP_TROUBLE_CODE_OFFLINE | RP_TROUBLE_CODE_OUT_OF_PAPER);

	if (troubleCode) {
		rpEntry->printerStatus.needBlocks = 0;
		rpEntry->printerStatus.finishedBlocks = 0;
	} else {
		rpEntry->printerStatus.needBlocks = 1;
	}

	if (rpEntry->printerStatus.troubleCode != troubleCode )
		InformWithInt( rpEntry, RPMSG_NEW_PRINTER_STATUS,
			(long) troubleCode, MSG_DEBUG );

	if (whenToSend == SEND_ALWAYS) {
		if (!troubleCode)
			rpEntry->printerStatus.finishedBlocks = 1;
		rpEntry->printerStatus.troubleCode = troubleCode;
		RPSendStatus( rpEntry );
	} else {
		if (rpEntry->printerStatus.troubleCode != troubleCode ) {
			rpEntry->printerStatus.troubleCode = troubleCode;
			RPSendStatus( rpEntry );
		}
	}
}

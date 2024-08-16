/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/commands.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/commands.c,v 1.1 1994/02/11 18:23:51 nick Exp $";
#endif

#include <ctype.h>
#include <unistd.h>
#include "rprinter.h"
#include "rpapi.h"
#include "inform.h"
#include "fdpoll_proto.h"
#include "spxapi.h"
#include "upconfig_proto.h"
#include "entry_proto.h"
#include "commands_proto.h"

#define MAX_RECIEVE_PACKET_LENGTH			520


int
HavePServerCommand(
	RPrinterInfo_t *rpList[],
	int				rpCount)
{
	int i;
	int count;
	int waitCount = 0;
	FDWait_t spxWaitList[MAX_PRINTERS];

	for (i = 0; i < rpCount; i++)
		if (rpList[i]->spxConnection) {
			spxWaitList[waitCount].index = i;
			spxWaitList[waitCount].fd =
				rpList[i]->spxHandle.fd;
			spxWaitList[waitCount++].hasInput = FALSE;
		}

#ifndef DEBUG
	if (!waitCount) {
		count = MaxWaitPacketTime() / 1000;
		if (!count)
			count = 1;
		sleep( count );
		return FALSE;
	}
#endif

	if ((count = FDWaitForInput( spxWaitList, waitCount,
	MaxWaitPacketTime() )) == FAILURE) {
		Inform( NULL, RPMSG_PSERVER_WAIT_ERROR, MSG_DEBUG );
		InformMsg( NULL, SPXDisplayErrno( (SPXHandle_t *) 0 ), MSG_DEBUG );
		return FALSE;
	}

	if (!count)			/* MAX_WAIT_FOR_PACKET_TIME exceeded */
		return FALSE;

	for (i = 0; i < waitCount; i++)
		if (spxWaitList[i].hasInput)
			rpList[spxWaitList[i].index]->hasCommand = TRUE;

	return TRUE;
}


void
ProcessPServerCommands(
RPrinterInfo_t *rpList[],
int				rpCount)
{
	int i;
	int length;
	uint8 pserverCommand;
	uint8 rcvPacket[MAX_RECIEVE_PACKET_LENGTH];
	register RPrinterInfo_t *rpPtr;

	for (i = 0; i < rpCount; i++) {
		rpPtr = rpList[i];

		if (rpPtr->hasCommand) {
			rpPtr->hasCommand = FALSE;

			length = MAX_RECIEVE_PACKET_LENGTH;
			if (SPXReceive( &rpPtr->spxHandle, rcvPacket, &length) == FAILURE) {
				Inform( rpPtr, RPMSG_COMMAND_NO_RECEIVE, MSG_DEBUG );
				InformMsg(rpPtr, SPXDisplayErrno( &rpPtr->spxHandle ), MSG_DEBUG );

				if (rpPtr->spxHandle.errnoType == SPXET_T_DISCONNECT) {
					rpPtr->spxConnection = FALSE;
					RestartRPEntry( rpPtr );
				}
				continue;
			}

			rpPtr->endOfJobCountDown =
				IdleBeforeJobEnd() + 1;

			pserverCommand = rpPtr->spxHandle.dataStreamType;
			switch (pserverCommand) {
				case RP_DATA:
					RPSendDataToPrintJob( rpPtr, rcvPacket, length );
					break;
				case RP_FLUSH:
					RPAbortPrintJob( rpPtr );
					break;
				case RP_PAUSE:
					RPPausePrintJob( rpPtr );
					break;
				case RP_START:
					RPUnpausePrintJob( rpPtr );
					break;
				case RP_SIDEBAND:
					RPSendSidebandPrintJob( rpPtr, rcvPacket,
						length );
					break;
				case RP_NEW_JOB:
					RPStartNewPrintJob( rpPtr, rcvPacket, length );
					break;
				case RP_RELEASE:
					RPReleasePrinter( rpPtr );
					break;
				case RP_RECLAIM:
					RPReclaimPrinter( rpPtr );
					break;
				case RP_EOJ:
					RPEndPrintJob( rpPtr );
					break;
				case SPX_CONNECTION_TERMINATED:
				case SPX_CONNECTION_FAILED:
					Inform(rpPtr, RPMSG_PSERVER_DOWN, MSG_WARN);
					RestartRPEntry( rpPtr );
					break;
					
				default:
					InformWithInt( rpPtr, RPMSG_BAD_RP_COMMAND,
						(long) pserverCommand, MSG_DEBUG );
					break;
			}
		}
	}
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/entry.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/entry.c,v 1.1 1994/02/11 18:23:57 nick Exp $";
#endif

#include "rprinter.h"
#include "inform.h"
#include "rpapi.h"
#include "entry_proto.h"

#define RESET_COUNT			30000


int
IsRPEntryInList(
	RPrinterInfo_t	*rpEntry,
	RPrinterInfo_t	*rpList[],
	int				 rpCount,
	int				*index)
{
	for (*index = 0; *index < rpCount; (*index)++)
		if (!strcmp( rpEntry->pserverName, rpList[*index]->pserverName )
		&& rpEntry->printerStatus.printerNumber
		== rpList[*index]->printerStatus.printerNumber
		&& !strcmp( rpEntry->hostPrinterName,
		rpList[*index]->hostPrinterName ))
			return TRUE;

	return FALSE;
}


void
AddRPEntry(
	RPrinterInfo_t *rpEntry,
	RPrinterInfo_t *rpList[],
	int			   *rpCount)
{
	RPrinterInfo_t *rpPtr;
	RPrinterStatus_t *rpStatus;

	if (*rpCount + 1 >= MAX_PRINTERS)
	{
		Inform( (RPrinterInfo_t *) 0, RPMSG_TOO_MANY_PRINTERS,
			MSG_ERROR );
		return;
	}

	rpPtr = (RPrinterInfo_t *) malloc( sizeof( RPrinterInfo_t ) );

	rpPtr->entryStatus = RP_ENTRY_STATUS_INIT_HAVE_NAME;
	rpPtr->toBeDeleted = FALSE;
	rpPtr->problemCount = RESET_COUNT;
	rpPtr->idleCount = 0;
	strcpy( rpPtr->pserverName, rpEntry->pserverName );
	rpPtr->spxTransportOpen = FALSE;
	rpPtr->spxConnection = FALSE;
	rpPtr->hostPrinterHandle.printerID = -1;
	strcpy( rpPtr->hostPrinterName, rpEntry->hostPrinterName );
	rpPtr->hostNameChange = FALSE;
	rpPtr->hasCommand = FALSE;

	rpStatus = &rpPtr->printerStatus;

	rpStatus->printerNumber = rpEntry->printerStatus.printerNumber;
	rpStatus->needBlocks = 1;
	rpStatus->finishedBlocks = 0;
	rpStatus->troubleCode = 0;
	rpStatus->inSideband = 0;

	rpList[(*rpCount)++] = rpPtr;
	InformWithStr( rpPtr, RPMSG_PRINTER_STARTING,
		rpPtr->hostPrinterName, MSG_VERBOSE );
}


void
DeleteRPEntry(
	RPrinterInfo_t *rpList[],
	int			   *rpCount,
	int				entryIndex)
{
	int i;
	RPrinterInfo_t *rpEntry;

	rpEntry = rpList[entryIndex];
	InformWithStr( rpEntry, RPMSG_PRINTER_STOPPING,
		rpEntry->hostPrinterName, MSG_VERBOSE );

	if (rpEntry->spxTransportOpen) {
		if (rpEntry->spxConnection)
			SPXDisconnect( &rpEntry->spxHandle );
		SPXCloseTransport( &rpEntry->spxHandle );
	}

	if (rpEntry->hostPrinterHandle.printerID >= 0)
		RPClosePrinter( rpEntry );

	free( (char *) rpEntry );

	for (i = entryIndex + 1; i < *rpCount; i++)
		rpList[i - 1] = rpList[i];

	(*rpCount)--;
}


void
RestartRPEntry(
	RPrinterInfo_t *rpEntry)
{
	RPrinterStatus_t *rpStatus;

	InformWithStr( rpEntry, RPMSG_PRINTER_RESTARTING,
		rpEntry->hostPrinterName, MSG_VERBOSE );

	if (rpEntry->spxTransportOpen) {
		if (rpEntry->spxConnection) {
			SPXDisconnect( &rpEntry->spxHandle );
			rpEntry->spxConnection = FALSE;
		}
		SPXCloseTransport( &rpEntry->spxHandle );
		rpEntry->spxTransportOpen = FALSE;
	}

	if (rpEntry->hostPrinterHandle.printerID >= 0)
		RPClosePrinter( rpEntry );

	/* RestartRPEntry should not mess with the 'toBeDeleted' flag. */
	rpEntry->entryStatus = RP_ENTRY_STATUS_INIT_HAVE_NAME;
	rpEntry->problemCount = RESET_COUNT;
	rpEntry->idleCount = 0;
	rpEntry->hostNameChange = FALSE;
	rpEntry->hasCommand = FALSE;

	rpStatus = &rpEntry->printerStatus;

	rpStatus->needBlocks = 0;
	rpStatus->finishedBlocks = 0;
	rpStatus->troubleCode = 0;
	rpStatus->inSideband = 0;
}


void
ChangeRPEntryStatus(
	RPrinterInfo_t *rpEntry,
	int				newStatus)
{
	if (newStatus < RP_ENTRY_STATUS_INIT_HAVE_NAME ||
	newStatus > RP_ENTRY_STATUS_PROCESSING_A_JOB) {
		InformWithInt( rpEntry, RPMSG_CHANGE_ENTRY_STATUS,
			(long) newStatus, MSG_DEBUG );
		RestartRPEntry( rpEntry );
		return;
	}

	rpEntry->entryStatus = newStatus;

	switch (rpEntry->entryStatus) {
		case RP_ENTRY_STATUS_INIT_HAVE_NAME:
		case RP_ENTRY_STATUS_INIT_HAVE_ADDRESS:
		case RP_ENTRY_STATUS_INIT_HAVE_SOCKET:
			rpEntry->problemCount = RESET_COUNT;
			break;
		case RP_ENTRY_STATUS_WAITING_FOR_JOB:
			rpEntry->printerStatus.needBlocks = 1;
			rpEntry->idleCount = 0;
			break;
		case RP_ENTRY_STATUS_PROCESSING_A_JOB:
			rpEntry->problemCount = 0;
			break;
	}
}

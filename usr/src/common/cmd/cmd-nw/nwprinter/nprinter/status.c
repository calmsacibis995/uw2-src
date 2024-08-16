/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/status.c	1.2"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/status.c,v 1.2 1994/04/13 21:10:03 novell Exp $";
#endif

#include <sys/types.h>
#include <string.h>
#include <nw/nwcaldef.h>
#include <nw/nwbindry.h>
#include "rprinter.h"
#include "inform.h"
#include "ipxapi.h"
#include "spxapi.h"
#include "psapi.h"
#include "rpapi.h"
#include "entry_proto.h"
#include "misc_proto.h"
#include "status_proto.h"
#include "upconfig_proto.h"



static int havePServerAddress( RPrinterInfo_t *rpEntry);
static int haveRPrinterSocket( RPrinterInfo_t *rpEntry);
static int haveHostPrinterOpen( RPrinterInfo_t *rpEntry);
static int havePServerRPConnection( RPrinterInfo_t *rpEntry);
static int isRPStatusSent( RPrinterInfo_t *rpEntry);

/*********************************************************************
** BEGIN_MANUAL_ENTRY( StatusCheckRPrinters, \
**                     api/utils/rprinter/status )
**			loops through the list of remote printers and monitors
**			and regulates the status of each entry
**
** SYNOPSIS
**			void StatusCheckRPrinters( rpList, rpCount )
**			RPrinterInfo_t *rpList[];
**			int			   *rpCount;
**
** INPUT
**			rpList				the list of remote printers which
**								consists of an array of pointers to
**								remote printer information structures;
**
**			rpCount				a pointer to the count of remote
**								printers currently in the list of
**								remote printers
**
** OUTPUT
**			rpList				if remote printer entries are deleted
**								the list is adjusted accordingly
**
**			rpCount				if remote printer entries are deleted
**								the count is decremented
**
** DESCRIPTION
**			As we loop through the list, the first thing we do is
**			see if a remote printer has been marked
**			"toBeDeleted == TRUE". And if that remote printer is not
**			in the middle of a job we delete it.
**
**			The other thing we do is make sure that the initialization
**			of a remote printer is pushed along as far as it can go
**			for the time being. If a remote printer ever looses its
**			connection to the PSERVER, it has to reinitialize through
**			this same process.
**
**			Significant global routines called are:
**				DeleteRPEntry
**				ChangeRPEntryStatus
**				RestartRPEntry
**				RPGetPrinterStatus
**				RPEndPrintJobTimeout
**
**			Significant local routines called are:
**				havePServerAddress
**				haveRPrinterSocket
**				haveHostPrinterOpen
**				havePServerRPConnection
**				isRPStatusSent
**
** SEE ALSO
**			main, Shutdown, IsTimeForCheckup, UpdateConfiguration,
**			UpdateControlInfo
**
** END_MANUAL_ENTRY ( 11/19/90 )
**/

void
StatusCheckRPrinters(
	RPrinterInfo_t *rpList[],
	int			   *rpCount)
{
	int i;
	int oldRPStatus;
	register RPrinterInfo_t *rpPtr;

	for (i = 0; i < *rpCount; i++) {
		rpPtr = rpList[i];

		if (rpPtr->toBeDeleted) {
			switch (rpPtr->entryStatus) {
				case RP_ENTRY_STATUS_PROCESSING_A_JOB:
					break;
				case RP_ENTRY_STATUS_INIT_HAVE_NAME:
				case RP_ENTRY_STATUS_INIT_HAVE_ADDRESS:
				case RP_ENTRY_STATUS_INIT_HAVE_SOCKET:
				case RP_ENTRY_STATUS_WAITING_FOR_JOB:
				default:
					DeleteRPEntry( rpList, rpCount, i );
					i--;
					continue;
			}
		}

		do {
			oldRPStatus = rpPtr->entryStatus;

			switch (rpPtr->entryStatus) {
				case RP_ENTRY_STATUS_INIT_HAVE_NAME:
					if (havePServerAddress( rpPtr ))
						ChangeRPEntryStatus( rpPtr,
							RP_ENTRY_STATUS_INIT_HAVE_ADDRESS);
					break;
				case RP_ENTRY_STATUS_INIT_HAVE_ADDRESS:
					if (haveRPrinterSocket( rpPtr ))
						ChangeRPEntryStatus( rpPtr,
							RP_ENTRY_STATUS_INIT_HAVE_SOCKET);
					break;
				case RP_ENTRY_STATUS_INIT_HAVE_SOCKET:
					if (havePServerRPConnection( rpPtr )
					&& isRPStatusSent( rpPtr )) {
						if (haveHostPrinterOpen( rpPtr )) {
							ChangeRPEntryStatus( rpPtr,
								RP_ENTRY_STATUS_WAITING_FOR_JOB);
							RPSendStatus( rpPtr );
							Inform( rpPtr, RPMSG_PRINTER_READY,
								MSG_VERBOSE );
						} else {
							SPXDisconnect( &rpPtr->spxHandle );
							rpPtr->spxConnection = FALSE;
							ChangeRPEntryStatus( rpPtr,
								RP_ENTRY_STATUS_INIT_HAVE_ADDRESS);
						}
					}
					break;
				case RP_ENTRY_STATUS_WAITING_FOR_JOB:
					if (rpPtr->hostNameChange) {
						rpPtr->hostNameChange = FALSE;
						InformWithStr( rpPtr, RPMSG_NEW_HOST_PRT_NAME,
							rpPtr->hostPrinterName, MSG_VERBOSE );
						RPClosePrinter( rpPtr );
						SPXDisconnect( &rpPtr->spxHandle );
						rpPtr->spxConnection = FALSE;
						ChangeRPEntryStatus( rpPtr,
							RP_ENTRY_STATUS_INIT_HAVE_SOCKET);
					} else {
						if (++(rpPtr->idleCount) >=
						IdleBeforePrtStatus()) {
							RPGetPrinterStatus( rpPtr );
							rpPtr->idleCount = 0;
						}
					}
					break;
				case RP_ENTRY_STATUS_PROCESSING_A_JOB:
					if (rpPtr->printJobStatus ==
					PJ_STATUS_PROCESS_JOB) {
						if (rpPtr->printerStatus.troubleCode) {
								RPGetPrinterStatus( rpPtr );
								rpPtr->problemCount = 0;
						} else {
							if (--(rpPtr->endOfJobCountDown) <= 0)
								RPEndPrintJobTimeout( rpPtr );
						}
					}
					break;
				default:
					InformWithInt( rpPtr, RPMSG_INVALID_ENTRY_STATUS,
						(long) rpPtr->entryStatus, MSG_DEBUG );
					RestartRPEntry( rpPtr );
					break;
			}
		} while (rpPtr->entryStatus > oldRPStatus);
	}
}


/*********************************************************************
** BEGIN_MANUAL_ENTRY( havePServerAddress, \
**                     api/utils/rprinter/std/haveaddr )
**			uses IPX protocol to get the IPX/SPX address of the
**			advertising PSERVER
**
** SYNOPSIS
**			ok = havePServerAddress( rpEntry )
**			int ok;
**			RPrinterInfo_t *rpEntry;
**
** INPUT
**			rpEntry				the pserverName field contains the
**								server name whose address we are
**								seeking
**
** OUTPUT
**			rpEntry				the pserverSPXAddress field is updated
**								upon successful completion
**
** RETURN VALUES
**			ok					return TRUE if we are able to get
**								the PSERVER address, otherwise FALSE
**
** DESCRIPTION
**			If anything goes wrong (ie. can't open IPX transport,
**			etc.) we report it at specified intervals.
**
**			Significant global routines called are:
**				IPXOpenTransport
**				IPXIsServerAdvertising
**				IPXCloseTransport
**
** SEE ALSO
**			StatusCheckConfiguration
**
** END_MANUAL_ENTRY ( 11/12/90 )
**/

static int
havePServerAddress(
	RPrinterInfo_t *rpEntry)
{
	IPXHandle_t			ipxHandle;
	IPXsapServerInfo_t	sapInfo;

	if (IPXOpenTransport( &ipxHandle, 0 ) == FAILURE) {
		if (rpEntry->problemCount >= ErrorBeforeInform()) {
			Inform( rpEntry, RPMSG_NO_IPX_OPEN, MSG_ERROR );
			InformMsg( rpEntry, IPXDisplayErrno( &ipxHandle ),
				MSG_VERBOSE );
			rpEntry->problemCount = 0;
		} else {
			rpEntry->problemCount++;
		}

		return FALSE;
	}

	sapInfo.serverType = NSwap16(OT_ADVERTISING_PRINT_SERVER);
	strncpy( sapInfo.serverName, rpEntry->pserverName,
		IPXMAX_SERVER_NAME_LENGTH );

	if (!IPXIsServerAdvertising( &ipxHandle, &sapInfo )) {
		if (rpEntry->problemCount >= WarnBeforeInform()) {
			Inform( rpEntry, RPMSG_NO_ADVERTISE, MSG_WARN );
			rpEntry->problemCount = 0;
		} else {
			rpEntry->problemCount++;
		}
		IPXCloseTransport( &ipxHandle );
		return FALSE;
	}

	IPXCloseTransport( &ipxHandle );
	memcpy( (char *) rpEntry->pserverSPXAddress,
		(char *) sapInfo.ipxAddress, SPX_ADDRESS_LENGTH );
	InformWithStr( rpEntry, RPMSG_PSERVER_ADDR,
		SPXDisplayAddress( rpEntry->pserverSPXAddress ),
		MSG_DEBUG );

	return TRUE;
}


/*********************************************************************
** BEGIN_MANUAL_ENTRY( haveRPrinterSocket, \
**                     api/utils/rprinter/std/haveaddr )
**			uses PSERVER client SPX protocol to get the socket number
**			for the PSERVER remote printer SPX connection
**
** SYNOPSIS
**			ok = haveRPrinterSocket( rpEntry )
**			int ok;
**			RPrinterInfo_t *rpEntry;
**
** INPUT
**			rpEntry				the pserverSPXAddress field contains
**								the address for the PSERVER client
**								SPX protocol
**
** OUTPUT
**			rpEntry				the socket number portion of the
**								pserverSPXAddress field is updated
**								upon successful completion
**
** RETURN VALUES
**			ok					return TRUE if we are able to get
**								the socket number for the PSERVER
**								remote printer SPX connection,
**								otherwise FALSE
**
** DESCRIPTION
**			If anything goes wrong (ie. can't open SPX transport,
**			etc.) we report it at specified intervals.
**
**			Significant global routines called are:
**				SPXOpenTransport
**				SPXConnect
**				PSRequestRemotePrinter
**				SPXDisconnect
**				SPXSetSocketInAddress
**
** SEE ALSO
**			StatusCheckConfiguration
**
** END_MANUAL_ENTRY ( 11/12/90 )
**/

static int
haveRPrinterSocket(
	RPrinterInfo_t *rpEntry)
{
	int msgNum;
	PSRPrinterInfo_t rprinterInfo;

	if (!rpEntry->spxTransportOpen) {
		if (SPXOpenTransport( &rpEntry->spxHandle, 0 ) == FAILURE) {
			if (rpEntry->problemCount >= ErrorBeforeInform()) {
				Inform( rpEntry, RPMSG_NO_SPX_OPEN, MSG_ERROR );
				InformMsg( rpEntry, SPXDisplayErrno( &rpEntry->spxHandle ),
						MSG_VERBOSE );
				rpEntry->problemCount = 0;
			} else {
				rpEntry->problemCount++;
			}
			return FALSE;
		} else {
			rpEntry->spxTransportOpen = TRUE;
		}
	}

	if (SPXConnect( &rpEntry->spxHandle, rpEntry->pserverSPXAddress,
	(SPXOptionsInfo_t *) 0 ) == FAILURE) {
		if (rpEntry->problemCount >= WarnBeforeInform()) {
			Inform( rpEntry, RPMSG_NO_SPX_CL_CONNECT, MSG_WARN );
			InformMsg( rpEntry, SPXDisplayErrno( &rpEntry->spxHandle ),
				MSG_VERBOSE );
			rpEntry->problemCount = 0;
		} else {
			rpEntry->problemCount++;
		}
		return FALSE;
	}

	if (PSRequestRemotePrinter( &rpEntry->spxHandle,
	rpEntry->printerStatus.printerNumber, &rprinterInfo ) == FAILURE) {
		if (rpEntry->problemCount >= WarnBeforeInform()) {
			if (rpEntry->spxHandle.errnoType == SPXET_USER &&
			rpEntry->spxHandle.errno == PSE_ALREADY_IN_USE)
				msgNum = RPMSG_RPRINTER_IN_USE;
			else
				msgNum = RPMSG_NO_PS_RPRINTER;
			Inform( rpEntry, msgNum, MSG_WARN );
			InformMsg( rpEntry, PSDisplayError( &rpEntry->spxHandle ),
				MSG_VERBOSE );
			rpEntry->problemCount = 0;
		} else {
			rpEntry->problemCount++;
		}

		SPXDisconnect( &rpEntry->spxHandle );
		return FALSE;
	}

	SPXDisconnect( &rpEntry->spxHandle );
	SPXSetSocketInAddress( (SPXAddress_ta *)rpEntry->pserverSPXAddress,
		rprinterInfo.socket );
	InformWithInt( rpEntry, RPMSG_PSRP_SOCKET,
		(long) rprinterInfo.socket, MSG_DEBUG );

	return TRUE;
}


static int
haveHostPrinterOpen(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->hostPrinterHandle.printerID < 0)
		RPOpenPrinter( rpEntry );

	return (rpEntry->hostPrinterHandle.printerID >= 0);
}


static int
havePServerRPConnection(
	RPrinterInfo_t *rpEntry)
{
	if (!rpEntry->spxConnection) {
		rpEntry->spxHandle.rprinterMode = TRUE;

		if (SPXConnect( &rpEntry->spxHandle, rpEntry->pserverSPXAddress,
		(SPXOptionsInfo_t *) 0 ) == FAILURE) {
			if (rpEntry->problemCount >= WarnBeforeInform()) {
				Inform( rpEntry, RPMSG_NO_SPX_RP_CONNECT, MSG_WARN );
				InformMsg( rpEntry, SPXDisplayErrno( &rpEntry->spxHandle ),
					MSG_VERBOSE );
				rpEntry->problemCount = 0;
			} else {
				rpEntry->problemCount++;
			}
			return FALSE;
		}

		rpEntry->spxConnection = TRUE;
	}

	return TRUE;
}


static int
isRPStatusSent(
	RPrinterInfo_t *rpEntry)
{
	if (rpEntry->spxConnection)
		RPSendStatus( rpEntry );

	return rpEntry->spxConnection;
}

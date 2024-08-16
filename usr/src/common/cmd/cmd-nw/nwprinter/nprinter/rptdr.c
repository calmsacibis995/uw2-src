/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/rptdr.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/rptdr.c,v 1.4 1994/08/18 16:27:19 vtag Exp $";
#endif

#include "rprinter.h"
#include "rpapi.h"
#include "nwprint_tdr.h"
#include "rptdr_proto.h"

void
TDRSendStatus(
	RPrinterStatus_t *printerStatus,
	uint8			  sendPacket[])
{
	uint8 *packetPtr;

	packetPtr = sendPacket;

	COPY_UINT8_FROM( printerStatus->printerNumber );
	COPY_UINT8_FROM( printerStatus->needBlocks );
	COPY_UINT8_FROM( printerStatus->finishedBlocks );
	COPY_UINT8_FROM( printerStatus->troubleCode );
	COPY_UINT8_FROM( printerStatus->inSideband );
}


void
TDRSidebandInfo(
	uint8		   packet[],
	PRTSideband_t *sidebandInfo)
{
	uint8 *packetPtr;

	packetPtr = packet;

	COPY_UINT8_TO( sidebandInfo->count );
	COPY_UINT8_TO( sidebandInfo->replicationChar );
	COPY_UINT8_TO( sidebandInfo->endChar );
}


void
TDRStartJobInfo(
	uint8			   packet[],
	int				   length,
	PRTStartJobInfo_t *startJobInfo)
{
	uint8 *packetPtr;

	packetPtr = packet;

	if (length > 0)
		COPY_UINT8_TO( startJobInfo->tabExpansion );
	else
		startJobInfo->tabExpansion = DEFAULT_TAB_EXPANSION;

	if (length > 1)
		COPY_UINT8_TO( startJobInfo->translation );
	else
		startJobInfo->translation = DEFAULT_TRANSLATION;
}

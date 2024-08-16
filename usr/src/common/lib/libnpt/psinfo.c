/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnpt:psinfo.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/psinfo.c,v 1.2 1994/03/24 18:34:10 aclark Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  PSInfo.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains routines to attach to and
 *				manipulate the print server.
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/

uint16
PSGetPrintServerInfo(connectID, serverInfo, reqSize)
	uint16		connectID;		/* SPX Connection number			*/
	PS_INFO		*serverInfo;	/* Server info structure			*/
	uint16		reqSize;		/* Size of information requested	*/
{
	uint16			ccode;
	NptRequest_t	request;
	NptReply_t		reply;


	dprintf("PSGetPrintServerInfo: id-%d\n",
		connectID);

	memset(&request, 0, sizeof(NptRequest_t));
	memset(&reply, 0, sizeof(NptReply_t));

	request.FunctionNumber = CMD_GET_PRINT_SERVER_INFO;

	ccode = GetSpxReply(connectID, &request, &reply);

	if (!ccode && serverInfo)
	{
		/*
			Set the returned fields
		*/
		if (reqSize > sizeof(PS_INFO))
			reqSize = sizeof(PS_INFO);
		else if (reqSize < sizeof(PS_INFO) && (reqSize > 6))
			reqSize = 6;

		switch (reqSize)
		{
		case sizeof(PS_INFO):
			serverInfo->serverType = reply.PSType;
			memcpy((char*)serverInfo->serialNumber,
				(char*)reply.SerialNumber, 4);
		case 6:
			serverInfo->revision = reply.RevisionNumber;
		case 5:
			serverInfo->minorVersion = reply.MinorVersionNumber;
		case 4:
			serverInfo->majorVersion = reply.MajorVersionNumber;
		case 3:
			serverInfo->numModes = reply.ServiceMode;
		case 2:
			serverInfo->numPrinters = reply.AttachedPrinters;
		case 1:
			serverInfo->status = reply.NptStatus;
		}
		ps_od(serverInfo, sizeof(*serverInfo));
	}

	return (ccode);
}

/********************************************************************/
/********************************************************************/

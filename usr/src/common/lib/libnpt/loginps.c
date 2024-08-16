/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnpt:loginps.c	1.4"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/loginps.c,v 1.3 1994/06/08 22:33:10 doug Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  LoginPS.c
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
PSLoginToPrintServer(PSconnectID, clientLevel)
	uint16		PSconnectID;	/* SPX Connection number			*/
	uint8		*clientLevel;	/* Client's access level			*/
{
	uint16			ccode;
	NptRequest_t	request;
	NptReply_t		reply;


	dprintf("PSLoginToPrintServer: id-%d\n", PSconnectID);

	memset(&request, 0, sizeof(NptRequest_t));
	memset(&reply, 0, sizeof(NptReply_t));

	/*
		Copy the request information
	*/
	request.FunctionNumber = CMD_LOGIN_TO_PRINT_SERVER;
	/*if (NWGetFileServerName( PSGetPreferredConnectionID(),
			(pnstr8)request.FileServerName)) {
		dprintf("NWGetServerPlatformName failed\n");
		return (-1);
	}
	if (NWGetConnectionNumber( PSGetPreferredConnectionID(),
			(NWCONN_NUM NWPTR)&request.ConnectionNumber)) {
		dprintf("NWGetClientConnID failed\n");
		return (-2);
	}*/

	ccode = GetSpxReply(PSconnectID, &request, &reply);

	/*
		Set the return fields
	*/
	if (clientLevel)
		*clientLevel = reply.AccessLevel;

	return (ccode);
}

/********************************************************************/
/********************************************************************/

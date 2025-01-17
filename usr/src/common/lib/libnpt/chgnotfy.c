/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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
#ident	"@(#)libnpt:chgnotfy.c	1.2"
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/chgnotfy.c,v 1.1 1994/02/11 23:05:10 eric Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  ChgNotfy.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains the ChangeNotifyInterval call
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/
/*
	Change Notify Interval
*/

uint16
PSChangeNotifyInterval(connectID, printerNumber, fserverName,
	objectName, objectType, firstNotice, notifyInterval)
	uint16		connectID;		/* SPX Connection number			*/
	uint8		printerNumber; 	/* Printer number					*/
	char		*fserverName;	/* File server name					*/
	char		*objectName;	/* Object name						*/
	uint16		objectType;		/* Object type						*/
	uint16		firstNotice;	/* First notify delay				*/
	uint16		notifyInterval;	/* Notify interval					*/
{
	uint16			ccode;
	NptRequest_t	request;


	dprintf("PSChangeNotifyInterval: id-%d pr-%d fs-%s\n",
		connectID, printerNumber, fserverName);
	dprintf("PSChangeNotifyInterval: ob-%s ty-%x 1-%d I-%d\n",
		objectName, objectType, firstNotice, notifyInterval);

	memset(&request, 0, sizeof(NptRequest_t));

	request.FunctionNumber = CMD_CHANGE_NOTIFY;
	request.PrinterNumber = printerNumber;

	if (fserverName)
	{
		ps_strupr(fserverName);
		strncpy((char*)request.FileServerName, (char*)fserverName,
			NWMAX_SERVER_NAME_LENGTH);
	}

	if (objectName)
	{
		ps_strupr(objectName);
		strncpy((char*)request.ObjectName, (char*)objectName,
			NWMAX_OBJECT_NAME_LENGTH);
	}

	request.ObjectType = objectType;
	request.FirstNotice = firstNotice;
	request.Interval = notifyInterval;

	ccode = GetSpxReply(connectID, &request, (NptReply_t *)NULL);

	return(ccode);
}

/********************************************************************/
/********************************************************************/

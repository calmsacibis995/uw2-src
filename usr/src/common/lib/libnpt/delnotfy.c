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
#ident	"@(#)libnpt:delnotfy.c	1.2"
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/delnotfy.c,v 1.1 1994/02/11 23:05:13 eric Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  DelNotfy.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains the DeleteNotifyObject routine
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/
/*
	Delete Notify Object
*/

uint16
PSDeleteNotifyObject(connectID, printerNumber, fserverName,
	objectName, objectType)
	uint16		connectID;		/* SPX Connection number			*/
	uint8		printerNumber; 	/* Printer number					*/
	char		*fserverName;	/* File server name					*/
	char		*objectName;	/* Object name						*/
	uint16		objectType;		/* Object type						*/
{
	uint16			ccode;
	NptRequest_t	request;


	dprintf("PSDeleteNotifyObject: id-%d pr-%d fs-%s ob-%s ty-%x\n",
		connectID, printerNumber, fserverName, objectName, objectType);

	memset(&request, 0, sizeof(NptRequest_t));

	request.FunctionNumber = CMD_DELETE_NOTIFY;
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

	ccode = GetSpxReply(connectID, &request, (NptReply_t *)NULL);

	return(ccode);
}

/********************************************************************/
/********************************************************************/

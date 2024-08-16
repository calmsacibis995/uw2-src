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
#ident	"@(#)libnpt:getprint.c	1.2"
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/getprint.c,v 1.1 1994/02/11 23:05:27 eric Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  GetPrint.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains GetPrintersServicingQueue
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/

uint16
PSGetPrintersServicingQueue(connectID, fserverName, queueName,
	maxPrinters, actualPrinters, printerArray)
	uint16		connectID;		/* SPX Connection number			*/
	char		*fserverName;	/* File server name					*/
	char		*queueName;		/* Queue name						*/
	uint8		maxPrinters;	/* Max number of printers to return	*/
	uint8		*actualPrinters;/* Actual number of printers returned*/
	uint8		*printerArray;	/* Array where printer numbs returned*/
{
	uint16			ccode;
	NptRequest_t	request;
	NptReply_t		reply;


	dprintf("PSGetPrintersServicingQueue: id-%d f-%s q-%s #-%d\n",
		connectID, fserverName, queueName, maxPrinters);

	memset(&request, 0, sizeof(NptRequest_t));
	memset(&reply, 0, sizeof(NptReply_t));

	request.FunctionNumber = CMD_GET_PRINTERS_FOR_QUEUE;
	if (fserverName)
	{
		ps_strupr(fserverName);
		strncpy((char*)request.FileServerName, (char*)fserverName,
			NWMAX_SERVER_NAME_LENGTH);
	}
	if (queueName)
	{
		ps_strupr(queueName);
		strncpy((char*)request.QueueName, (char*)queueName,
			NWMAX_QUEUE_NAME_LENGTH);
	}
	request.MaxNumberOfEntries = maxPrinters;

	/* Set up the rely packet */
	reply.ServicingQueueArray = printerArray;

	ccode = GetSpxReply(connectID, &request, &reply);

	if (actualPrinters)
		*actualPrinters = reply.NumPrinters;

	return(ccode);
}

/********************************************************************/
/********************************************************************/
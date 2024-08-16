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
#ident	"@(#)libnpt:jobstat.c	1.2"
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/jobstat.c,v 1.1 1994/02/11 23:05:33 eric Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  JobStat.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains the GetPrintJobStatus function
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/

uint16
PSGetPrintJobStatus(connectID, printerNumber, fserverName,
	queueName, jobNumber, jobDescription, printCopies,
	printSize, copiesFinished, bytesFinished, formNumber, textFlag)
	uint16		connectID;		/* SPX Connection number			*/
	uint8		printerNumber;	/* Printer number					*/
	char		*fserverName;	/* File server name					*/	
	char		*queueName;		/* Queue name						*/	
	uint16		*jobNumber;		/* Job number						*/
	char		*jobDescription;/* Description of job				*/
	uint16		*printCopies;	/* Number of copies to be printed	*/
	uint32		*printSize;		/* Size of print job				*/
	uint16		*copiesFinished;/* Copies finished		 			*/
	uint32		*bytesFinished;	/* Bytes into current copy			*/
	uint16		*formNumber;	/* Form number for job				*/
	uint8		*textFlag;		/* Is job text?						*/
{
	uint16			ccode;
	NptRequest_t	request;
	NptReply_t		reply;


	dprintf("PSGetPrintJobStatus: id-%d prnt-%d\n",
		connectID, printerNumber);

	memset(&request, 0, sizeof(NptRequest_t));
	memset(&reply, 0, sizeof(NptReply_t));

	request.FunctionNumber = CMD_GET_JOB_STATUS;
	request.PrinterNumber = printerNumber;

	ccode = GetSpxReply(connectID, &request, &reply);

	if (fserverName)
		strcpy((char*)fserverName, (char*)reply.FileServerName);
	if (queueName)
		strcpy((char*)queueName, (char*)reply.QueueName);
	if (jobNumber)
		*jobNumber = reply.JobQueueNumber;
	if (jobDescription)
		strcpy((char*)jobDescription, (char*)reply.JobDescription);
	if (printCopies)
		*printCopies = reply.CopiesInJob;
	if (printSize)
		*printSize = reply.CopySize;
	if (copiesFinished)
		*copiesFinished = reply.CopiesPrinted;
	if (bytesFinished)
		*bytesFinished = reply.BytesIntoCurrentCopy;
	if (formNumber)
		*formNumber = reply.FormNumber;
	if (textFlag)
		*textFlag = reply.TextByteStream;

	return(ccode);
}

/********************************************************************/
/********************************************************************/

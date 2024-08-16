/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnpt:pstatus.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/pstatus.c,v 1.2 1994/03/24 18:34:11 aclark Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  PStatus.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains routines to manipulate an
 *				active printer.
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/
/*
	Printer Control
*/

uint16
PSGetPrinterStatus(connectID, printerNumber, printerStatus,
	troubleCode, printerHasJob, serviceMode, mountedForm,
	formName, printerName)
	uint16		connectID;		/* SPX Connection number			*/
	uint8		printerNumber;	/* Printer number					*/
	uint8		*printerStatus;	/* Printer status					*/	
	uint8		*troubleCode;	/* On line/Off line/Out of paper	*/	
	uint8		*printerHasJob;	/* Printer has an active job		*/	
	uint8		*serviceMode;	/* New mode							*/
	uint16		*mountedForm;	/* Mounted form	number				*/
	char		*formName;		/* Mounted form name				*/
	char		*printerName;	/* Printer name						*/
{
	uint16			ccode;
	NptRequest_t	request;
	NptReply_t		reply;


	dprintf("PSGetPrinterStatus: id-%d pr-%d\n",
		connectID, printerNumber);

	memset(&request, 0, sizeof(NptRequest_t));
	memset(&reply, 0, sizeof(NptReply_t));

	request.FunctionNumber = CMD_GET_PRINTER_STATUS;
	request.PrinterNumber = printerNumber;

	ccode = GetSpxReply(connectID, &request, &reply);

	if (printerStatus)
		*printerStatus = reply.NptStatus;
	if (troubleCode)
		*troubleCode = reply.ErrorCode;
	if (printerHasJob)
		*printerHasJob = reply.ActiveJob;
	if (serviceMode)
		*serviceMode = reply.ServiceMode;
	if (mountedForm)
		*mountedForm = reply.FormNumber;
	if (formName)
		strcpy((char*)formName, (char*)reply.FormName);
	if (printerName)
		strcpy((char*)printerName, (char*)reply.PrinterName);

	return(ccode);
}

/********************************************************************/
/********************************************************************/

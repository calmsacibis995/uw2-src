/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/prtapi.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/prtapi.c,v 1.4 1994/08/18 16:27:14 vtag Exp $";
#endif


#include "rptune.h"
#include "prtapi.h"
#include "prt.h"
#include "rprinter.h"
#include "inform.h"
#include "prtjob_proto.h"
#include "prtconf_proto.h"


static PRTPrinterInfo_t *prtList[MAX_PRINTERS];


int
PRTClosePrinter(
	PRTHandle_t *printerHandle)
{
	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt(RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	free( prtList[printerHandle->printerID] );
	prtList[printerHandle->printerID] = (PRTPrinterInfo_t *) 0;
	printerHandle->printerID = -1;
	return SUCCESS;
}


int
PRTOpenPrinter(
	PRTHandle_t *printerHandle,
	char hostPrinterName[],
	int  printerNumber,
	char pserverName[])
{
	PRTPrinterInfo_t *prtEntry;

	prtEntry = (PRTPrinterInfo_t *)
		malloc( sizeof( PRTPrinterInfo_t ) );

	for (printerHandle->printerID = 0;
	prtList[printerHandle->printerID];
	printerHandle->printerID++)
		;

	prtList[printerHandle->printerID] = prtEntry;

	prtEntry->printerConfig.lastMtime = 0;
	strcpy( prtEntry->printerConfig.hostPrinterName, hostPrinterName );

	if (CheckPrinterConfiguration( &prtEntry->printerConfig )
	== FAILURE) {
		PRTClosePrinter( printerHandle );
		return FAILURE;
	}

	strncpy( prtEntry->pserverName, pserverName,
		MAX_PSERVER_NAME_LENGTH );
	prtEntry->pserverName[MAX_PSERVER_NAME_LENGTH - 1] = 0;
	prtEntry->printerNumber = printerNumber;
	prtEntry->printJob.status = PJ_STATUS_NO_JOB;
	prtEntry->printJob.bufType = BUFFER_TYPE_NULL;
	printerHandle->printerType =
		prtEntry->printerConfig.hostPrinterType;

	if (GetPrinterStatus( prtEntry, printerHandle,
	TEST_QUEUES_AND_QUEUE ) == FAILURE) {
		PRTClosePrinter( printerHandle );
		return FAILURE;
	}

	return SUCCESS;
}


int
PRTGetPrinterStatus(
	PRTHandle_t *printerHandle)
{
	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt(RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	return GetPrinterStatus( prtList[printerHandle->printerID],
		printerHandle, TEST_QUEUE_ONLY );
}


int
PRTSendDataToPrintJob(
	PRTHandle_t *printerHandle,
	uint8 data[],
	int dataLength)
{
	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt( RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	return WriteToPrintJob( prtList[printerHandle->printerID],
		(char *)data, dataLength, printerHandle );
}


int
PRTAbortPrintJob(
	PRTHandle_t *printerHandle)
{
	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt( RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	return AbortPrintJob( prtList[printerHandle->printerID], printerHandle);
}


int
PRTPausePrintJob(
	PRTHandle_t *printerHandle)
{
	return SUCCESS;
}


int
PRTUnpausePrintJob(
	PRTHandle_t *printerHandle)
{
	return SUCCESS;
}


int
PRTSidebandPrintJob(
	PRTHandle_t *printerHandle,
	PRTSideband_t *sidebandInfo)
{
	int dataLength;
	char *data;
	PRTPrinterInfo_t *prtEntry;

	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt(RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	prtEntry = prtList[printerHandle->printerID];
	prtEntry->printJob.tabExpansion = 8;
	prtEntry->printJob.translation = 0;

	if (StartNewPrintJob( prtEntry, printerHandle ) == SUCCESS) {
		data = (char *)malloc( sidebandInfo->count + 1 );
		for (dataLength = 0; dataLength < (int)(sidebandInfo->count);
		dataLength++)
			data[dataLength] = sidebandInfo->replicationChar;
		if (sidebandInfo->endChar)
			data[dataLength++] = sidebandInfo->endChar;
		WriteToPrintJob( prtEntry, data, dataLength, printerHandle );
		free( data );
		return EndPrintJob( prtEntry, printerHandle );
	}

	return FAILURE;
}


int
PRTStartNewPrintJob(
	PRTHandle_t *printerHandle,
	PRTStartJobInfo_t *startJobInfo)
{
	PRTPrinterInfo_t *prtEntry;

	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt( RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	prtEntry = prtList[printerHandle->printerID];
	prtEntry->printJob.tabExpansion = startJobInfo->tabExpansion;
	prtEntry->printJob.translation = startJobInfo->translation;

	GetPrinterStatus( prtEntry, printerHandle, TEST_QUEUE_ONLY );

	return StartNewPrintJob( prtEntry, printerHandle );
}


int
PRTEndPrintJob(
	PRTHandle_t *printerHandle)
{
	if (printerHandle->printerID < 0
	|| !prtList[printerHandle->printerID])
	{
		PRTInformWithInt( RPMSG_BAD_PRT_ID,
			(long) printerHandle->printerID, MSG_ERROR );
		return FAILURE;
	}

	return EndPrintJob( prtList[printerHandle->printerID],
		printerHandle );
}


int
PRTReleasePrinter(
	PRTHandle_t *printerHandle)
{
	return SUCCESS;
}


int
PRTReclaimPrinter(
	PRTHandle_t *printerHandle)
{
	return SUCCESS;
}


void
PRTInform(
	int		message,
	int		msgType)
{
	Inform(NULL, message, msgType);
}


void
PRTInformWithInt(
	int		message,
	long	msgInt,
	int		msgType)
{
	InformWithInt(NULL, message, msgInt, msgType);
}


void
PRTInformWithStr(
	int		message,
	char	auxStr[],
	int		msgType)
{
	InformWithStr(NULL, message, auxStr, msgType);
}

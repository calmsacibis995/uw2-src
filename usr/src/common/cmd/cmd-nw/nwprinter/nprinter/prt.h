/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/prt.h	1.3"
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
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/prt.h,v 1.2.2.1 1994/11/02 17:58:39 nick Exp $
 */

#ifndef __PRT_H__
#define __PRT_H__

#include <sys/types.h>
#include <sys/nwportable.h>

#define TEST_QUEUES_AND_QUEUE			0
#define TEST_QUEUE_ONLY					1

#define MAX_PSERVER_NAME_LENGTH			48
#define MAX_DESTINATION_NAME_LENGTH		255
#define MAX_FORM_ID_LENGTH				16
#define MAX_OUTFILE_PATH_LENGTH			128

#define PJ_STATUS_NO_JOB				0
#define PJ_STATUS_ACTIVE_JOB			1
#define PJ_STATUS_PAUSED_JOB			2
#define PJ_STATUS_RELEASED				3

#define BUFFER_TYPE_NULL				0
#define BUFFER_TYPE_NORMAL_DATA			1
#define BUFFER_TYPE_SIDEBAND_DATA		2

typedef struct {
	time_t	lastMtime;
	int		hostPrinterType;
	char	hostPrinterName[MAX_HOST_PRINTER_NAME_LENGTH];
	char	destinationName[MAX_DESTINATION_NAME_LENGTH];
	int		queuePriority;
	char	formID[MAX_FORM_ID_LENGTH];
} PRTConfig_t;

typedef struct {
	int			 status;
	PRTCount_ts	 tabExpansion;
	PRTCount_ts	 translation;
	char		 outFilePath[MAX_OUTFILE_PATH_LENGTH];
	int			 outFD;
	int			 bufType;
	char		*bufPtr;
} PRTJob_t;

typedef struct {
	char		 pserverName[MAX_PSERVER_NAME_LENGTH];
	int			 printerNumber;
	PRTConfig_t  printerConfig;
	PRTJob_t  	 printJob;
} PRTPrinterInfo_t;


int PRTClosePrinter(
	PRTHandle_t *printerHandle);
int PRTOpenPrinter(
	PRTHandle_t *printerHandle,
	char hostPrinterName[],
	int  printerNumber,
	char pserverName[]);
int PRTGetPrinterStatus(
	PRTHandle_t *printerHandle);
int PRTSendDataToPrintJob(
	PRTHandle_t *printerHandle,
	uint8	data[],
	int dataLength);
int PRTAbortPrintJob(
	PRTHandle_t *printerHandle);
int PRTPausePrintJob(
	PRTHandle_t *printerHandle);
int PRTUnpausePrintJob(
	PRTHandle_t *printerHandle);
int PRTSidebandPrintJob(
	PRTHandle_t *printerHandle,
	PRTSideband_t *sidebandInfo);
int PRTStartNewPrintJob(
	PRTHandle_t *printerHandle,
	PRTStartJobInfo_t *startJobInfo);
int PRTEndPrintJob(
	PRTHandle_t *printerHandle);
int PRTReleasePrinter(
	PRTHandle_t *printerHandle);
int PRTReclaimPrinter(
	PRTHandle_t *printerHandle);
void PRTInform(
	int		message,
	int		msgType);
void PRTInformWithInt(
	int		message,
	long	msgInt,
	int		msgType);
void PRTInformWithStr(
	int		message,
	char	auxStr[],
	int		msgType);
#endif


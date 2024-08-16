/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/prtjob.c	1.2"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/prtjob.c,v 1.1.2.1 1994/11/02 17:58:41 nick Exp $";
#endif

#include <stdio.h>
#include <fcntl.h>
#include "rprinter.h"
#include "prt.h"
#include "inform.h"
#include "prtjob_proto.h"

static void reset_expansion( PRTCount_ts tabWidth);
static int write_expanded( int fd, char *buf, int len);


#if defined(OS_AIX)
#define TEST_QUEUES_COMMAND	"lpstat >/dev/null 2>&1"
#define TEST_QUEUE_TEMPLATE	"lpstat -p%s >/dev/null 2>&1"
#define CLEANUP_NEEDED
#define ENQUEUE_TEMPLATE1	"lp -c -s -o -dp -d %s %s"
#endif

#if defined(OS_SUN4)
#define TEST_QUEUES_COMMAND	"lpq >/dev/null 2>&1"
#define TEST_QUEUE_TEMPLATE	"lpq -P%s >/dev/null 2>&1"
#define ENQUEUE_TEMPLATE1	"lpr -r -s -P%s %s > /dev/null 2>&1"
#endif

#ifndef TEST_QUEUES_COMMAND
#define TEST_QUEUES_COMMAND	"lpstat >/dev/null 2>&1"
#define TEST_QUEUE_TEMPLATE	"lpstat -p %s >/dev/null 2>&1"
#define CLEANUP_NEEDED
#define SUPPORTS_QPRI
#define SUPPORTS_FORMS
#define ENQUEUE_TEMPLATE1	"lp -c -s -d %s -q %d %s"
#define ENQUEUE_TEMPLATE2	"lp -c -s -d %s -q %d -f %s %s"
#endif


int
GetPrinterStatus(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle,
	int testType)
{
	char testQueue[300];

	sprintf( testQueue, TEST_QUEUE_TEMPLATE,
		prtEntry->printerConfig.destinationName );

	if (testType == TEST_QUEUES_AND_QUEUE) {
		if (system( TEST_QUEUES_COMMAND )) {
			Inform( NULL, RPMSG_NO_QUEUES, MSG_ERROR );
			return FAILURE;
		} else if (system( testQueue )) {
			InformPrinterWithStr(
					prtEntry->printerNumber,
					prtEntry->pserverName,
					RPMSG_BAD_DEST,
					prtEntry->printerConfig.destinationName,
					MSG_ERROR );
			return FAILURE;
		}
	} else {	 /* TEST_QUEUE_ONLY */
		if (system( testQueue ))
			printerHandle->printerStatus = PRINTER_STATUS_OFFLINE;
		else
			printerHandle->printerStatus = 0;
	}

	return SUCCESS;
}


int
StartNewPrintJob(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle)
{
	tmpnam( prtEntry->printJob.outFilePath );
	prtEntry->printJob.outFD = open( prtEntry->printJob.outFilePath,
		O_WRONLY | O_CREAT | O_EXCL, 0600 );
	reset_expansion(prtEntry->printJob.tabExpansion);

	return SUCCESS;
}


int
WriteToPrintJob(
	PRTPrinterInfo_t *prtEntry,
	char			  data[],
	int				  dataLength,
	PRTHandle_t		 *printerHandle)
{
	(void)write_expanded( prtEntry->printJob.outFD, data, dataLength );

	return SUCCESS;
}


int
EndPrintJob(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle)
{
	char enqueueCmd[250];
	PRTConfig_t *printConfig;
	PRTJob_t *printJob;

	printJob = &prtEntry->printJob;
	printConfig = &prtEntry->printerConfig;

	close( printJob->outFD );

#ifdef SUPPORTS_FORMS
	if (printConfig->formID[0])
		sprintf(enqueueCmd,
			ENQUEUE_TEMPLATE2,
			printConfig->destinationName,
#ifdef SUPPORTS_QPRI
			printConfig->queuePriority,
#endif
			printConfig->formID,
			printJob->outFilePath);
	else
#endif
		sprintf(enqueueCmd,
			ENQUEUE_TEMPLATE1,
			printConfig->destinationName,
#ifdef SUPPORTS_QPRI
			printConfig->queuePriority,
#endif
			printJob->outFilePath);

	(void)system( enqueueCmd );

#ifdef CLEANUP_NEEDED
	unlink( printJob->outFilePath );
#endif

	return SUCCESS;
}


int
AbortPrintJob(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle)
{
	close( prtEntry->printJob.outFD );
	unlink( prtEntry->printJob.outFilePath );

	return SUCCESS;
}


static int _e_tabstop = 0;
static int _e_column = 0;


static void
reset_expansion(
	PRTCount_ts tabWidth)
{
	_e_tabstop = (int) tabWidth;
	_e_column = 0;
}


#define flushbuf(l)	{ \
				if (write(fd, ebuf, (l)) < 0) \
					return(-1); \
				ebp = ebuf; \
			}

#define putbuf(c)	{ \
				if ((*ebp++ = (c)) == '\n') \
					_e_column = 0; \
				else \
					_e_column++; \
				if (ebp == ebufend) \
					flushbuf(sizeof(ebuf)); \
			}

static int
write_expanded(
	int fd,
	char *buf,
	int len)
{
	char *bp;
	char *bufend;
	char *ebp;
	static char ebuf[4096];
	static char *ebufend = &ebuf[sizeof(ebuf)];

	ebp = ebuf;
	bufend = &buf[len];
	for (bp = buf ; bp < bufend; bp++) {
		if (*bp == '\t' && _e_tabstop) {
			if ((_e_column % _e_tabstop) == 0)
				putbuf(' ');
			while (_e_column % _e_tabstop)
				putbuf(' ');
			continue;
		}
		putbuf(*bp);
	}
	flushbuf((int) (ebp - ebuf));
	return SUCCESS;
}

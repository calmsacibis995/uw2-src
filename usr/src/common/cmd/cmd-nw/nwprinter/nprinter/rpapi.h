/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/rpapi.h	1.1"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/rpapi.h,v 1.1 1994/02/11 18:24:50 nick Exp $
 */

#ifndef __RPAPI_H__
#define __RPAPI_H__

#define SEND_RPRINTER_STATUS_LENGTH			5
#define SIDEBAND_INFO_LENGTH				3

#define DEFAULT_TAB_EXPANSION				8
#define DEFAULT_TRANSLATION					0

/*
 * Data Stream Type (Command) codes
 */
#define RP_STATUS			0

#define RP_DATA				0
#define RP_FLUSH			1
#define RP_PAUSE			2
#define RP_START			3
#define RP_SIDEBAND			4
#define RP_NEW_JOB			5
#define RP_RELEASE			6
#define RP_RECLAIM			7
#define RP_EOJ				8

void
RPSendStatus(
	RPrinterInfo_t *rpEntry);
void
RPOpenPrinter(
	RPrinterInfo_t *rpEntry);
void
RPClosePrinter(
	RPrinterInfo_t *rpEntry);
void
RPGetPrinterStatus(
	RPrinterInfo_t *rpEntry);
void
RPSendDataToPrintJob( 
	RPrinterInfo_t *rpEntry,
	uint8			packet[],
	int				length);
void
RPAbortPrintJob(
	RPrinterInfo_t *rpEntry);

void
RPPausePrintJob(
	RPrinterInfo_t *rpEntry);

void
RPUnpausePrintJob(
	RPrinterInfo_t *rpEntry);

void
RPSendSidebandPrintJob(
	RPrinterInfo_t *rpEntry,
	uint8			packet[],
	int				length);

void
RPStartNewPrintJob(
	RPrinterInfo_t *rpEntry,
	uint8			packet[],
	int				length);

void
RPEndPrintJob(
	RPrinterInfo_t *rpEntry);

void
RPEndPrintJobTimeout(
	RPrinterInfo_t *rpEntry);

void
RPReleasePrinter(
	RPrinterInfo_t *rpEntry);

void
RPReclaimPrinter(
	RPrinterInfo_t *rpEntry);

#endif

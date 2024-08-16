/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/psapi.h	1.1"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/psapi.h,v 1.1 1994/02/11 18:24:42 nick Exp $
 */
#ifndef __PSAPI_H__
#define __PSAPI_H__

#define PSRP_REQUEST_REMOTE_PRINTER			0x81

#define REQUEST_RPRINTER_QUERY_LENGTH		2
#define REQUEST_RPRINTER_RESPONSE_LENGTH	22

#define INVALID_RESPONSE_PACKET_LENGTH		0x0001

#define PSE_NOT_ENOUGH_MEMORY				0x0301
#define PSE_NO_SUCH_PRINTER					0x0302
#define PSE_ALREADY_IN_USE					0x0308
#define PSE_DOWN							0x030C

typedef struct {
	uint16	printerType;
	uint16	useInterrupts;
	uint16	irqNumber;
	uint16	numBlocks;
	uint16	useXonXoff;
	uint16	baudRate;
	uint16	dataBits;
	uint16	stopBits;
	uint16	parity;
	uint16	socket;
} PSRPrinterInfo_t;

int PSRequestRemotePrinter(
	SPXHandle_t		 *spxHandle,
	uint8			  printerNumber,
	PSRPrinterInfo_t *rprinterInfo);
char * PSDisplayError( SPXHandle_t *spxHandle);
#endif


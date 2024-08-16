/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/prtjob_proto.h	1.1"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/prtjob_proto.h,v 1.1 1994/02/11 18:24:37 nick Exp $
 */
#ifndef __PRTJOB_PROTO_H__
#define __PRTJOB_PROTO_H__

int
GetPrinterStatus(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle,
	int type);
int
StartNewPrintJob(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle);
int
WriteToPrintJob(
	PRTPrinterInfo_t *prtEntry,
	char			  data[],
	int				  dataLength,
	PRTHandle_t		 *printerHandle);
int
EndPrintJob(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle);
int
AbortPrintJob(
	PRTPrinterInfo_t *prtEntry,
	PRTHandle_t		 *printerHandle);
#endif

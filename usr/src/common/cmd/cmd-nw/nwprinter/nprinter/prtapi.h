/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/prtapi.h	1.1"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/prtapi.h,v 1.1 1994/02/11 18:24:29 nick Exp $
 */
#ifndef __PRTAPI_H__
#define __PRTAPI_H__

#define HP_TYPE_DEVICE						0
#define HP_TYPE_QUEUE						1

#define PRINTER_STATUS_OFFLINE				0x00000001
#define PRINTER_STATUS_OUT_OF_PAPER			0x00000002

typedef unsigned long PRTStatus_ts;

typedef unsigned char PRTCount_ts;

typedef struct {
	int			 printerID;
	int			 printerType;
	PRTStatus_ts printerStatus;
} PRTHandle_t;

typedef struct {
	PRTCount_ts	count;
	char		replicationChar;
	char		endChar;
} PRTSideband_t;

#define PRT_TRANSLATION_NONE		0
#define PRT_TRANSLATION_JAPANESE	1
#define PRT_TRANSLATION_ENGLISH		2

typedef struct {
	PRTCount_ts	tabExpansion;
	PRTCount_ts	translation;
} PRTStartJobInfo_t;

#endif

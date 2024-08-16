/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/spxtdr.h	1.1"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/spxtdr.h,v 1.1 1994/02/11 18:18:51 nick Exp $
 */
#ifndef __SPXTDR_H__
#define __SPXTDR_H__
void TDRGetSocketFromSPXAddress(
	SPXAddress_ta spxAddress,
	uint16		 *socket);

void
TDRSetSocketInSPXAddress(
	SPXAddress_ta spxAddress,
	uint16		  socket);

void
TDRGetTCallOptionsInfo(
	uint8			  spxOpt[],
	SPXOptionsInfo_t *spxOptions);

void
TDRGetSPXHeader(
	uint8		 packet[],
	SPXIIHeader_t *spxHeader);

#endif

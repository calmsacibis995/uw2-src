/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/acct.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/acct.h 1.2 (Novell) 7/30/91
 */

/*
**
**	TDR structure used for accounting calls
**
*/
typedef struct acctInfo {
	char	objName[ NWMAX_OBJECT_NAME_LENGTH ];
	char	cmnt[ NWMAX_COMMENT_LENGTH ];
	uint8	objNameLength;
	uint8	cmntLength;
	uint16	objType;
	uint16	servType;
	uint16	cmntType;
	int32	acctBalance;
	int32	acctLimit;
	int32	chargeAmnt;
	int32	cancelHoldAmnt;
	int32	reserveAmnt;
	uint32	acctHolds[ NWMAX_NUMBER_OF_HOLDS ];
	NWHoldInfo_t	*holdPtr;
} acctInfo_t;





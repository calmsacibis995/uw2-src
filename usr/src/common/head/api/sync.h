/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/sync.h	1.1"
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
 *    include/api/sync.h 1.2 (Novell) 7/30/91
 */

/*
**
**	TDR structure used with synchronization calls
**
*/
typedef struct syncInfo {
	char	fileNm[ NWMAX_FILE_NAME_LENGTH ];
	char	logRecName[ NWMAX_LOGICAL_RECORD_NAME_LENGTH ];
	char	semaName[ NWMAX_SEMAPHORE_NAME_LENGTH ];
	int		semaVal;
	int		initialSemaVal;
	uint8	lockFlg;
	uint8	fileNmLength;
	uint8	logRecNameLength;
	uint8	semaNameLength;
	uint16	semaOpenCount;
	uint16	timeoutLimit;
	uint32	recStartOffset;
	uint32	recLength;
	uint32	semaHandle;
	NWFileHandle_ta	fileHndle;
	NWDirHandle_ts	dirHndle;
} syncInfo_t;



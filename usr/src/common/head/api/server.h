/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/server.h	1.1"
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
 *    include/api/server.h 1.2 (Novell) 7/30/91
 */

/*
**
**	TDR structure used with server platform calls
**
*/

typedef struct serverPlatformInfo {
	char	objName[ NWMAX_OBJECT_NAME_LENGTH ];
	char	serverName[ NWMAX_SERVER_NAME_LENGTH ];
	uint8	objNameLength;
	uint8	volNumber;
	uint8	majorVersNum;
	uint8	minorVersNum;
	uint8	revisionNum;
	uint8	SFTLvlSupported;
	uint8	TTSupported;
	uint8	loginAllowed;
	uint8	dayOfWeek;
	uint8	printServerVers;
	uint8	forceFlag;
	uint16	objType;
	uint16	dirCount;
	uint16	clusterCnt;
	uint16	fileCnt;
	uint16	maxConnSupported;
	uint16	peakConnUsed;
	uint16	connInUse;
	uint16	maxVolSupported;
	uint16	listLength;
	uint16	*connList;
	uint32	trsteeID;
	NWPrinterInfo_t					*printerData;
	NWDescriptionStrings_t			*serverDesc;
	NWServerPlatformInfo_t			*serverInformation;
	NWServerPlatformDateAndTime_t 	*serverDateAndTime;
} serverPlatformInfo_t;










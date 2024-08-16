/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/qms.h	1.1"
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
 *    include/api/qms.h 1.3 (Novell) 7/30/91
 */

/*
**
**	TDR structure used with qms calls
**
*/

typedef struct qmsInfo {
	uint8	serverStatusRec[ NWMAX_SERVER_STATUS_RECORD_LENGTH ];
	char	jStruct[ NWMAX_JOB_STRUCT_SIZE ];
	char	qName[ NWMAX_QUEUE_NAME_LENGTH ];
	char	qSubdir[ NWMAX_QUEUE_SUBDIR_LENGTH ];
	NWFileHandle_ta	fileHndle;
	uint8	newJobPos;
	uint8	jobID;
	uint8	dirHandle;
	uint8	qStatus;
	uint8	qSubdirLength;
	uint8	qNameLength;
	uint16	jobNum;
	uint16	qObjType;
	uint16	printerNum;
	uint16	numOfJobsInQ;
	uint16	numOfServers;
	uint16	qServerConnNum;
	uint16	targetJType;
	uint16	*listOfJobNums;
	uint16	serverConnNumList[ NWMAX_NUMBER_OF_SERVER_CONN_NUMBERS ];
	uint32	qID;
	uint32  newQID;
	uint32	fileSz;
	uint32	serverObjIDList[ NWMAX_NUMBER_OF_SERVER_OBJECT_IDS ];	
	uint32	qServerID;
	NWQueueJobStruct_t	*jStructPtr;
} qmsInfo_t;



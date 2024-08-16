/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnpt:libnpt.h	1.3"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/libnpt.h,v 1.2 1994/03/24 18:34:04 aclark Exp $
 */

/********************************************************************
 *
 * Program Name:	PServer - NetWare Print Server
 *
 * Filename:		LibNPT.H
 *
 * Date Created:	November 1, 1990
 *
 * Version:		  	1.00
 *
 * Programmers:		Joe Ivie
 *
 * Comments:		This file contains includes, defines, structures,
 *					variables and prototypes specific to the print
 *					services library
 *
 *					THIS CODE IS NOT PLATFORM INDEPENDENT
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

/*
	Includes specific to the library
*/
/* Portable includes */
#include <sys/types.h>		/* System variable types				*/
#include <string.h>			/* C strings							*/
#include <memory.h>			/* Memory Definitions					*/
#include <errno.h>			/* UNIX Errors							*/
#include <stdio.h>			/* Standard I/O calls					*/
#include <fcntl.h>			/* File Control flags					*/
#include <tiuser.h>			/* TLI definitions						*/
extern int 		t_errno;


#include <sys/ipx_app.h>		/* NetWare IPX interface definitions	*/
#include "nw/nwcalls.h"			/* NWCALLS API definitions			*/
#include "npt.h"			/* NetWare print services API			*/

/********************************************************************/
/*
	Global defines
*/
extern int			UniqueLibNptDebugFlag;

#undef dprintf
#undef od

#ifdef DEBUG
#define dprintf		if (UniqueLibNptDebugFlag) printf
#define od			if (UniqueLibNptDebugFlag) _od
#else
#define dprintf
#define od
#endif

/********************************************************************/
/*
	Global structures
*/
typedef struct NptRequest {
	uint8	FunctionNumber;			/* used by all packets */
	uint8	ByPage;
	char	Character;
	uint8	Immediately;
	uint8	JobOutcome;
	uint8	MaxNumberOfEntries;
	uint8	PrinterNumber;
	uint8	Priority;
	uint8	Relative;
	uint8	Sequence8;
	uint8	ServiceMode;
	uint8	Shared;
	uint16	ConnectionNumber;
	uint16	CopyNumber;
	uint16	FirstNotice;
	uint16	FormNumber;
	uint16	Interval;
	uint16	ObjectType;
	uint16	Sequence16;
	uint32	Offset;
	char	FileServerName[NWMAX_SERVER_NAME_LENGTH];
	char	ObjectName[NWMAX_OBJECT_NAME_LENGTH];
	char	QueueName[NWMAX_QUEUE_NAME_LENGTH];
	char	Password[NWMAX_PROPERTY_VALUE_LENGTH];
} NptRequest_t;

typedef struct NptReply {
	uint16	CompletionCode;			/* used by all packets */
	uint8	AccessLevel;
	uint8	ActiveJob;
	uint8	AttachedPrinters;
	uint8	ErrorCode;
	uint8	MajorVersionNumber;
	uint8	MinorVersionNumber;
	uint8	NumPrinters;
	uint8	PSType;
	uint8	PrinterNumber;
	uint8	Priority;
	uint8	RevisionNumber;
	uint8	Sequence8;
	uint8	ServiceMode;
	uint8	NptStatus;
	uint8	TextByteStream;
	uint16	BaudRate;
	uint16	Blocks;
	uint16	CopiesInJob;
	uint16	CopiesPrinted;
	uint16	DataBits;
	uint16	FirstNotice;
	uint16	FormNumber;
	uint16	IRQNumber;
	uint16	Interval;
	uint16	JobQueueNumber;
	uint16	ObjectType;
	uint16	ParityType;
	uint16	PrinterType;
	uint16	Protocol;
	uint16	Sequence16;
	uint16	Socket;
	uint16	StopBits;
	uint16	UseInterrupts;
	uint32	BytesIntoCurrentCopy;
	uint32	CopySize;
	uint8	SerialNumber[4];
	char	FormName[NWMAX_FORM_NAME_LENGTH];
	char	FileServerName[NWMAX_SERVER_NAME_LENGTH];
	char	QueueName[NWMAX_QUEUE_NAME_LENGTH];
	char	ObjectName[NWMAX_OBJECT_NAME_LENGTH];
	char	PrinterName[NWMAX_OBJECT_NAME_LENGTH];
	char	JobDescription[NWMAX_JOB_DESCRIPTION_LENGTH];
	uint8	*ServicingQueueArray;
} NptReply_t;

/********************************************************************/
/*
	Global functions
*/
#ifdef PROTO

uint16	OpenSpxConnection(ipxAddr_t*);
uint16	GetSpxReply(uint16, NptRequest_t*, NptReply_t*);
char	*strupr(char*);

#else

uint16	OpenSpxConnection();
uint16	GetSpxReply();
char	*strupr();

#endif	/* PROTO */

/********************************************************************/
/********************************************************************/


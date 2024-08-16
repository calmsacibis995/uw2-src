/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nwapi.h	1.2"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/headers/nwapi.h,v 1.2 1994/04/20 15:30:03 jodi Exp $
 */

/*
 *
 *	This is a header file to be included by the application program 
 *
 */

#ifndef _API_H_
#define _API_H_

#include "nwapidef.h"

/*
**	Engine layer structures
*/

/*************************
**	Fragmenter define
**************************/
#define NDS_MAX_FRAGS		5

typedef struct _NWADDR_LIST_ {
    NWTRAN  addr;
    nuint8  fsname[NWMAX_SERVER_NAME_LENGTH];
    struct _NWADDR_LIST_ *next;
} NWADDR_LIST;
 
/*
**	TDR structure used for connection calls
*/

typedef struct tagCONN_INFO
{
	pnuint8			serverName;
	pnuint8			objName;
	pnuint8			password;
	nuint8			objNameLength;
	nuint8			passwdLength;
	nuint32			objID;
	nuint32			connectionNumber;
	nuint32			objType;
	nint32			searchNum;
	nuint32			maxConnSupported;
	nuint32			numOfConnections;
	nuint32			listLen;
	nuint32			passKey1;
	nuint32			passKey2;
	nuint32			inUse;
	nuint32			connFlags;
	NWTRAN			*address;
	NWADDR_LIST		*addrList;
	pnuint32		pConnList;
} CONN_INFO, N_FAR * pCONN_INFO;




typedef struct
{
  nuint8  srcVolNum;
  nuint32 srcDirBase;
  nuint8  srcHandleFlag;
  nuint8  srcCompCount;
  nuint8  destVolNum;
  nuint32 destDirBase;
  nuint8  destHandleFlag;
  nuint8  destCompCount;
  nuint8  componentPath[520];  /* What's left of MAX packet size */
} PHSTRUCT;

#endif


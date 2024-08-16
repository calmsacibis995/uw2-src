/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/applets/nwreaddirectory.c	1.1"
/*
 * Copyright 1991, 1992, 1993, 1994 Unpublished Work of Novell, Inc.
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
 */

/**************************************************************************
	Program:	nwreaddirectory
	Author :	Rebekah R. Olson
	Date   :	10/15/94

	Program Description:
		 This program scans the specified directory and writes the filenames
		 out to stdout.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nw/nwcalls.h"
#include "nct.h"

void main(int argc, char *argv[])
{
	NWCCODE				ccode;
	char *				serverName;
	char *				netwarePathName;
	NWCONN_HANDLE		connID;
	SEARCH_FILE_INFO	searchInfo;
	nuint8				vol;
	nuint16				dirID;
	nuint8				accessRights;

	if ( argc != 3 ) {
		fprintf( stderr,"Usage: %s <serverName> <NetWarePathName>\n",argv[0]);
		exit( 1 );
	}

	serverName = argv[1];
	netwarePathName = argv[2];

	/*  Call the nct library routine to get a connection to the server
	*	for this app.  We assume that the calling program has already 
	*	opened a connection to the server with NWAttach/NWLogin.
	*/
	ccode = NWGetConnIDByName( serverName, &connID );
	if ( ccode ) {
		fprintf( stderr,"Unable to get connection to server %s\n", serverName);
		exit( 1 );
	}

	/* 
	*	Make sure that the path name is in upper case.  The servername is
	*   in upper case due to the NWGetConnIDByName.
	*/
	strtoupper( netwarePathName );

	/*
	*	Set up a scan of the directory.  I used NWFileSearchInit and 
	* 	NWIntFileSearchContinue since they work against all server 
	*	types. (Otherwise use NWIntScanDirEntryInfo call!) 
	*/
	searchInfo.sequenceNumber = (nuint16)-1;
	ccode = NWFileSearchInitialize( connID, 0, netwarePathName, &vol, 
									&dirID, &searchInfo.sequenceNumber, 
									&accessRights);
	if ( ccode ) {
		fprintf( stderr, "Initialize on Directory %s Failed. Error: 0x%X\n", 
				 netwarePathName,ccode);
		NWCloseConn( connID );
		exit( (nuint8)ccode );
	}

	while((ccode = NWIntFileSearchContinue( connID, vol, dirID, 
											searchInfo.sequenceNumber, 
											SA_NORMAL, "*.*", 
											(pnuint8)&searchInfo,
											USE_DOS_WILD_MATCH)) == 0) {
		/* 
		*	We found a file. Return the	fileName to stdout.
		*/
		printf( "%s/%s\n",netwarePathName, searchInfo.fileName);
	}

	if ( ccode != NO_FILES_FOUND_ERROR ) 
		fprintf( stderr, "Directory Scan on %s Failed. Error: 0x%X\n", 
				 netwarePathName,ccode);
	else 
		ccode = 0;
		
	NWCloseConn( connID );
	exit( (nuint8)ccode );
}

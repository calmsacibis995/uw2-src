/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/applets/nwrenamefile.c	1.1"
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
	Program:	nwrenamefile
	Author :	Rebekah R. Olson
	Date   :	10/14/94

	Program Description:
		This applet renames filename1 to filename2 on the specified NetWare
		server.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nw/nwcalls.h"
#include "nct.h"

void main(int argc, char *argv[])
{
	NWCCODE			ccode;
	char *			serverName;
	char *			originalPathName;
	char *			newPathName;
	NWCONN_HANDLE	connID;

	if (argc != 4) {
		fprintf(stderr,
			"Usage: %s <serverName> <OriginalPathName> <NewPathName>\n",
			argv[0]);
		exit(1);
	}

	serverName = argv[1];
	originalPathName = argv[2];
	newPathName = argv[3];


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
	*	Make sure both names are in upper case.  
	*/
	strtoupper( originalPathName );
	strtoupper( newPathName );

	/* 
	* Make the NetWare call to rename the file.  We are assuming full path
	* names on the rename.
	*/
	ccode = NWRenameFile( connID, 
						  0,   /* dirHandle field.  O since full path */
						  originalPathName, 
						  SA_NORMAL, 
						  0,   /* dirHandle field.  O since full path */
						  newPathName);
	if ( ccode ) 
		fprintf( stderr, "Error 0x%X Renaming File %s to %s\n", 
				 ccode, originalPathName, newPathName);
	NWCloseConn( connID );

	exit((nuint8)(ccode));
}

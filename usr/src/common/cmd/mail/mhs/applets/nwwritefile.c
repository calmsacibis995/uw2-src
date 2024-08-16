/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/applets/nwwritefile.c	1.1"
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
	Program:	nwwritefile
	Author :	Rebekah R. Olson
	Date   :	10/13/94

	Program Description:

	This program reads a file off of stdio and puts it on the specified
		NetWare server.  Two parameters specify the server and the filename
		of the file to be created.  A file will be created on the 
		server and the contents of stdin copied to that file.
	

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nw/nwcalls.h"
#include "nct.h"

void main(int argc, char *argv[])
{
	NWCCODE			ccode, ccode2;
	char *			serverName;
	char *			netwarePathName;
	NWCONN_HANDLE	connID;
	NWFILE_HANDLE	file;
	char			buffer[1024];
	int				sizeRead;

	if (argc != 3) {
		fprintf(stderr,"Usage: %s <serverName> <NetWarePathName>\n",argv[0]);
		exit(1);
	}

	serverName = argv[1];
	netwarePathName = argv[2];

	/*  
	*	Call the nct library routine to get a connection to the server
	*	for this app.  We assume that the calling program has already 
	*	opened a connection to the server with NWAttach/NWLogin.
	*/
	ccode = NWGetConnIDByName( serverName, &connID );
	if (ccode) {
		fprintf(stderr,"Unable to get connection to server %s\n", serverName);
		exit(1);
	}
	
	/* 
	* Make sure the path name is in upper case incase we have to delete the
	* file on an error condition.
	*/
	strtoupper( netwarePathName );

	/* 
	* Create the file with the overwrite flag (2) set.  The file will overwrite
	* a file that exists with the same name.
	*/
	ccode = NWCreateFile( connID, 0, netwarePathName, FA_NORMAL, 
							&file, NWOVERWRITE_FILE);
	if (ccode) {
		fprintf(stderr, "Unable to create file %s. Error: 0x%X\n", 
							netwarePathName,ccode);
		NWCloseConn( connID );
		exit( (nuint8)ccode );
	}
	
	/*
	* Read stdin for the file.  Read in blocks of 1024.  Continue 
	* to write the read info into the NetWare file.  If there is a 
	* problem with writing to the NetWare file, close the file and
	* then delete it.
	*/
	while (( sizeRead = read( STDIN_FILENO, buffer, sizeof(buffer))) != 0) {
		ccode = NWWriteFile( file, sizeRead, (pnuint8)buffer);
		if ( ccode ) {
			fprintf( stderr, "Error 0x%X Writing to file %s\n", 
					 ccode, netwarePathName);
			ccode2 = NWCloseFile( file );
			if ( ccode2 ) 
				fprintf( stderr,"Error 0x%X Closing File %s\n", ccode2,
						 netwarePathName);
			ccode2 = NWIntEraseFiles( connID, 0, netwarePathName, 
									  SA_ALL, USE_NW_WILD_MATCH);
			if ( ccode2 ) 
				fprintf( stderr, "Error 0x%X Deleting File %s\n", 
						 ccode2, netwarePathName);
			NWCloseConn( connID );
			exit( (nuint8)ccode );
		}
	}

	ccode = NWCloseFile( file );
	if ( ccode ) 
		fprintf( stderr,"Error 0x%X Closing File %s\n", ccode, netwarePathName);
	NWCloseConn( connID );
	exit( (nuint8)ccode );
}

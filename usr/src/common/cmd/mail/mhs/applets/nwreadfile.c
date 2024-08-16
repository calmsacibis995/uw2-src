/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/applets/nwreadfile.c	1.1"
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
	Program:	nwreadfile
	Author :	Rebekah R. Olson
	Date   :	10/13/94

	Program Description:

	This program reads a file off of the NetWare server and puts the file 
		on stdout.  Two parameters specify the server and the filename to 
		read.  The file will NOT be deleted after the read is done.

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
	nuint32			bytesToRead, bytesRead;
	int				writeSize;

	if ( argc != 3 ) {
		fprintf( stderr, "Usage: %s <serverName> <NetWarePathName>\n", argv[0]);
		exit(1);
	}

	serverName = argv[1];
	netwarePathName = argv[2];

	/*  Call the nct library routine to get a connection to the server
	*	for this app.  We assume that the calling program has already 
	*	opened a connection to the server with NWAttach/NWLogin.
	*/
	ccode = NWGetConnIDByName( serverName, &connID );
	if ( ccode ) {
		fprintf(stderr,"Unable to get connection to server %s\n", serverName);
		exit( 1 );
	}

	/* 
	*	Make sure that the path name is in upper case.  The servername is
	*   in upper case due to the NWGetConnIDByName.
	*/
	strtoupper( netwarePathName );

	/*
	*	Open the NetWare file.  If there is a failure, close the connection
	*	and return.
	*/
	ccode = NWOpenFile( connID, 0, netwarePathName, SA_NORMAL, AR_READ, &file );
	if (ccode) {
		fprintf( stderr, "Unable to open file %s. Error: 0x%X\n", 
				 netwarePathName,ccode);
		NWCloseConn( connID );
		exit( (nuint8)ccode );
	}
	
	/*
	* Initialize the number of bytes to read.  The buffer is currently 
	* 1024.  The max for the UNIX Requester is 1500 so I think this is 
	* a good packet size.  As well, through a router, the packet size 
	* will drop to 512.
	*/
	bytesToRead = sizeof(buffer);

	/*
	* Read the file.  Instead of keying off an error code on the read, 
	* we want to check the bytesRead.  If we ask for 512 and there are 
	* only 3 bytes left, the read will return 3. 
	*/
	do {
		ccode = NWReadFile( file, bytesToRead, &bytesRead, (pnuint8)buffer);
		if ( ccode ) {
			fprintf( stderr, "Unable to read file %s. Error: 0x%X\n", 
					 netwarePathName,ccode );
			ccode2 = NWCloseFile( file );
			if ( ccode2 ) 
				fprintf( stderr,"Failure Closing File %s, Error: 0x%X\n", 
						 netwarePathName, ccode2 );
			NWCloseConn( connID );
			exit( (nuint8)ccode );
		} else if ( bytesRead != 0 ) {
			/* 
			* Write the number of bytes read by the API to 
			* stdout.  Make sure that we wrote the correct number of
			* bytes.  If we don't, cleanup and exit.
			*/
			writeSize = write( STDOUT_FILENO, buffer, bytesRead );
			if ( writeSize != bytesRead ) {
				/*fprintf( stderr,"Error Writing to STDOUT\n" );*/
				ccode = NWCloseFile( file );
				if ( ccode ) 
					fprintf( stderr,"Error Closing File %s, Error: 0x%X\n", 
							 netwarePathName, ccode );
				NWCloseConn( connID );
				exit( 1 );
			}
		}
	} while (bytesRead == bytesToRead);
			
	ccode = NWCloseFile( file );
	if ( ccode ) 
		fprintf( stderr,"Error Closing File %s, Error: 0x%X\n", 
				 netwarePathName, ccode );
	NWCloseConn( connID );
	exit( (nuint8)ccode );
}

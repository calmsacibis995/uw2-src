/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:filerdwr.c	1.7"
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
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/filerdwr.c,v 1.10 1994/09/30 22:56:33 rebekah Exp $";
#endif

/***********************************************************************
 *
 * Program Name:
 *
 * Filename:	  fileio.c
 *
 * Date Created:  February 1, l990
 *
 * Version:	  1.0
 *
 * Modifications: (What, When, Who)
 *	Ported to NWU, 4/92, Mark Worwetz
 *	Changed to fit Cross-Platform spec, 8/21/92, Mark Worwetz
 *	Changed again to fit NWCALLS 10/29/93, Mark Worwetz (still!)
 *
 * Comments:
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 **********************************************************************/

#include "nwclient.h"
#include "ntypes.h"
#include "nwaccess.h"

#include "ncpfile.h"
#include "nwnamspc.h"
#include "nwfile.h"

#define FIRST_WRITE_BUFFER_LENGTH                   512
#define FIRST_READ_BUFFER_LENGTH                    512
#ifndef MIN
#define MIN( x, y )  ( ( x < y ) ? ( x ) : ( y ) )
#endif
#define MOD4K( x )	( x % 4096 )

/* Local function declarations */
N_INTERN_FUNC_C (nuint16) NWGetNegotiatedBufferSize( pNWAccess, pnuint32 );


/*
** BEGIN_MANUAL_ENTRY (NWReadFile, api/file/readfile)
** END_MANUAL_ENTRY
*/

NWCCODE N_API NWReadFile
(
   NWFILE_HANDLE  fileHandle,
	nuint32		   bytesToRead,
	pnuint32		   bytesActuallyRead,
	pnuint8		   data )
{
	NWCCODE			ccode;
   NWCONN_HANDLE  conn;
   nuint8         abuNWHandle[6];
	char			   *bufPtr;
	char			   buffer[ FIRST_READ_BUFFER_LENGTH ];
	nuint32			oddOffset;
	nuint32			maxSize;
	nuint32			totalBytesLeft;
	nuint32			endOffset;
	nuint32			endBndry;
	nuint32			firstByteCount;
	nuint16			tmpBytesRead;
	nuint32			nextByteCount;
   nuint32        startingOffset;
	NWCDeclareAccess(access);

   ccode = NWConvertFileHandle(fileHandle, 6, abuNWHandle, &conn);
   if (ccode)
      return(ccode);
	NWCSetConn(access, conn);

   ccode = NWGetFilePos(fileHandle, &startingOffset);

/*
**	Initialize errno and return values
*/
	ccode = 0;

	ccode = NWGetNegotiatedBufferSize( &access, &maxSize );
	if(ccode)
		return( ccode );

/*
** When reading from a file, if the starting byte offset
** begins on an odd byte, the first byte returned in the
** buffer will be garbage.
** oddOffset will be either a 1 or a 0, depending on whether
** the offset is odd.  If it is odd, we want to read an odd
** amount, so that the next offset will be even (and this
** problem eliminated).	 We, therefore, subtract the oddOffset
** from the buffer length.
*/
	oddOffset = startingOffset & 1;
	firstByteCount = (nuint32)MIN( bytesToRead,
						 (nuint32)( FIRST_READ_BUFFER_LENGTH - oddOffset ) );
	endOffset = startingOffset + firstByteCount;
	endBndry = MOD4K( endOffset );

/*
** NetWare doesn't allow reads that cross a 4K boundary
** block.  The calculation below determines if this is the
** case; if so we read up to the 4K boundary and make a second
** fileSys for the rest of the read.
*/
	if( (int)endBndry > 0 && (int)endBndry < MOD4K( startingOffset ) )
	{
		firstByteCount = firstByteCount - endBndry;
	}

/*
**	Send NCP Service Request
*/
	ccode = NWNCP72FileRead(&access,
                          0,
                          abuNWHandle,
                          startingOffset,
                          (nuint16)(firstByteCount + oddOffset),
                          &tmpBytesRead,
                          (pnuint8)buffer);

	if( ccode )
		return( ccode );

	if ( tmpBytesRead < firstByteCount + oddOffset )
		*bytesActuallyRead = tmpBytesRead;
	else
		*bytesActuallyRead = tmpBytesRead - oddOffset;

/*
** The first byte of the reply.dataBuffer will be garbage if
** the starting offset is odd.	oddOffset will be either 1 or 0,
** depending on whether the starting offset is odd, we
** therefore, copy the buffer starting at the oddOffset.
*/

	(void) NWCMemCpy( (char *)data, buffer + oddOffset, *bytesActuallyRead );

/*
** If less than the requested number of bytes was read,
** assume that the end of the file has been reached and
** return.
*/
	if( *bytesActuallyRead < firstByteCount )
	{
      goto FinishedRead;
	}

	totalBytesLeft = bytesToRead - ( nuint32 )firstByteCount;

/*
** To maximize speed (by minimizing copies) any additional
** reads will be read directly into the user's space.
** However, the buffer will also be copied into the user's
** space.  bufPtr, therefore, must be adjusted for the header
** length.
*/
	bufPtr = (char *)data + firstByteCount;
	startingOffset = startingOffset + firstByteCount;

	while( (int)totalBytesLeft > 0 ) {
		nextByteCount = (nuint32)MIN( totalBytesLeft, maxSize );
		endOffset = startingOffset + nextByteCount;
		endBndry = MOD4K( endOffset );
		if( (int)endBndry > 0 && (int)endBndry < MOD4K( startingOffset )){
			nextByteCount = nextByteCount - endBndry;
		}

		ccode = NWNCP72FileRead(&access,
                            0,
                            abuNWHandle,
                            startingOffset,
                            (nuint16)nextByteCount,
                            &tmpBytesRead,
                			(pnuint8)bufPtr);
		if(ccode)
			return( ccode );

		*bytesActuallyRead = *bytesActuallyRead + tmpBytesRead;

/*
** If less than the requested number of bytes was read,
** assume that the end of the file has been reached and
** return.
*/
/*
** Return the user's data to its original place.
*/
		if( tmpBytesRead < nextByteCount ) {
         	goto FinishedRead;
		}
		totalBytesLeft = totalBytesLeft - nextByteCount;
		startingOffset = startingOffset + nextByteCount;
		bufPtr = bufPtr + nextByteCount;
	}

FinishedRead:
   	NWSetFilePos(fileHandle, SEEK_FROM_CURRENT_OFFSET, *bytesActuallyRead);
	return ( 0 );
}


/*
** BEGIN_MANUAL_ENTRY (NWWriteFile, api/file/writefil.e)
** END_MANUAL_ENTRY
*/

NWCCODE N_API NWWriteFile
(
    NWFILE_HANDLE   fileHandle,
	nuint32			bytesToWrite,
	pnuint8			data )
{
	NWCONN_HANDLE	conn;
	NWCCODE			ccode = 0;
    nuint8          abuNWHandle[6];
	char			*bufPtr;
	char			buffer[ FIRST_WRITE_BUFFER_LENGTH ];
	nuint32			maxSize;
	nuint32			totalBytesLeft;
	nuint32			endOffset;
	nuint32			endBndry;
	nuint32			firstByteCount;
	nuint32			nextByteCount;
    nuint32    		startingOffset;
	NWCDeclareAccess(access);

   ccode = NWConvertFileHandle(fileHandle, 6, abuNWHandle, &conn);
   if (ccode)
		return(ccode);

   NWCSetConn(access, conn);

   ccode = NWGetFilePos(fileHandle, &startingOffset);

/*
**	Initialize the return value and the nwerrno
*/
	ccode = NWGetNegotiatedBufferSize( &access, &maxSize );
	if(ccode)
		return( ccode );

/*
** When writing to a file, if the starting byte offset
** begins on an odd byte, the first byte sent in the
** buffer will be ignored.
** oddOffset will be either a 1 or a 0, depending on whether
** the offset is odd.  If it is odd, we want to read an odd
** amount, so that the next offset will be even (and this
** problem eliminated).	 We, therefore, subtract the oddOffset
** from the buffer length.
*/
	firstByteCount = (nuint32)MIN( bytesToWrite,
					(nuint32)( FIRST_WRITE_BUFFER_LENGTH ) );
	endOffset = startingOffset + firstByteCount;
	endBndry = MOD4K( endOffset );

/*
** NetWare doesn't allow writes that cross a 4K boundary
** block.  The calculation below determines if this is the
** case; if so we write up to the 4K boundary and make a second
** request for the rest of the write.
*/
	if( (int)endBndry > 0 && (int)endBndry < MOD4K( startingOffset ) ) {
		firstByteCount = firstByteCount - endBndry;
	}

/*
** oddOffset will be either 1 or 0, depending on whether
** the starting offset is odd.	Therefore, we want to copy
** the data from the buffer at the header length plus the
** oddOffset.
*/
	(void) NWCMemCpy( buffer, (char *)data, firstByteCount );

/*
**	Send NCP Service Request
*/
	ccode = NWNCP73FileWrite(&access,
                           0,
                           abuNWHandle,
                           startingOffset,
                           (nuint16)firstByteCount,
                           (pnuint8)buffer);
	if( ccode )
		return( ccode );

	totalBytesLeft = bytesToWrite - firstByteCount;

/*
** To maximize speed (by minimizing copies) any additional
** writes will be written directly from the user's space.
** bufPtr, therefore, must be adjusted for the header
** length.
*/
	/* header taken care of in kernel space*/
	bufPtr = (char *)data + firstByteCount;
	startingOffset = startingOffset + firstByteCount;

	while( (int)totalBytesLeft > 0 )
	{
		nextByteCount = (nuint32)MIN(totalBytesLeft, maxSize);
		endOffset = startingOffset + nextByteCount;
		endBndry = MOD4K(endOffset);
		if((int)endBndry > 0 && (int)endBndry < MOD4K(startingOffset))
      {
			nextByteCount = nextByteCount - endBndry;
		}

		ccode = NWNCP73FileWrite(&access,
                             0,
                             abuNWHandle,
                             startingOffset,
                             (nuint16)nextByteCount,
                             (pnuint8)bufPtr);
		if( ccode )
			return( ccode );

		totalBytesLeft = totalBytesLeft - nextByteCount;
		startingOffset = startingOffset + nextByteCount;
		bufPtr = bufPtr + nextByteCount;
	}

   NWSetFilePos(fileHandle, SEEK_FROM_CURRENT_OFFSET, bytesToWrite);
	return( 0 );
}


/***********************
 **	LOCAL FUNCTIONS
 ***********************/
/****************************************************************************
 **	   FUNCTION:  NWGetNegotiatedBufferSize
 **	DESCRIPTION:  Gets the negotiated buffer size from the connection table.
 ****************************************************************************/
N_INTERN_FUNC_C (nuint16)
NWGetNegotiatedBufferSize
(
	pNWAccess	pAccess,
	pnuint32	maxSize
)
{
	nuint16			ccode;


/*
**	Make a NWCLIENT request to get connections max buffer size.
*/
	ccode = NWCGetConnInfo(pAccess,
                         NWC_CONN_INFO_MAX_PACKET_SIZE,
                         sizeof(nuint),
                         (nptr)maxSize);

	return( ccode );
}



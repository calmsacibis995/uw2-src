/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:examsem.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwsync.h"
#include "nwmisc.h"

/*manpage*NWExamineSemaphore************************************************
SYNTAX:  NWCCODE N_API NWExamineSemaphore
			(
				NWCONN_HANDLE conn,
				nuint32  semHandle,
				pnint16  semValue,
				pnuint16 semOpenCount
			)

REMARKS: This call returns the current value of the target semaphore.  Semaphore
			Open Count is the number of clients that are using the semaphore.  Semaphore
			Value is the current value of the semaphore.  Semaphore Value is discussed
			in the Wait On Semaphore and Signal Semaphore calls.


ARGS: <> conn
		 > semHandle
		 < semValue
		 < semOpenCount

INCLUDE: nwsync.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     32 01  Examine Semaphore

CHANGES: 19 Jun 1993 - modified - jwoodbur
			removed intelisms to increase portability
			30 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
			Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWExamineSemaphore
(
	NWCONN_HANDLE conn,
	nuint32       semHandle,
	pnint16       semValue,
	pnuint16      semOpenCount
)
{
   NWCCODE ccode;
	nuint8 tmpOpenCount, tmpSemVal;
	NWCDeclareAccess(access);

	NWCSetConn(access, conn);

	if ((ccode = (NWCCODE)NWNCP32s1SyncSemExamine( &access, semHandle,
				&tmpSemVal, &tmpOpenCount)) == 0)
	{
		if(semValue!=NULL)
			*semValue = (nint16) tmpSemVal;

		if(semOpenCount!=NULL)
			*semOpenCount = (nuint16) tmpOpenCount;
	}

	return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/examsem.c,v 1.7 1994/09/26 17:45:25 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rellgrec.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY   /* only use the locks part of nwfile.h */
#include "nwfile.h"
#include "nwmisc.h"

/*manpage*NWReleaseLogicalRecord********************************************
SYNTAX:  NWCCODE N_API NWReleaseLogicalRecord
         (
            NWCONN_HANDLE  conn,
            pnstr8         logicalRecordName
         )

REMARKS: Releases a lock on a logical record at the
         specified file server, but will not remove it from the log table.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or
         unlocking a logical record does not physically lock or unlock
         those resources associated with the logical record since only the
         applications using the record know about such an association.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     12 --  Release Logical Record

CHANGES: 30 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReleaseLogicalRecord
(
   NWCONN_HANDLE   conn,
   pnstr8          logicalRecordName
)
{
  nuint8 recNameLen;
  NWCCODE ccode;
  NWCDeclareAccess(access);

  NWCSetConn(access, conn);

  recNameLen = (nuint8) NWCStrLen(logicalRecordName);
  if(recNameLen > 128)
     recNameLen = 128;

  ccode = (NWCCODE) NWNCP12SyncRelLogRec (&access, recNameLen,
                     logicalRecordName);
#ifdef N_PLAT_OS2
  if(ccode)
    return (ccode);
  return(NWClearLockSemList(1));
#else
  return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rellgrec.c,v 1.7 1994/09/26 17:48:56 rebekah Exp $
*/

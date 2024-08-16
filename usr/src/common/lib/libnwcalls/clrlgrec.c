/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrlgrec.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of file*/
#include "nwfile.h"

/*manpage*NWClearLogicalRecord**********************************************
SYNTAX:  NWCCODE N_API NWClearLogicalRecord
         (
            NWCONN_HANDLE  conn,
            pnstr8         logRecName
         );

REMARKS: Clear the lock associated with the specified logical record.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or
         unlocking a logical record does not physically lock or unlock
         those resources associated with the logical record since only the
         applications using the record know about such an association.

ARGS: >  conn
      >  logRecName

INCLUDE: nwfile.h

RETURN:  0x0000  Successful
         0x89FF  Unlock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     11 --  Clear Logical Record

CHANGES: 27 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWClearLogicalRecord
(
   NWCONN_HANDLE  conn,
   pnstr8         logRecName
)
{
   NWCCODE ccode;
   nuint8 len;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   len = (nuint8) NWCStrLen(logRecName);
   if (len > 128)
      len = (nuint8) 128;
   ccode = (NWCCODE) NWNCP11SyncClrLogRec(&access, len,
                        (pnuint8) logRecName);

#ifdef N_PLAT_OS2
   if(ccode)
      return (ccode);
   return(NWClearLockSemList(1));
#else
   return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrlgrec.c,v 1.7 1994/09/26 17:44:31 rebekah Exp $
*/


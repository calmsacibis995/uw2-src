/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrlgset.c	1.5"
#include "nwcaldef.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of file*/
#include "nwfile.h"

/*manpage*NWClearLogicalRecordSet*******************************************
SYNTAX:  NWCCODE N_API NWClearLogicalRecordSet
         (
            void
         );

REMARKS: Clears the lock on a set of logical records.

         A set of logical records is created by logging these logical
         records with a call to NWLogLogicalRecord.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or
         unlocking a logical record does not physically lock or unlock
         those resources associated with the logical record since only the
         applications using the record know about such an association.

         Because a set of logical records may span more than one file
         server, this function clears the current set on all filer servers
         to which the workstation is attached.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     14 --  Clear Logical Record Set

CHANGES: 30 Aug 1993 - purposely NOT NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWClearLogicalRecordSet
(
   void
)
{
   nuint8  reqBuf[1];
   NWCCODE ccode;
   NW_FRAGMENT reqFrag[1];

   reqBuf[0] = 0;  /* Clear all sets */

   reqFrag[0].fragAddress = reqBuf;
   reqFrag[0].fragSize = 1;

   ccode = NWRequestToAll(14, 1, reqFrag);

#ifdef N_PLAT_OS2
   if(ccode)
      return (ccode);
   return (NWClearLockSemList(1));
#else
   return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrlgset.c,v 1.7 1994/09/26 17:44:32 rebekah Exp $
*/

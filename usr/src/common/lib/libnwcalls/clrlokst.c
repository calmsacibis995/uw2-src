/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrlokst.c	1.5"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of file*/
#include "nwfile.h"

/*manpage*NWClearFileLockSet************************************************
SYNTAX:  NWCCODE N_API NWClearFileLockSet
         (
            void
         );

REMARKS: This function unlocks all files identified in a log table, and
         removes them from the log table.

         In order to avoid deadlock, a workstation is required to request
         those files it will need to lock, and it does so by making an entry
         into a log table at the file server.  Once the log table is
         complete, the application can then lock those files.  The locking
         will only work if all files in the table are available.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     08 --  Clear File Set

CHANGES: 30 Aug 1993 - purposly NOT NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWClearFileLockSet
(
   void
)
{
   nuint8  reqBuf[1];
   NWCCODE ccode;
   NW_FRAGMENT reqFrag[1];

   reqBuf[0] = 0;  /* Clear all locked sets */

   reqFrag[0].fragAddress = reqBuf;
   reqFrag[0].fragSize = 1;

   ccode = NWRequestToAll(8, 1, reqFrag);

#ifdef N_PLAT_OS2
   if(ccode)
      return (ccode);
   return(NWClearLockSemList(2));
#else
   return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrlokst.c,v 1.7 1994/09/26 17:44:33 rebekah Exp $
*/

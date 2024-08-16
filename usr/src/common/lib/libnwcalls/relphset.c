/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:relphset.c	1.5"
#include "nwcaldef.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY        /* only use the locks part of nwfile.h */
#include "nwfile.h"

/*manpage*NWReleasePhysicalRecordSet****************************************
SYNTAX:  NWCCODE N_API NWReleasePhysicalRecordSet
         (
            void
         )

REMARKS: Unlocks all physical records currently locked in the log table of
         the requesting workstation, but does not remove them from the table.

         A physical record lock, as opposed to a logical lock, is the actual
         lock of a specified record relative to a          physical file.
         When a record is locked, it is also entered into a log table.
         Records are allowed to be locked only if all records in the log
         table are available for locking.  This is done to avoid deadlock.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     29 --  Release Physical Record Set

CHANGES: 21 Sep 1993 - purposely NOT NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReleasePhysicalRecordSet
(
   void
)
{
   nuint8 abuReq[1];
   NWCCODE ccode;
   NW_FRAGMENT reqFrag[1];

   abuReq[0] = 0;  /* Release all locked sets */

   reqFrag[0].fragAddress = abuReq;
   reqFrag[0].fragSize    = 1;

   ccode = NWRequestToAll(29, 1, reqFrag);
#ifdef N_PLAT_OS2
   if(ccode)
      return (ccode);
   return (NWClearLockSemList(0));
#else
   return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/relphset.c,v 1.7 1994/09/26 17:49:01 rebekah Exp $
*/

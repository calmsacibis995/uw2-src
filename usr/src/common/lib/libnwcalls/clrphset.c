/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrphset.c	1.5"
#include "nwcaldef.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of file*/
#include "nwfile.h"

/*manpage*NWClearPhysicalRecordSet******************************************
SYNTAX:  NWCCODE N_API NWClearPhysicalRecordSet
         (
            void
         )

REMARKS: Unlocks all physical records identified in a log table and removes
         them from the log table.

         The log table resides on the file server and is associated
         exclusively with the requesting task on the workstation.

         NetWare ignores this call if the workstation attempts to unlock and
         clear physical records that are not locked or logged.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     31 --  Clear Physical Record Set

CHANGES: 30 Aug 1993 - purposely NOT NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWClearPhysicalRecordSet
(
  void
)
{
   nuint8       abuReq[1];
   NWCCODE      ccode;
   NW_FRAGMENT  reqFrag[1];

   abuReq[0] = 0;  /* Clear all locked sets */

   reqFrag[0].fragAddress = abuReq;
   reqFrag[0].fragSize    = 1;

   ccode = NWRequestToAll(31, 1, reqFrag);

#ifdef N_PLAT_OS2
   if (ccode)
      return (ccode);

   return (NWClearLockSemList(0));

#else
   return (ccode);

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrphset.c,v 1.7 1994/09/26 17:44:35 rebekah Exp $
*/

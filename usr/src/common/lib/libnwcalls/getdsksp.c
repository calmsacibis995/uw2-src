/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdsksp.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwbindry.h"

#include "ncpbind.h"
#include "nwcaldef.h"

/*manpage*NWGetObjectDiskSpaceLeft******************************************
SYNTAX:  NWCCODE N_API NWGetObjectDiskSpaceLeft
         (
            NWCONN_HANDLE  conn,
            nuint32        objID,
            pnuint32       systemElapsedTime,
            pnuint32       unusedDiskBlocks,
            pnuint8        restrictionEnforced
         )

REMARKS: This call returns the number of unused disk blocks available to the
         specified Object ID.  A client can only obtain the information for the
         Object ID by which that client logged in; clients must have console
         operator rights to make this call for any other Object ID.

         The Object ID indicates which Object ID requested the information.

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and
         is used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, the value
         wraps back to zero.

         Configured Max Open Files contains the number of files the server can
         open simultaneously.

         Unused Disk Blocks indicates how many blocks the file server has
         available to allocate to a bindery object.

         Restrictions Enforced indicates whether the file server operating
         system can place limitations on disk resources (0x00 = enforced;
         0xFF = not enforced).

ARGS: >  conn
      >  objID
      <  systemElapsedTime (optional)
      <  unusedDiskBlocks (optional)
      <  restrictionEnforced (optional)

INCLUDE: nwbindry.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 230  Get Object's Remaining Disk Space

CHANGES: 25 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetObjectDiskSpaceLeft
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   pnuint32       systemElapsedTime,
   pnuint32       unusedDiskBlocks,
   pnuint8        restrictionsEnforced
)
{
   nuint32 unused;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE) NWNCP23s230GetObjFreeDiskSpace(&access,
         NSwap32(objID), systemElapsedTime, NULL, &unused,
         restrictionsEnforced)) == 0)
   {
      if (unusedDiskBlocks)
         *unusedDiskBlocks = unused;
   }
   else  /* error */
   {
      if (systemElapsedTime)
         *systemElapsedTime = 0;

      if (unusedDiskBlocks)
         *unusedDiskBlocks = 0;

      if (restrictionsEnforced)
         *restrictionsEnforced = 0;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdsksp.c,v 1.7 1994/09/26 17:45:55 rebekah Exp $
*/

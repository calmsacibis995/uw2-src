/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gdskrest.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwvol.h"

/*manpage*NWGetObjDiskRestrictions******************************************
SYNTAX:  NWCCODE N_API NWGetObjDiskRestrictions
         (
            NWCONN_HANDLE  conn,
            nuint8         volNum,
            nuint32        objID,
            pnuint32       restriction,
            pnuint32       inUse
         )

REMARKS: Returns the disk restrictions on a volume for
         the specified bindery object. The restrictions are returned in
         units of 4K blocks.  The inUse number returns the number of blocks
         currently in use by the object.

         NOTE: if restriction is greater than 0x4000 0000 the object has no
         restrictions.

ARGS: >  conn
      >  volNum
      >  objID
      <  restriction (optional)
      <  inUse (optional)

INCLUDE: nwvol.h

RETURN:  0x0000 Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 41  Get Object Disk Usage and Restrictions

CHANGES: 14 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetObjDiskRestrictions
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  nuint32         objID,
  pnuint32        restriction,
  pnuint32        inUse
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP22s41VolGetRestrict(&access, volNum,
         NSwap32(objID), restriction, inUse));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gdskrest.c,v 1.7 1994/09/26 17:45:38 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:cvoldskr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwvol.h"

/*manpage*NWRemoveObjectDiskRestrictions************************************
SYNTAX:  NWCCODE N_API NWRemoveObjectDiskRestrictions
         (
            NWCONN_HANDLE  conn,
            nuint8         volNum,
            nuint32        objID
         )

REMARKS:

ARGS: >  conn
      >  volNum
      >  objID

INCLUDE: nwvol.h

RETURN:  0x0000 Successful
         0x898C No Set Privileges
         0x89FE User Not Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 34  Remove User Disk Space Restriction

CHANGES: 12 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWRemoveObjectDiskRestrictions
(
   NWCONN_HANDLE  conn,
   nuint8         volNum,
   nuint32        objID
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP22s34VolRemoveRestrict(&access, volNum,
               NSwap32(objID)));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/cvoldskr.c,v 1.7 1994/09/26 17:45:03 rebekah Exp $
*/

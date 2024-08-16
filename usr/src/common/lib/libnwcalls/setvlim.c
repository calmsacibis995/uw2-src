/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setvlim.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"
#include "nwcaldef.h"
#include "nwvol.h"

/*manpage*NWSetObjectVolSpaceLimit******************************************
SYNTAX:  NWCCODE N_API NWSetObjectVolSpaceLimit
         (
             NWCONN_HANDLE conn,
             nuint16       volNum,
             nuint32       objID,
             nuint32       restriction
         );

REMARKS: Sets a Bindery object's disk space limit on a volume.
         Valid space limits range from 0 - 0x4000 0000.

ARGS: >  conn
      >  volNum
      >  objID
      >  restriction

INCLUDE: nwvol.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 33  Add User Disk Space Restriction

CHANGES: 21 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetObjectVolSpaceLimit
(
  NWCONN_HANDLE   conn,
  nuint16         volNum,
  nuint32         objID,
  nuint32         restriction
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP22s33VolAddRestrict(&access, (nuint8) volNum,
                        NSwap32(objID), restriction));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setvlim.c,v 1.7 1994/09/26 17:50:03 rebekah Exp $
*/

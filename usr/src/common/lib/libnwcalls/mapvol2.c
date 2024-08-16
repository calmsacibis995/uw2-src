/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:mapvol2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwvol.h"

/*manpage*NWGetVolumeName****************************************************
SYNTAX:  NWCCODE N_API NWGetVolumeName
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            pnstr8         volName
         )

REMARKS:

ARGS:

INCLUDE: nwvol.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 06  Get Volume Name

CHANGES: 15 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetVolumeName
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   pnstr8         volName
)
{
   NWCCODE  ccode;
   nuint8   buLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE)NWNCP22s6VolGetName(&access,(nuint8) volNum, &buLen,
                     volName);
   if (ccode == 0)
      volName[buLen] = (nstr8) 0x00;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/mapvol2.c,v 1.7 1994/09/26 17:48:04 rebekah Exp $
*/

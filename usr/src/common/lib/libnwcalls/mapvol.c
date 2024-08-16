/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:mapvol.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwvol.h"

/*manpage*NWGetVolumeNumber*******************************************************************
SYNTAX:  NWCCODE N_API NWGetVolumeNumber
         (
            NWCONN_HANDLE  conn,
            pnstr8         volName,
            pnuint16       volNum
         )

REMARKS: Returns the volume number given a connection ID and volume Name.

ARGS:  > conn
       > volName
      <  volNum

INCLUDE: nwvol.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 05  Get Volume Number

CHANGES: 15 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetVolumeNumber
(
   NWCONN_HANDLE  conn,
   pnstr8         volName,
   pnuint16       volNum
)
{
   NWCCODE ccode;
   nuint8 buTemp;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   *volNum = 0; /* init upper byte to zero - buTemp is only a byte */

   if ((ccode = (NWCCODE) NWNCP22s5VolGetNum(&access,
         (nuint8)NWCStrLen(volName), volName, &buTemp)) == 0)
   {
       *volNum = (nuint16) buTemp;
   }

   return((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/mapvol.c,v 1.7 1994/09/26 17:48:02 rebekah Exp $
*/

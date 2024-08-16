/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getacces.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"

/*manpage*NWGetBinderyAccessLevel*******************************************
SYNTAX:  NWCCODE N_API NWGetBinderyAccessLevel
         (
            NWCONN_HANDLE  conn,
            pnuint8        accessLevel,
            pnuint32       objID
         )

ARGS: >  conn
      <  accessLevel
      <  objID

INCLUDE: nwbindry.h

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

NCP:     23 70  Get Bindery Access Level

CHANGES: 20 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetBinderyAccessLevel
(
   NWCONN_HANDLE  conn,
   pnuint8        accessLevel,
   pnuint32       objID
)
{
   NWCCODE ccode;
   nuint32 tObjID;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);


   if((ccode = (NWCCODE)NWNCP23s70GetClntAccessMask(&access, accessLevel,
         &tObjID)) == 0)
   {
      if(objID)
         *objID = NSwap32(tObjID);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getacces.c,v 1.7 1994/09/26 17:45:41 rebekah Exp $
*/

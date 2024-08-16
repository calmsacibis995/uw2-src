/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtobjnam.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"

#include "ncpbind.h"
#include "nwbindry.h"

/*manpage*NWGetObjectName***************************************************
SYNTAX:  NWCCODE N_API NWGetObjectName
         (
            NWCONN_HANDLE  conn,
            nuint32        objID,
            pnstr8         objName,
            pnuint16       objType
         );

REMARKS: This call allows a client to map an Object ID number to an Object Name
         and Object Type.

         Any client can use this call successfully if an object in the bindery
         corresponds to the indicated Object ID number and if the client has
         read privileges to that object.

ARGS: >  conn
      >  objID
      <  objName
      <  objType

INCLUDE: nwbindry.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F1  Bindery Security
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 54  Get Bindery Object Name

CHANGES: 25 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetObjectName
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   pnstr8         objName,
   pnuint16       objType
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE)NWNCP23s54GetObjName(&access, NSwap32(objID), NULL,
         objType, objName)) == 0)
   {
      if (objType)
         *objType = NSwap16(*objType);
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtobjnam.c,v 1.7 1994/09/26 17:47:20 rebekah Exp $
*/

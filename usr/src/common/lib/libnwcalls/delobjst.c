/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:delobjst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWDeleteObjectFromSet*********************************************
SYNTAX:  NWCCODE N_API NWDeleteObjectFromSet
         (
            NWCONN_HANDLE  conn,
            pnstr8         objectName,
            nuint16        objectType,
            pnstr8         propertyName,
            pnstr8         memberName,
            nuint16        memberType
         );

REMARKS: Deletes a member from a bindery property of type SET
         on the file server associated with the given connection ID.

         There are two types of bindery properties: ITEM and SET.  SET
         properties are those that contain multiple bindery objs (refer
         to the "Introduction").

ARGS: >  conn
      >  objectName
      >  objectType
      >  propertyName
      >  memberName
      >  memberType

INCLUDE: nwbindry.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 66  Delete Bindery Object From Set

CHANGES: 24 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDeleteObjectFromSet
(
   NWCONN_HANDLE  conn,
   pnstr8         objectName,
   nuint16        objectType,
   pnstr8         propertyName,
   pnstr8         memberName,
   nuint16        memberType
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWNCP23s66DeleteObjFromSet(&access, NSwap16(objectType),
               (nuint8)NWCStrLen(objectName), objectName,
               (nuint8)NWCStrLen(propertyName), propertyName,
               NSwap16(memberType), (nuint8)NWCStrLen(memberName),
               memberName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/delobjst.c,v 1.7 1994/09/26 17:45:10 rebekah Exp $
*/

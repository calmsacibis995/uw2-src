/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:addobj.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWAddObjectToSet**************************************************
SYNTAX:  NWCCODE N_API NWAddObjectToSet
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType,
            pnstr8         propertyName,
            pnstr8         memberName,
            nuint16        memberType
         );

REMARKS: Adds a member to a bindery property of type SET.

         The objName, objType, and propertyName parameters must
         uniquely identify the property and cannot contain wildcard
         characters.

         The memberName and memberType parameters must uniquely identify
         the bindery object to be added and cannot contain wildcard
         characters.

         The property must be of type SET.

         This function searches consecutive segments of the property's
         value for an open slot where it can record the unique bindery
         object identification of the new member.  The new member is
         inserted into the first available slot.  if no open slot is found,
         a new segment is created and the new member's unique bindery
         object identification is written into the first slot of the new
         segment.  The rest of the segment is filled.

         A client must have write access to the property to make this call.

ARGS: >  conn
      >  objName
      >  objType
      >  propertyName
      >  memberName
      >  memberType

INCLUDE: nwbindry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 65  Add Bindery Object To Set

CHANGES: 21 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAddObjectToSet
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   pnstr8         memberName,
   nuint16        memberType
)
{
   nuint8 objNameLen, propertyNameLen, memberNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   objNameLen  = (nuint8)NWCStrLen(objName);
   propertyNameLen = (nuint8)NWCStrLen(propertyName);
   memberNameLen = (nuint8)NWCStrLen(memberName);

   return ((NWCCODE) NWNCP23s65AddObjToSet(&access, NSwap16(objType),
               objNameLen, objName, propertyNameLen,
               propertyName, NSwap16(memberType), memberNameLen,
               memberName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/addobj.c,v 1.7 1994/09/26 17:43:43 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:isobjset.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"

/*manpage*NWIsObjectInSet***************************************************
SYNTAX:  NWCCODE N_API NWIsObjectInSet
         (
            NWCONN_HANDLE conn,
            pnstr8        objName,
            nuint16       objType,
            pnstr8        propertyName,
            pnstr8        memberName,
            nuint16       memberType
         );

REMARKS: Searches a property of type SET for a specified object.

         The setObjectName, setObjectType, and setPropertyName parameters
         must uniquely identify the property and cannot contain wildcard
         specifiers.

         The memberName and memberType parameters must uniquely identify
         the bindery object and cannot contain wildcard specifiers.  The
         property must be of type SET.

         This function does not expand members of type GROUP in an attempt
         to locate a specific member.  For example, assume the following
         bindery objects and properties exist:


            ObjectProperty         Property Value

            JOAN                   SECRETARIES
            GROUP_MEMBERS          JOAN's object ID
            EMPLOYEESGROUP_MEMBERS SECRETARIES' object ID


         JOAN is not considered a member of EMPLOYEES because she is not
         explicitly listed in the EMPLOYEES' GROUP_MEMBERS property.  In
         addition, the bindery does not check for recursive (direct or
         indirect) membership definitions.

         Read access to the property is required for this call.

ARGS: >  conn
      >  objName
      >  objType
      >  propertyName
      >  memberName
      >  memberType

INCLUDE: nwbindry.h

RETURN:  Zero (0)    Object was found
         Non-zero    Object NOT found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 67  Is Bindery Object In Set

CHANGES: 25 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWIsObjectInSet
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   pnstr8         memberName,
   nuint16        memberType
)
{
   nuint8 objNameLen;
   nuint8 propertyNameLen;
   nuint8 memberNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   objNameLen      = (nuint8)NWCStrLen(objName);
   propertyNameLen = (nuint8)NWCStrLen(propertyName);
   memberNameLen   = (nuint8)NWCStrLen(memberName);

   return((NWCCODE)NWNCP23s67IsObjInSet(&access, NSwap16(objType),
          objNameLen, objName, propertyNameLen, propertyName,
         NSwap16(memberType), memberNameLen, memberName));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/isobjset.c,v 1.7 1994/09/26 17:47:42 rebekah Exp $
*/

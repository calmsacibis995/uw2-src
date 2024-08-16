/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:propsrty.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWChangePropertySecurity******************************************
SYNTAX:  NWCCODE N_API NWChangePropertySecurity
         (
            NWCONN_HANDLE  conn,
            pnstr8         objectName,
            nuint16        objectType,
            pnstr8         propertyName,
            nuint8         newPropertySecurity
         );

REMARKS:
         Changes the security access mask of a property.

         The objName, objType, and propertyName parameters must
         uniquely identify the property and cannot contain wildcard
         specifiers.

         This function cannot set or clear BINDERY read or write security.

         The requesting process cannot change a property's security to a
         level greater than the process's access to the property.

         This function requires write access to the bindery object, and
         read and write access to the property.

ARGS: >  conn
      >  objectName
      >  objectType
      >  propertyName
      >  newPropertySecurity

INCLUDE: nwbindry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 59  Change Property Security

CHANGES: 24 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWChangePropertySecurity
(
   NWCONN_HANDLE  conn,
   pnstr8         objectName,
   nuint16        objectType,
   pnstr8         propertyName,
   nuint8         newPropertySecurity
)
{
   nuint8 newConnStatus;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s59ChgPropertySecurity(&access,
	         NSwap16(objectType), (nuint8)NWCStrLen(objectName),
            objectName, newPropertySecurity, (nuint8)NWCStrLen(propertyName),
            propertyName, &newConnStatus));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/propsrty.c,v 1.7 1994/09/26 17:48:35 rebekah Exp $
*/

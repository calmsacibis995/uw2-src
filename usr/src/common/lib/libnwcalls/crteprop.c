/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:crteprop.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWCreateProperty**************************************************
SYNTAX:  NWCCODE N_API NWCreateProperty
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType,
            pnstr8         propertyName,
            nuint8         propertyFlags,
            nuint8         propertySecurity
         );

REMARKS:
         Adds a property to a bindery object on the file server associated
         with the given connection ID.

         The propertyFlags parameter defines a property's type and whether
         the property is dynamic. A dynamic property is one that is created
         and deleted frequently. Dynamic properties are deleted from the
         bindery when the file server is reinitialized.

         The property type indicates the type of data a property value
         contains. SET property types contain a set of bindery object
         identifications. The bindery attaches no significance to the
         contents of a property value if the property is of type ITEM (see
         the discussion in the "Introduction").

         The newPropertySecurity parameter is a byte in which the low 4 bits
         (nibble) control read security and the high 4 bits control write
         security.  Read security controls which clients can read the
         property.  Write security controls which clients can write to the
         property.  The values for the newPropertySecurity parameter are
         described in the chart below.

         The requesting process cannot create properties that have security
         greater than the process's access to the bindery object.

         The password property is created by calling NWChangeObjectPassword
         rather than by using the NWCreateProperty function.

         This function requires write access to the bindery object.

ARGS: >  objName
         Pointer to the object name receiving the new property

      >  objType
         The type of the affected bindery object

      >  propertyName
         Pointer to the name of the property being created

      >  propertyFlags
         The bindery flags of the new property (ORed with BF_ITEM or BF_SET):

         BF_STATIC
         BF_DYNAMIC

      >  propertySecurity
         The new property's security access mask

INCLUDE: nwbindry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 57  Create Property

CHANGES: 23 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWCreateProperty
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   nuint8         propertyFlags,
   nuint8         propertySecurity
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s57CreateProperty(&access, NSwap16(objType),
      (nuint8)NWCStrLen(objName), objName, propertyFlags,
      propertySecurity, (nuint8)NWCStrLen(propertyName), propertyName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/crteprop.c,v 1.7 1994/09/26 17:44:59 rebekah Exp $
*/

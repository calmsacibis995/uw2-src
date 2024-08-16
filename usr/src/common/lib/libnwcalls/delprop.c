/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:delprop.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWDeleteProperty**************************************************
SYNTAX:  NWCCODE N_API NWDeleteProperty
         (
            NWCONN_HANDLE  conn,
            pnstr8         objectName,
            nuint16        objectType,
            pnstr8         propertyName
         );

REMARKS: Removes a property from a bindery object on the specified file server.

         The objName and objType must uniquely identify the bindery
         object and cannot contain wildcard characters.

         All matching properties of the bindery object are deleted when the
         propertyName contains wildcard characters.

         This function requires write access to the bindery object and the
         property.

ARGS: >  conn
      >  objectName
      >  objectType
      >  propertyName


INCLUDE: nwbindry.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 58  Delete Property

CHANGES: 24 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDeleteProperty
(
   NWCONN_HANDLE  conn,
   pnstr8         objectName,
   nuint16        objectType,
   pnstr8         propertyName
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s58DeleteProperty(&access,
                                        NSwap16(objectType),
                                        (nuint8)NWCStrLen(objectName),
                                        objectName,
                                        (nuint8)NWCStrLen(propertyName),
                                        propertyName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/delprop.c,v 1.7 1994/09/26 17:45:12 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:crteobj.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"

/*manpage*NWCreateObject****************************************************
SYNTAX:  NWCCODE N_API NWCreateObject
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType,
            nuint8         objFlags,
            nuint8         objSecurity
         );

REMARKS:
         Adds a new object to the bindery on the file server associated with
         the given server connection identification number.

         The objName and objType parameters must uniquely identify the
         bindery object and cannot contain wildcard specifiers.

         Only SUPERVISOR or a bindery object that is security equivalent to
         SUPERVISOR can create bindery objects.

         The bindery object must have a password property to log in to a file
         server. The password property is created with the
         NWChangeObjectPassword function.

ARGS: >  conn
      >  objName
      >  objType
      >  objFlags
      >  objSecurity

INCLUDE: nwbindry.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 50  Create Bindery Object

CHANGES: 23 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWCreateObject
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   nuint8         objFlags,
   nuint8         objSecurity
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s50CreateObj(&access, objFlags,
                        objSecurity, NSwap16(objType),
                        (nuint8)NWCStrLen(objName), objName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/crteobj.c,v 1.7 1994/09/26 17:44:57 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:delobj.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"

#include "ncpbind.h"
#include "nwbindry.h"

/*manpage*NWDeleteObject****************************************************
SYNTAX:  NWCCODE N_API NWDeleteObject
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType
         );

REMARKS: Removes an object from the bindery of the file
         server associated with the given connection identification.

         The objName and objType parameters must uniquely identify
         the bindery object and cannot contain wildcard specifiers.

         Only SUPERVISOR or a bindery object that is security equivalent to
         SUPERVISOR can delete bindery objects.

ARGS: >  conn
      >  objName
      >  objType

INCLUDE: nwbindry.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 51  Delete Bindery Object

CHANGES: 21 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDeleteObject
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType
)
{
   nuint8 objNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   objNameLen = (nuint8)NWCStrLen(objName);

   return ((NWCCODE) NWNCP23s51DeleteObj(&access, NSwap16(objType),
					objNameLen, objName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/delobj.c,v 1.7 1994/09/26 17:45:09 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:renamobj.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWRenameObject****************************************************
SYNTAX:  NWCCODE N_API NWRenameObject
         (
            NWCONN_HANDLE  conn,
            pnstr8         oldObjName,
            pnstr8         newObjName,
            nuint16        objType
         );

REMARKS: Renames an object in the bindery.

         The oldObjName, newObjName, and oldObjectType parameters
         must uniquely identify the bindery object and cannot contain
         wildcard specifiers.

         Only SUPERVISOR or equivalent can rename bindery objects.

ARGS: >  conn
      >  oldObjName
      >  newObjName
      >  objType

INCLUDE: nwbindry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 52  Rename Object

CHANGES: 26 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWRenameObject
(
   NWCONN_HANDLE  conn,
   pnstr8         oldObjName,
   pnstr8         newObjName,
   nuint16        objType
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s52RenameObj(&access,
            NSwap16(objType), (nuint8)NWCStrLen(oldObjName),
            oldObjName, (nuint8)NWCStrLen(newObjName), newObjName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/renamobj.c,v 1.7 1994/09/26 17:49:06 rebekah Exp $
*/


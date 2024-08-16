/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getobjid.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"

#include "ncpbind.h"
#include "nwbindry.h"

/*manpage*NWGetObjectID*****************************************************
SYNTAX:  NWCCODE N_API NWGetObjectID
         (
            NWCONN_HANDLE  conn,
            pnstr8         objectName,
            nuint16        objectType,
            pnuint32       objectID
         );

REMARKS: This call allows a client to map an Object Name to its corresponding unique
         Object ID number.  Object ID numbers are valid only on the server from which
         they are extracted since each server maintains its own bindery.

         The Object Type cannot be WILD (-1); the Object Name cannot contain wildcard
         characters.

         Any client can use this call successfully if the specified object is in the
         bindery and if the client has read security privileges to the object.

ARGS: >  conn
      >  objectName
      >  objectType
      <  objectID

INCLUDE: nwbindry.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 53  Get Bindery Object ID

CHANGES: 23 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetObjectID
(
   NWCONN_HANDLE  conn,
   pnstr8         objectName,
   nuint16        objectType,
   pnuint32       objectID
)
{
   NWCCODE ccode;
   nuint8 objNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   objNameLen = (nuint8)NWCStrLen(objectName);

   if((ccode = (NWCCODE)NWNCP23s53GetObjID(&access, NSwap16(objectType), objNameLen,
                     objectName, objectID, NULL, NULL)) == 0)
   {
      if(objectID)
         *objectID = NSwap32(*objectID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getobjid.c,v 1.7 1994/09/26 17:46:15 rebekah Exp $
*/

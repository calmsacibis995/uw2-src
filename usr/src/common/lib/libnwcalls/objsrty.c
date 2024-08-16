/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:objsrty.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"

#include "ncpbind.h"
#include "nwbindry.h"

/*manpage*NWChangeObjectSecurity********************************************
SYNTAX:  NWCCODE N_API NWChangeObjectSecurity
         (
            NWCONN_HANDLE  conn,
            pnstr8         objectName,
            nuint16        objectType,
            nuint8         newObjectSecurity
         );

REMARKS: This call allows an object supervisor to change the security level
         mask of an object in the bindery.

         The Object Type cannot be WILD (-1), and the Object Name cannot
         contain wildcard characters.

         Security levels above level 3 (supervisor) cannot be changed with
         this call.

         Only object supervisors can use this call.

ARGS: >  conn
      >  objectName
      >  objectType
      >  newObjectSecurity

INCLUDE: nwbindry.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F5  No Object Create
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 56  Change Bindery Object Security

CHANGES: 24 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWChangeObjectSecurity
(
   NWCONN_HANDLE  conn,
   pnstr8         objectName,
   nuint16        objectType,
   nuint8         newObjectSecurity
)
{
   nuint8 objectNameLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   objectNameLen = (nuint8)NWCStrLen(objectName);

   return ((NWCCODE) NWNCP23s56ChangeObjSecurity(&access, newObjectSecurity,
                        NSwap16(objectType), objectNameLen, objectName,
                        NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/objsrty.c,v 1.7 1994/09/26 17:48:23 rebekah Exp $
*/


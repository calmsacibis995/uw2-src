/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtobjrts.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"
#include "nwclocal.h"

/*manpage*NWGetObjectEffectiveRights****************************************
SYNTAX:  NWCCODE N_API NWGetObjectEffectiveRights
         (
            NWCONN_HANDLE  conn,
            nuint32        objID,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            pnuint16       rightsMask
         );

REMARKS: This NCP gets an object's effective access rights to a specified
         directory or file.  The caller must be a supervisor, a manager of the
         object for whom righs to the specified directory of file are requested,
         or the object itself.

         This call is similar to Get Effective Rights For Directory Entry
         (0x2222 22 42); however, this call (22 50) allows the caller to specify
         an object (ObjectID parameter) for whom rights will be returned.

ARGS: >  conn
      >  objID
      >  dirHandle
      >  path
      <  rightsMask

INCLUDE: nwdirect.h

RETURN:  0x0000  Success
         0x897E  Invalid Length
         0x899B  Invalid Directory Error
         0x899C  Invalid Path

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 50  Get Effective Rights for Object

CHANGES: 25 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetObjectEffectiveRights
(
   NWCONN_HANDLE   conn,
   nuint32         objID,
   NWDIR_HANDLE    dirHandle,
   pnstr8          path,
   pnuint16        rightsMask
)
{
   nuint8 pathLen;
   nuint8 tmpPath[256];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(path)
   {
      NWCStrCpy(tmpPath, path);
      NWConvertToSpecChar(tmpPath);
      pathLen = (nuint8) NWCStrLen(tmpPath);
   }
   else
   {
      pathLen = 0;
   }

   return ((NWCCODE) NWNCP22s50GetObjEffectRights(&access,
               NSwap32(objID), dirHandle, pathLen, tmpPath,
               rightsMask));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtobjrts.c,v 1.7 1994/09/26 17:47:21 rebekah Exp $
*/

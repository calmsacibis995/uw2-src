/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scanobj.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"

/*manpage*NWScanObject******************************************************
SYNTAX:  NWCCODE N_API NWScanObject
         (
            NWCONN_HANDLE  conn,
            pnstr8         searchName,
            nuint16        searchType,
            pnuint32       objID,
            pnstr8         objName,
            pnuint16       objType,
            pnuint8        hasPropertiesFlag,
            pnuint8        objFlags,
            pnuint8        objSecurity
         );

REMARKS: If NULL is passed in for objID, this function defaults to initial
         sequence number of -1L.  This means you can only get the first item
         of any sequence with a NULL.

ARGS: >  conn
      >  searchName
      >  searchType
      <> objectID
      <  objectName (optional)
      <  objectType (optional)
      <  hasPropertiesFlag (optional)
      <  objectFlags (optional)
      <  objectSecurity (optional)

INCLUDE: nwbindry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 55  Scan Bindery Object

CHANGES: 26 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanObject
(
   NWCONN_HANDLE  conn,
   pnstr8         searchName,
   nuint16        searchType,
   pnuint32       objID,
   pnstr8         objName,
   pnuint16       objType,
   pnuint8        hasPropertiesFlag,
   pnuint8        objFlags,
   pnuint8        objSecurity
)
{
   NWCCODE ccode;
   nuint32 tObjID;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   tObjID = objID ? *objID : 0xFFFFFFFFL;
   tObjID = NSwap32(tObjID);

   if ((ccode = (NWCCODE)NWNCP23s55ScanObj(&access, NSwap16(searchType),
            (nuint8)NWCStrLen(searchName), searchName, &tObjID,
            objType, objName, objFlags, objSecurity,
            hasPropertiesFlag)) == 0)
   {
      if (objType != NULL)
         *objType = NSwap16(*objType);

      if (objID)
         *objID = NSwap32(tObjID);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scanobj.c,v 1.7 1994/09/26 17:49:22 rebekah Exp $
*/

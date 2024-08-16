/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scanprop.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"

/*manpage*NWScanProperty****************************************************
SYNTAX:  NWCCODE N_API NWScanProperty
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType,
            pnstr8         srchPropertyName,
            pnuint32       iterHandle,
            pnstr8         propertyName,
            pnuint8        propertyFlags,
            pnuint8        propertySecurity,
            pnuint8        valueAvailable,
            pnuint8        moreFlag
         )

REMARKS:

ARGS:  > conn
       > objName
       > objType
       > srchPropertyName
      <> iterHandle (optional)
      <  propertyName
      <  propertyFlags
      <  propertySecurity
      <  valueAvailable
      <  moreFlag

INCLUDE: nwbindry.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 60  Scan Property

CHANGES: 26 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*******************************************************************s*********/
NWCCODE N_API NWScanProperty
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         srchPropertyName,
   pnuint32       iterHandle,
   pnstr8         propertyName,
   pnuint8        propertyFlags,
   pnuint8        propertySecurity,
   pnuint8        valueAvailable,
   pnuint8        moreFlag
)
{
   nuint32 startIterHandle;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(!iterHandle)
   {
      startIterHandle = (nuint32) -1L;
      iterHandle = &startIterHandle;
   }

   return (NWCCODE) NWNCP23s60ScanProperty(&access, NSwap16(objType),
      (nuint8)NWCStrLen(objName), objName,  iterHandle,
      (nuint8)NWCStrLen(srchPropertyName), srchPropertyName,
      propertyName, propertyFlags, propertySecurity,
      valueAvailable, moreFlag);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scanprop.c,v 1.7 1994/09/26 17:49:24 rebekah Exp $
*/

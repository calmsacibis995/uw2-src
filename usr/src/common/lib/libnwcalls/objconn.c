/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:objconn.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpconn.h"

#include "nwserver.h"
#include "nwconnec.h"

/*manpage*NWGetObjectConnectionNumbers**************************************
SYNTAX:  NWCCODE N_API NWGetObjectConnectionNumbers
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType,
            pnuint16       numConns,
            NWCONN_NUM NWPTR connList,
            nuint16        maxConns
         )

REMARKS: Returns a list of the connection numbers on the
         server for clients logged in with the specified object name and
         object type.

ARGS:  > conn
       > objName
       > objType
      <  numConns
      <  connList
       > maxConns

INCLUDE: nwundoc.h

RETURN:  0x0000    Successful
         0x8996    Server Out Of Space
         0x89F0    Illegal Wildcard
         0x89FC    No Such Object
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 21  Get Object Connection List
         23 27  Get Object Connection List

CHANGES: 16 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetObjectConnectionNumbers
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnuint16       numConns,
   NWCONN_NUM NWPTR connList,
   nuint16        maxConns
)
{
   NWCCODE ccode;
   nuint32 srchConnNum;
   nuint32 connNumList[128];
   nuint8  objNameLen, listLen;
   nuint16 serverVer, tempNumConns = 0;
   nint    i;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      objNameLen = (nuint8) NWCStrLen(objName);
      srchConnNum = (nuint32) 0;

      do
      {
         if((ccode = (NWCCODE) NWNCP23s27GetObjConnList(&access,
               srchConnNum, NSwap16(objType), objNameLen, objName,
               &listLen, connNumList)) == 0)
         {
            if(listLen > 0)
            {
               for(i = 0; (i < (nint)listLen) &&
                           (tempNumConns < maxConns); i++, tempNumConns++)
                  *connList++ = (NWCONN_NUM) connNumList[i];

               srchConnNum = connNumList[i - 1];
            }
         }
       if (listLen == 0)
         break;
      } while(!ccode && (tempNumConns < maxConns) && listLen);

      if(ccode && tempNumConns)
         ccode = 0;  /* if any read call successful    */
      *numConns = tempNumConns;
   }
   else
   {
      nuint8 byteConnList[256];

      objNameLen = (nuint8)NWCStrLen(objName);

      if((ccode = (NWCCODE) NWNCP23s21GetObjConnList(&access,
            NSwap16(objType), objNameLen, objName, &listLen,
            byteConnList)) == 0)
      {
         *numConns = (nuint16) listLen;

         if(maxConns > (nuint16) listLen)
            maxConns = (nuint16) listLen;

         for(i = 0; i < (nint) maxConns; i++)
            *connList++ = (NWCONN_NUM) byteConnList[i];
      }
      else
         *numConns = (nuint16) 0;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/objconn.c,v 1.7 1994/09/26 17:48:22 rebekah Exp $
*/

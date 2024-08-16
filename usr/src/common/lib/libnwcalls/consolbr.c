/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:consolbr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpmsg.h"

#include "nwcaldef.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwmsg.h"

/*manpage*NWSendConsoleBroadcast********************************************
SYNTAX:  NWCCODE N_API NWSendConsoleBroadcast
         (
           NWCONN_HANDLE   conn,
           pnstr8          message,
           nuint16         connCount,
           pnuint16        connList
         )

REMARKS: This call sends a message to a list of logical connections.  A
         message will not reach a station that has disabled broadcasts or is
         not logged in. The requesting client must have console operator
         rights.

ARGS:

INCLUDE: nwmsg.h

RETURN:  0x89c6  No Console Rights
         0x89fd  Bad Station Number

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:      23 209  Send Console Broadcast
          23 253  Send Console Broadcast

CHANGES:  26 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSendConsoleBroadcast
(
   NWCONN_HANDLE   conn,
   pnstr8          message,
   nuint16         connCount,
   pnuint16        connList
)
{
   nuint16 i;
   NWCCODE ccode;

   nuint16 serverVer;
   nuint8  msgLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);


   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
    return (ccode);

   if(serverVer >= 3110)
   {
      nuint32 luTConnList[63];

      if(connCount > 62)
         return 0x89ff;

      for(i = 0; i < connCount; i++)        /* convert nuint16 to nuint32 */
         luTConnList[i] = (nuint32) connList[i];

      msgLen = (nuint8) NWLTruncateString(message, 255);


      ccode = (NWCCODE) NWNCP23s253SendConsoleBroadcast(&access,
                           (nuint8) connCount, luTConnList,
                           msgLen, message);

      if(serverVer > 3110 || !ccode)
         return ccode;          /* return if thousand user support */
   }

   if(serverVer <= 3110)
   {
      nuint8 abuTConnList[255];

      msgLen = (nuint8)NWLTruncateString(message, 59);

      for (i = 0; i < connCount; i++)        /* convert nuint16 to nuint8 */
         abuTConnList[i] = (nuint8) connList[i];

      ccode = (NWCCODE) NWNCP23s209SendConsoleBroadcast(&access,
                              (nuint8)connCount, abuTConnList, msgLen,
                              message);
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/consolbr.c,v 1.7 1994/09/26 17:44:43 rebekah Exp $
*/

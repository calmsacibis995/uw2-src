/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:sendmsg.c	1.5"
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

/*manpage*NWSendBroadcastMessage********************************************
SYNTAX:  NWCCODE N_API NWSendBroadcastMessage
         (
            NWCONN_HANDLE  conn,
            pnstr8         message,
            nuint16        connCount,
            pnuint16       connList,
            pnuint8        resultList
         )

REMARKS: This function allows a client to send a message through a
         communication pipe to another client. Personal messages are
         truncated at 60 bytes unless the server is 3.11 or above (see note
         for exceptions) in which case messages are truncated at 255 bytes.
         The message can be sent to multiple clients at once.  The function
         returns a list of send status codes in the resultList.

         NOTE:  Because the thousand user send NCP was avaliable on some
               3.11 revisions, it is not known how to check which NCP to
               use. As a result, if the server version is 3.11 the new
               21 10 NCP is called. If it returns a 0xff failure, the
               old NCP is called.  This will cause a message longer than 60
               bytes to be truncated if the new NCP is not supported.

ARGS:

INCLUDE: nwmsg.h

RETURN:  0x89FF  no pipe half exists from calling client
         0x89FE  no pipe half exists to destination client
         0x89FD  message queue full

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     21 04  Send Personal Message
         21 10  Send Broadcast Message

CHANGES: 16 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSendBroadcastMessage
(
   NWCONN_HANDLE  conn,
   pnstr8         message,
   nuint16        connCount,
   pnuint16       connList,
   pnuint8        resultList
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint   i;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if (serverVer >= 3110)
   {
      nuint32 aluTConnList[62];

      if(connCount > 62)
         return ((NWCCODE) 0x89ff);

      for(i = 0; i < connCount; i++)
         aluTConnList[i] = (nuint32) connList[i];

      if (NWNCP21s10MsgSendBroadcast(&access, connCount, aluTConnList,
               (nuint8) NWLTruncateString(message, 255), message, &connCount,
               aluTConnList) == 0)
      {
         if (resultList)
         {
            for(i = 0; i < connCount; i++)
               *resultList++ = (nuint8) aluTConnList[i];
         }

         return ((NWCCODE) 0);
      }
   }

   if (serverVer <= 3110)
   {
      nuint8 abuTConnList[512];
      nuint8 buBucket;

      /* Copy the nuint16 connection numbers to the nuint8 core protocol packet */
      for(i = 0; i < connCount; i++)
         abuTConnList[i] = (nuint8) connList[i];

      return ((NWCCODE) NWNCP21s0MsgSendBroadcast(&access,
                  (nuint8) connCount, abuTConnList,
                  (nuint8) NWLTruncateString(message, 59), message,
                  &buBucket, resultList));
   }

   return ((NWCCODE) 0x00FF);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/sendmsg.c,v 1.7 1994/09/26 17:49:42 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtserial.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetNetworkSerialNumber*******************************************
SYNTAX:  NWCCODE N_API NWGetNetworkSerialNumber
         (
            NWCONN_HANDLE  conn,
            pnuint32       serialNum,
            pnuint16       appNum
         );

REMARKS:

ARGS: >  conn
      <  serialNum
      <  appNum

INCLUDE: nwserver.h

RETURN:  standard errors

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 18  Get Network Serial Number

CHANGES: 28 May 1993 - written - jwoodbur
         13 Sep 1993 - NWNCP Enabled - jsumsion
****************************************************************************/
NWCCODE N_API NWGetNetworkSerialNumber
(
   NWCONN_HANDLE  conn,
   pnuint32       serialNum,
   pnuint16       appNum
)
{
   NWCCODE ccode;
   nuint16 suServerVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = NWGetFileServerVersion(conn, &suServerVer);
   if (ccode != 0)
      return (ccode);

   if ((ccode = (NWCCODE)NWNCP23s18GetNetSerialNum(&access, serialNum, appNum)) == 0)
   {
      if (suServerVer <= 3120)
      {
         /***** this is a workaround for the NCP, which swaps differently
                in different versions *****/

         *serialNum = NSwap32(*serialNum);
         *appNum    = NSwap16(*appNum);
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtserial.c,v 1.7 1994/09/26 17:47:28 rebekah Exp $
*/

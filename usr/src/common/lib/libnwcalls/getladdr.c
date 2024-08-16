/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getladdr.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpconn.h"

#include "nwclient.h"
#include "nwconnec.h"
#include "nwserver.h"

/*manpage*NWGetInetAddr******************************************************
SYNTAX:  NWCCODE N_API NWGetInetAddr
         (
           NWCONN_HANDLE      conn,
           NWCONN_NUM         connNum,
           NWINET_ADDR NWPTR  inetAddr
         )

REMARKS: Gets the internet address for a connection number

ARGS: >  conn
      >  connNum
      <  inetAddr

         typedef struct tNWINET_ADDR
         {
           nuint8 networkAddr[4];
           nuint8 netNodeAddr[6];
           nuint16 socket;
           nuint16 connType;
         } NWINET_ADDR;

         connType is used for 3.11 and above only:
           0=not in use
           2=NCP
           3=AFP

INCLUDE: nwconnec.h

RETURN:  0x0000    Successful
         0x89FF    Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:     NWGetInternetAddress

NCP:     23 19  Get Internet Address
         23 26  Get Internet Address

CHANGES: 14 Jun 1993 - written - jwoodbur
---------------------------------------------------------------------------
       Copyright (c) 1991, 1993 by Novell, Inc. All Rights Reserved.
****************************************************************************/
NWCCODE N_API NWGetInetAddr
(
  NWCONN_HANDLE      conn,
  NWCONN_NUM         connNum,
  NWINET_ADDR NWPTR  inetAddr
)
{
   nint    i;
   NWCCODE ccode = 0;
   nuint16 serverVer;
   nuint8  abuInetAddr[12];
   NWNCPNetAddr netAddr;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110 || serverVer < 2000)
   {
      nuint8 buConnType;

      ccode = (NWCCODE) NWNCP23s26GetInternetAddr(&access, connNum,
                  &netAddr, &buConnType);

      if(ccode == 0)
      {
         for (i = 0; i < 4; i++)
            inetAddr->networkAddr[i] = netAddr.abuNetAddr[i];

         for (i = 0; i < 6; i++)
            inetAddr->netNodeAddr[i] = netAddr.abuNetNodeAddr[i];

         inetAddr->socket   = netAddr.suNetSocket;
         inetAddr->connType = (nuint16) buConnType;
      }
   }

   if (ccode == 0x89fb || (serverVer < 3110 && serverVer >= 2000))
   {
      ccode = (NWCCODE) NWNCP23s19GetInternetAddr(&access,
                  (nuint8) connNum, abuInetAddr);

      if(ccode == 0)
      {
         for (i = 0; i < 4; i++)
            inetAddr->networkAddr[i] = abuInetAddr[i];

         for (i = 0; i < 6; i++)
            inetAddr->netNodeAddr[i] = abuInetAddr[i+4];

         NCopy16(&inetAddr->socket, &abuInetAddr[10]);
         inetAddr->connType = 0;
      }
   }

   return ccode;
}

/*manpage*NWGetInternetAddress**********************************************
SYNTAX:  NWCCODE N_API NWGetInternetAddress
         (
            NWCONN_HANDLE  conn,
            NWCONN_NUM     connNum,
            pnuint8        inetAddr
         )

REMARKS: Gets the internet address for a connection number

ARGS: >  conn
      >  connNumber
      <  inetAddr
         10 byte array to receive internet address

INCLUDE: nwconnec.h

RETURN:  0x0000    Successful
         0x89FF    Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:     NWGetInetAddr

NCP:     23 19  Get Internet Address
         23 26  Get Internet Address

CHANGES: 14 Jun 1993 - written - jwoodbur
         16 Sep 1993 - NWNCP Enabled - lbendixs
---------------------------------------------------------------------------
         Copyright (c) 1991, 1993 by Novell, Inc. All Rights Reserved.
****************************************************************************/
NWCCODE N_API NWGetInternetAddress
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNum,
   pnuint8        inetAddr
)
{
   nint    i;
   NWCCODE ccode;
   nuint16 serverVer;
   NWINET_ADDR iNetAddr;

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if ((ccode = NWGetInetAddr(conn, connNum, &iNetAddr)) == 0)
   {
      for (i = 0; i < 4; i++)
         inetAddr[i] = iNetAddr.networkAddr[i];
      for (i = 0; i < 6; i++)
         inetAddr[i + 4] = iNetAddr.netNodeAddr[i];
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getladdr.c,v 1.8 1994/09/26 17:46:08 rebekah Exp $
*/

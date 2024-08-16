/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrconum.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwclient.h"
#include "nwconnec.h"
#include "nwserver.h"

/*manpage*NWClearConnectionNumber*******************************************
SYNTAX:  NWCCODE N_API NWClearConnectionNumber
         (
           NWCONN_HANDLE   conn,
           NWCONN_NUM      connNumber
         )

REMARKS: Clears the specified connection number on the server

ARGS:

INCLUDE: nwconnec.h

RETURN:  0x89c6 NO_CONSOLE_PRIVILEGES
         0x89fd BAD_STATION_NUMBER

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 210  clear connection number
         23 254  clear connection number

CHANGES: 18 Feb 1993 - modified - jwoodbur
           added 1000 user support
         20 May 1993 - pnw enabled - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWClearConnectionNumber
(
  NWCONN_HANDLE   conn,
  NWCONN_NUM      connNum
)
{
   NWCCODE ccode;
   nuint16 serverVer;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;


   if(connNum > 255 || serverVer > 3000)
      return( (NWCCODE) NWNCP23s254ClearConn( &access, (nuint32) connNum));

   return ((NWCCODE) NWNCP23s210ClearConn( &access, (nuint8) connNum));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrconum.c,v 1.7 1994/09/26 17:44:29 rebekah Exp $
*/

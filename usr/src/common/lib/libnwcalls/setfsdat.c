/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setfsdat.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWSetFileServerDateAndTime****************************************
SYNTAX:  NWCCODE N_API NWSetFileServerDateAndTime
         (
            NWCONN_HANDLE  conn,
            nuint8         year,
            nuint8         month,
            nuint8         day,
            nuint8         hour,
            nuint8         minute,
            nuint8         second
         );

REMARKS:

ARGS: >  conn
      >  year
      >  month
      >  day
      >  hour
      >  minute
      >  second

INCLUDE: nwserver.h

RETURN:  0x0000   SUCCESS
         0x89C6   NO_CONSOLE_RIGHTS

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 202  Set File Server Date And Time

CHANGES: 13 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetFileServerDateAndTime
(
   NWCONN_HANDLE  conn,
   nuint8         year,
   nuint8         month,
   nuint8         day,
   nuint8         hour,
   nuint8         minute,
   nuint8         second
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s202SetServerDateTime(&access, year, month,
               day, hour, minute, second));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setfsdat.c,v 1.7 1994/09/26 17:49:58 rebekah Exp $
*/

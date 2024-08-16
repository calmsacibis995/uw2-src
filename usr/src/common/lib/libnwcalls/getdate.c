/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdate.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerDateAndTime****************************************
SYNTAX:  NWCCODE N_API NWGetFileServerDateAndTime
         (
            NWCONN_HANDLE  conn,
            pnuint8        dateTimeBuffer
         );

REMARKS:

ARGS: <  dateTimeBuffer
         Byte number:
         1   Year     - a number from 0 - 179 (80 = 1980 & 102 = 2002 etc.)
         2   Month    - a value from 1 - 12 for monts Jan. to Dec.
         3   Day      - a value from 1 - 31 for day of month.
         4   Hour     - value from 0 - 23 (0 is 12 midnight & 23 is 11 p.m.)
         5   Minute   - value from 0 - 59
         6   Second   - value from 0 - 59
         7   Week Day - a value from 0 - 6 (0 is Sunday)

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 202  Set File Server Date And Time

NCP:     20 --  Get File Server Date And Time

CHANGES: 9 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerDateAndTime
(
   NWCONN_HANDLE  conn,
   pnuint8        dateTimeBuffer
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP20GetServerDateAndTime(&access,
               &dateTimeBuffer[0], &dateTimeBuffer[1],
               &dateTimeBuffer[2], &dateTimeBuffer[3],
               &dateTimeBuffer[4], &dateTimeBuffer[5],
               &dateTimeBuffer[6]));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdate.c,v 1.7 1994/09/26 17:45:49 rebekah Exp $
*/

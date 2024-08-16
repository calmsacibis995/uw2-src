/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdskch.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetDiskChannelStats*********************************************
SYNTAX:  NWCCODE N_API NWGetDiskChannelStats
         (
            NWCONN_HANDLE  conn,
            nuint8         channelNum,
            DSK_CHANNEL_STATS N_FAR * statBuffer
         );

REMARKS:

ARGS: >  conn
      >  channelNum
      <  statBuffer

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 217  Get Disk Channel Statistics

CHANGES: 8 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDiskChannelStats
(
   NWCONN_HANDLE     conn,
   nuint8            channelNum,
   DSK_CHANNEL_STATS N_FAR * statBuffer
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE)NWNCP23s217GetDiskChannelStats(&access, channelNum,
         (pNWNCPDiskChannelStats) statBuffer)) != 0)
      NWCMemSet(statBuffer, 0x00, sizeof(statBuffer));

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdskch.c,v 1.7 1994/09/26 17:45:54 rebekah Exp $
*/

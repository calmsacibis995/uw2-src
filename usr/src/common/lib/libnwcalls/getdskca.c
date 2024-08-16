/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdskca.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetDiskCacheStats***********************************************
SYNTAX:  NWCCODE N_API NWGetDiskCacheStats
         (
            NWCONN_HANDLE conn,
            DSK_CACHE_STATS N_FAR * statBuffer
         );

REMARKS:

ARGS:
      >  conn
      <  statBuffer

INCLUDE: nwserverh.h

RETURN:

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 214  Read Disk Cache Statistics

CHANGES: 10 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDiskCacheStats
(
   NWCONN_HANDLE     conn,
   DSK_CACHE_STATS   N_FAR * statBuffer
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(statBuffer, 0x00, (nuint) sizeof(*statBuffer));

   return ((NWCCODE) NWNCP23s214ReadDiskCacheStats(&access,
               (pNWNCPDskCacheStats) statBuffer));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdskca.c,v 1.7 1994/09/26 17:45:53 rebekah Exp $
*/

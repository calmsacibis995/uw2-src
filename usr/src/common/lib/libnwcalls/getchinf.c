/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getchinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetCacheInfo****************************************************
SYNTAX:  NWCCODE N_API NWGetCacheInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_CACHE_INFO NWPTR fseCacheInfo
         )

REMARKS:

ARGS: >  conn
      <  fseCacheInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 01  Get Cache Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetCacheInfo
(
   NWCONN_HANDLE     conn,
   NWFSE_CACHE_INFO  NWPTR fseCacheInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s1GetCacheInfo(&access,
         (pNWNCPFSEVConsoleInfo)&fseCacheInfo->serverTimeAndVConsoleInfo,
         &fseCacheInfo->reserved,
         (pNWNCPFSECacheCounters) &fseCacheInfo->cacheCounters,
         (pNWNCPFSEMemoryCounters) &fseCacheInfo->cacheMemCounters,
         (pNWNCPFSETrendCounters) &fseCacheInfo->cacheTrendCounters,
         (pNWNCPFSECacheInfo) &fseCacheInfo->cacheInformation));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getchinf.c,v 1.7 1994/09/26 17:45:45 rebekah Exp $
*/


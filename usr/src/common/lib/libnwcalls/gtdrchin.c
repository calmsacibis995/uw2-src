/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtdrchin.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetDirCacheInfo*************************************************
SYNTAX:  NWCCODE N_API NWGetDirCacheInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_DIR_CACHE_INFO NWPTR fseDirCacheInfo
         )

REMARKS:

ARGS:  > conn
       < fseDirCacheInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 12  Get Directory Cache Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDirCacheInfo
(
   NWCONN_HANDLE        conn,
   NWFSE_DIR_CACHE_INFO NWPTR fseDirCacheInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s12GetDirCacheInfo(&access,
      (pNWNCPFSEVConsoleInfo) &fseDirCacheInfo->serverTimeAndVConsoleInfo,
      &fseDirCacheInfo->reserved,
      (pNWNCPFSEDirCacheInfo) &fseDirCacheInfo->dirCacheInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtdrchin.c,v 1.7 1994/09/26 17:46:58 rebekah Exp $
*/


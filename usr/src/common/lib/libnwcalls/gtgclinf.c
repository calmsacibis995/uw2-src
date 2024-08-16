/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtgclinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetGarbageCollectionInfo****************************************
SYNTAX:  NWCCODE N_API NWGetGarbageCollectionInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_GARBAGE_COLLECTION_INFO NWPTR fseGarbageCollectionInfo
         )

REMARKS:

ARGS: >  conn
      <  fseGarbageCollectionInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 07  Garbage Collection Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetGarbageCollectionInfo
(
   NWCONN_HANDLE conn,
   NWFSE_GARBAGE_COLLECTION_INFO NWPTR fseGarbageCollectionInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s7GetGarbCollectInfo(&access,
         (pNWNCPFSEVConsoleInfo)
         &fseGarbageCollectionInfo->serverTimeAndVConsoleInfo,
         &fseGarbageCollectionInfo->reserved,
         &fseGarbageCollectionInfo->failedAllocRequestCount,
         &fseGarbageCollectionInfo->numOfAllocs,
         &fseGarbageCollectionInfo->noMoreMemAvailableCount,
         &fseGarbageCollectionInfo->numOfGarbageCollections,
         &fseGarbageCollectionInfo->garbageFoundSomeMem,
         &fseGarbageCollectionInfo->garbageNumOfChecks));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtgclinf.c,v 1.7 1994/09/26 17:47:01 rebekah Exp $
*/


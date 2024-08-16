/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtmmoinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetMediaMgrObjInfo**********************************************
SYNTAX:  NWCCODE N_API NWGetMediaMgrObjInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        objNum,
            NWFSE_MEDIA_MGR_OBJ_INFO NWPTR fseMediaMgrObjInfo
         )

REMARKS:

ARGS:  > conn
       > objNum
       < fseMediaMgrObjInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 30  Get Media Manager Object Information

CHANGES: 22 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetMediaMgrObjInfo
(
  NWCONN_HANDLE            conn,
  nuint32                  objNum,
  NWFSE_MEDIA_MGR_OBJ_INFO NWPTR pFseMediaMgrObjInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP123s30GetMediaObjectInfo(&access, objNum,
      (pNWNCPFSEVConsoleInfo) &pFseMediaMgrObjInfo->serverTimeAndVConsoleInfo,
      &pFseMediaMgrObjInfo->reserved,
      (pNWNCPFSEMediaInfo) &pFseMediaMgrObjInfo->fseMMObjInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtmmoinf.c,v 1.7 1994/09/26 17:47:09 rebekah Exp $
*/


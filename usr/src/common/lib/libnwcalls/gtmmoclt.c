/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtmmoclt.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetMediaMgrObjChildrenList**************************************
SYNTAX:  NWCCODE N_API NWGetMediaMgrObjChildrenList
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            nuint32        objType,
            nuint32        parentObjNum,
            NWFSE_MEDIA_MGR_OBJ_LIST NWPTR fseMediaMgrObjList
         )

REMARKS:

ARGS: >  conn
      >  startNum
      >  objType
      >  parentObjNum
      <  fseMediaMgrObjList

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 32  Get Media Manager Object Children's List

CHANGES: 22 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetMediaMgrObjChildrenList
(
   NWCONN_HANDLE            conn,
   nuint32                  startNum,
   nuint32                  objType,
   nuint32                  parentObjNum,
   NWFSE_MEDIA_MGR_OBJ_LIST NWPTR fseMediaMgrObjList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s32GetMediaChildList(&access,
      startNum, objType, parentObjNum,
      (pNWNCPFSEVConsoleInfo) &fseMediaMgrObjList->serverTimeAndVConsoleInfo,
      &fseMediaMgrObjList->reserved,
      &fseMediaMgrObjList->nextStartObjNum,
      &fseMediaMgrObjList->objCount, fseMediaMgrObjList->objs));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtmmoclt.c,v 1.7 1994/09/26 17:47:08 rebekah Exp $
*/


/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gentinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWGetNSEntryInfo**************************************************
SYNTAX:  NWCCODE N_API NWGetNSEntryInfo
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         pbstrPath,
            nuint8         buNamSpc,
            nuint8         buDstNamSpc,
            nuint16        suSrchAttrs,
            nuint32        luRetMask,
            NW_ENTRY_INFO NWPTR pEntryInfo
         );

REMARKS:

ARGS:  > conn
       > dirHandle
       > pbstrPath
       > buNamSpc
       > buDstNamSpc
       > suSrchAttrs
       > luRetMask
      <  pEntryInfo

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 06  Obtain File or SubDirectory Information

CHANGES: 20 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNSEntryInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         pbstrPath,
   nuint8         buNamSpc,
   nuint8         buDstNamSpc,
   nuint16        suSrchAttrs,
   nuint32        luRetMask,
   NW_ENTRY_INFO NWPTR pEntryInfo
)
{
   NWNCPCompPath compPath;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   compPath.luDirBase = (nuint32) dirHandle;
   compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
   NWNCPPackCompPath(-1, pbstrPath, buNamSpc, &compPath, 0);

   if ((ccode = (NWCCODE) NWNCP87s6GetEntryInfo(&access, buNamSpc,
         buDstNamSpc, suSrchAttrs, luRetMask, &compPath,
         (pNWNCPEntryStruct) pEntryInfo)) == 0)
   {
      pEntryInfo->creatorID  = NSwap32(pEntryInfo->creatorID);
      pEntryInfo->modifierID = NSwap32(pEntryInfo->modifierID);
      pEntryInfo->archiverID = NSwap32(pEntryInfo->archiverID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gentinfo.c,v 1.7 1994/09/26 17:45:40 rebekah Exp $
*/

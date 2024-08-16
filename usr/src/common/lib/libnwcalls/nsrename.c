/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:nsrename.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwerror.h"
#include "nwnamspc.h"
#include "nwmisc.h"
#include "nwserver.h"

/*manpage*NWNSRename********************************************************
SYNTAX:  NWCCODE N_API NWNSRename
         (
            NWCONN_HANDLE conn,
            nuint8  dirHandle,
            nuint8  namSpc,
            pnstr8  oldName,
            nuint16 oldType,
            pnstr8  newName,
            nuint8  renameFlag
         )

REMARKS: Given a path specifying an entry name the entry will be renamed in
         the specified name space.

         The rename flag determines if the name is mangled or not.

         The dirHandle MUST point to the parent directory.

         The oldName and newName must be valid names containing only one
         component. No relative paths should be used.

ARGS: >  dirHandle
         The NetWare directory handle pointing to the parent directory.

      >  namSpc
         The name space of the oldName.

      >  oldName
         Pointer to the name of the directory or file to rename.

      >  oldType
         Whether destName is file (NW_TYPE_FILE) or subdirectory
         (NW_TYPE_SUBDIR).

      >  newName
         Pointer to the new name (256 bytes MAX).

      >  renameFlag
         Whether to mangle the name in other name spaces or not. This flag is
         only looked at for 3.2+ servers.  (NW_NAME_CONVERT) or
         (NW_NO_NAME_CONVERT). If NW_NO_NAME_CONVERT is NOT used,
         NW_NAME_CONVERT will be used by default, regardless of the value of
         this flag.

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 04  Rename or Move A File or SubDirectory

CHANGES: Art 9/22/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWNSRename
(
   NWCONN_HANDLE  conn,
   nuint8         dirHandle,
   nuint8         namSpc,
   pnstr8         oldName,
   nuint16        oldType,
   pnstr8         newName,
   nuint8         renameFlag
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWNCPCompPath2 cPath;
   nuint8 rFlag;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);


   if(!newName || !oldName)
      return (INVALID_FILENAME);

   if(!dirHandle)
      return (BAD_DIRECTORY_HANDLE);

   cPath.luSrcDirBase = cPath.luDstDirBase = (nuint32) dirHandle;
   cPath.buSrcHandleFlag = cPath.buDstHandleFlag = (nuint8) NWNCP_COMPPATH_USE_DIRHANDLE;

   NWNCPPackCompPath2(-1, oldName, namSpc, -1, newName, namSpc,
                        &cPath, (nflag32)0);

   if(cPath.buSrcCompCnt != 1)
      return (INVALID_FILENAME);

   if(cPath.buDstCompCnt != 1)
      return (INVALID_FILENAME);

   if(renameFlag == NW_NO_NAME_CONVERT)
   {
      if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
         return (ccode);

      rFlag   = (serverVer >= 3200) ? renameFlag : (nuint8) NW_NAME_CONVERT;
   }
   else
      rFlag   = NW_NAME_CONVERT;


   return ((NWCCODE) NWNCP87s4RenameMoveEntry(&access, namSpc, rFlag,
               oldType, &cPath));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/nsrename.c,v 1.7 1994/09/26 17:48:20 rebekah Exp $
*/

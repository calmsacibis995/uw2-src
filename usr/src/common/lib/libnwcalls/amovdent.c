/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:amovdent.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwncp.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwserver.h"
#include "nwmisc.h"
#include "nwdentry.h"
#include "nwnamspc.h"
#include "nwclocal.h"

/*manpage*NWIntMoveDirEntry*************************************************
SYNTAX:  NWCCODE N_API NWIntMoveDirEntry
         (
            NWCONN_HANDLE  conn,
            nuint8         srchAttr,
            NWDIR_HANDLE   srcDirHandle,
            pnstr8         srcPath,
            NWDIR_HANDLE   dstDirHandle,
            pnstr8         dstPath,
            nuint16        augmentFlag
         )

REMARKS: Moves or renames a directory entry (file or directory) on the same
         server. The move is performed by the server and requires no client
         intervention. This call does not cause the data itself to be moved,
         rather it causes the equivilant of a directory table entry to be
         reassigned.

         For 3.11 and above require that the srcDirHandle be pointed at
         least at the volume.

         For 2.2 this function will only work on files.

         The advantange of using this function over DOS, OS/2 or whatever
         client API, is that it is very quick and efficient.  Since the move
         is withing the server, all that happens is that the entry in the
         file system is deleted from the source, and inserted in the
         destination.

ARGS:

INCLUDE: nwdentry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     69 --  Rename File
         87 04  Rename or Move a File or Subdirectory

CHANGES: 9 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWIntMoveDirEntry
(
   NWCONN_HANDLE  conn,
   nuint8         srchAttr,
   NWDIR_HANDLE   srcDirHandle,
   pnstr8         srcPath,
   NWDIR_HANDLE   dstDirHandle,
   pnstr8         dstPath,
   nuint16        augmentFlag
)
{
   NWCCODE  ccode;
   nuint8   srcNamSpc, dstNamSpc;
   nuint16  serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110)
   {
      NWNCPCompPath2 compPath2;

      srcNamSpc =(nuint8) __NWGetCurNS(conn, srcDirHandle, srcPath);
      dstNamSpc =(nuint8) __NWGetCurNS(conn, dstDirHandle, dstPath);

      compPath2.luSrcDirBase = (nuint32) srcDirHandle;
      compPath2.luDstDirBase = (nuint32) dstDirHandle;
      compPath2.buSrcHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      compPath2.buDstHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      NWNCPPackCompPath2( -1, srcPath, srcNamSpc, -1,
               dstPath, dstNamSpc, &compPath2, (nflag32) augmentFlag);

      return ((NWCCODE) NWNCP87s4RenameMoveEntry(&access, srcNamSpc,
                        (nuint8) 0, (nuint16) srchAttr, &compPath2));
/*      return ((NWCCODE) NWNCP87s4RenameMoveEntry(&access, srcNamSpc,
                        (nuint8) 0, (nuint16) srchAttr, &compPath2,
                        augmentFlag ? NCP_AUGMENT : (nflag32) 0));
*/

   }
   else
   {
      nuint8 srcTPathLen, dstTPathLen;

      nuint8 asrcTPath[256];
      nuint8 adstTPath[256];

      NWCStrCpy(asrcTPath, srcPath);
      NWConvertAndAugment(asrcTPath, augmentFlag);

      NWCStrCpy(adstTPath, dstPath);
      NWConvertAndAugment(adstTPath, augmentFlag);

      srcTPathLen = (nuint8)NWCStrLen(asrcTPath);
      dstTPathLen = (nuint8)NWCStrLen(adstTPath);

/*      return ((NWCCODE) NWNCP69FileRename(&access, srcDirHandle, srchAttr,
                           srcTPathLen, asrcTPath, dstDirHandle,
                           dstTPathLen, adstTPath,
                           augmentFlag ? NCP_AUGMENT : (nflag32) 0));
*/
      return ((NWCCODE) NWNCP69FileRename(&access, srcDirHandle, srchAttr,
                           srcTPathLen, asrcTPath, dstDirHandle,
                           dstTPathLen, adstTPath));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/amovdent.c,v 1.8 1994/09/26 17:43:58 rebekah Exp $
*/


/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:modmaxrt.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwdirect.h"
#include "nwmisc.h"
#include "nwserver.h"

/*manpage*NWModifyMaximumRightsMask*****************************************
SYNTAX:  NWCCODE N_API NWModifyMaximumRightsMask
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint8         revokeRightsMask,
            nuint8         grantRightsMask
         )

REMARKS: Modifies a directory's maximum rights mask.

         The rights in the revokeRightsMask parameter are deleted from the
         directory's maximum rights mask, and the rights in the
         grantRightsMask parameter are added.

         The maximum rights mask can be completely reset by setting the
         revokeRightsMask parameter to 0xFF and then setting the
         grantRightsMask parameter to the desired maximum rights mask.

         Maximum rights affect the specified directory only, and are not
         inherited by subdirectories.

         To modify a directory's maximum rights mask, the requesting
         workstation must have parental rights to the directory.

ARGS:  > revokeRightsMask
         The rights mask of rights being revoked

       > grantRightsMask
         The rights mask of rights being granted

INCLUDE: nwdirect.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 04  Modify Maximum Rights Mask
         87 07  Modify File or SubDirectory DOS Information

CHANGES: 15 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWModifyMaximumRightsMask
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         revokeRightsMask,
   nuint8         grantRightsMask
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWNCPCompPath compPath;
   NWNCPModifyDosInfo  mdInfo;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   compPath.luDirBase = dirHandle;
   compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

   NWNCPPackCompPath(-1, path, -1, &compPath, (nflag32) 0);


   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      mdInfo.suFileAttrs= (nuint16) 0;
      mdInfo.buFileMode= (nuint8) 0;
      mdInfo.buFileXAttrs= (nuint8) 0;
      mdInfo.suCreationDate= (nuint16) 0;
      mdInfo.suCreationTime= (nuint16) 0;
      mdInfo.luCreatorID=   (nuint32) 0;
      mdInfo.suModifiedDate= (nuint16) 0;
      mdInfo.suModifiedTime= (nuint16) 0;
      mdInfo.luModifierID=  (nuint32) 0;
      mdInfo.suArchivedDate= (nuint16) 0;
      mdInfo.suArchivedTime= (nuint16) 0;
      mdInfo.luArchiverID= (nuint32) 0;
      mdInfo.suLastAccessDate= (nuint16) 0;
      mdInfo.suInheritanceGrantMask= (nuint16) grantRightsMask;
      mdInfo.suInheritanceRevokeMask=
            (nuint16) (revokeRightsMask|grantRightsMask);
      mdInfo.luMaxSpace= (nuint32) 0;

      ccode = (NWCCODE)NWNCP87s7EntrySetDOSInfo(&access, compPath.buNamSpc, (nuint8) 0,
            (nuint16) FA_DIRECTORY, (nuint32) 0x1000, &mdInfo, &compPath);
   }
   else ccode = (NWCCODE)NWNCP22s4DirModMaxRightsMask(&access, (nuint8) dirHandle,
                  grantRightsMask, revokeRightsMask,
                  (nuint8) NWCStrLen(path), path);

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/modmaxrt.c,v 1.7 1994/09/26 17:48:14 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwcaldef.h"
#include "nwnamspc.h"
#include "ncpfile.h"

/*manpage*NWSetNSEntryDOSInfo***********************************************
SYNTAX:  NWCCODE N_API NWSetNSEntryDOSInfo
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8  pbstrPath,
            nuint8  buNameSpace,
            nuint16 suSrchAttr,
            nuint32 luModifyDOSMask,
            MODIFY_DOS_INFO NWPTR dosInfo
         )

REMARKS: Modified information in one name space using a path from another.

ARGS: >  conn
      >  dirHandle
      >  pbstrPath
      >  buNameSpace
      >  suSrchAttr
         what search attributes to use. SA_ constants in nwnamspc.h

          SA_HIDDEN         0x0002
          SA_SYSTEM         0x0004
          SA_SUBDIR_ONLY    0x0010
          SA_SUBDIR_FILES   0x8000


      >  modifyDOSMask
         what information to return.

         DM_ATTRIBUTES             0x0002L
         DM_CREATE_DATE            0x0004L
         DM_CREATE_TIME            0x0008L
         DM_CREATOR_ID             0x0010L
         DM_ARCHIVE_DATE           0x0020L
         DM_ARCHIVE_TIME           0x0040L
         DM_ARCHIVER_ID            0x0080L
         DM_MODIFY_DATE            0x0100L
         DM_MODIFY_TIME            0x0200L
         DM_MODIFIER_ID            0x0400L
         DM_LAST_ACCESS_DATE       0x0800L
         DM_INHERITED_RIGHTS_MASK  0x1000L
         DM_MAXIMUM_SPACE          0x2000L

      >  dosInfo
         MODIFY_DOS_INFO structure. Only those fields which are related
         to the modifyInfoMask will be valid.

         typedef struct _MODIFY_DOS_INFO
         {
           nuint32 attributes;
           nuint16 createDate;
           nuint16 createTime;
           nuint32 creatorID;
           nuint16 modifyDate;
           nuint16 modifyTime;
           nuint32 modifierID;
           nuint16 archiveDate;
           nuint16 archiveTime;
           nuint32 archiverID;
           nuint16 lastAccessDate;
           nuint16 inheritanceGrantMask;
           nuint16 inheritanceRevokeMask;
           nuint32 maximumSpace;
         } MODIFY_DOS_INFO;

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 07  Modify File or SubDirectory DOS Information

CHANGES: 08 Jun 1992 - written - jwoodbur
         22 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetNSEntryDOSInfo
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8  pbstrPath,
   nuint8  buNameSpace,
   nuint16 suSrchAttr,
   nuint32 luModifyDOSMask,
   MODIFY_DOS_INFO NWPTR dosInfo
)
{
   NWNCPCompPath cPath;
   NWNCPModifyDosInfo dosInfo2;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   cPath.luDirBase    = (nuint32) dirHandle;
   cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
   NWNCPPackCompPath(-1, pbstrPath, -1, &cPath, (nflag32) 0);

   /* look out when this goes multi-platform */

   dosInfo2.suFileAttrs  = NGetLo16(dosInfo->attributes);
   dosInfo2.buFileMode   = NGetLo8(NGetHi16(dosInfo->attributes));
   dosInfo2.buFileXAttrs = NGetHi8(NGetHi16(dosInfo->attributes));

   dosInfo2.suCreationDate = dosInfo->createDate;
   dosInfo2.suCreationTime = dosInfo->createTime;
   dosInfo2.luCreatorID    = dosInfo->creatorID;
   dosInfo2.suModifiedDate = dosInfo->modifyDate;
   dosInfo2.suModifiedTime = dosInfo->modifyTime;
   dosInfo2.luModifierID   = dosInfo->modifierID;
   dosInfo2.suArchivedDate = dosInfo->archiveDate;
   dosInfo2.suArchivedTime = dosInfo->archiveTime;
   dosInfo2.luArchiverID   = dosInfo->archiverID;
   dosInfo2.suLastAccessDate        = dosInfo->lastAccessDate;
   dosInfo2.suInheritanceGrantMask  = dosInfo->inheritanceGrantMask;
   dosInfo2.suInheritanceRevokeMask = dosInfo->inheritanceRevokeMask;
   dosInfo2.luMaxSpace     = dosInfo->maximumSpace;

   return ((NWCCODE) NWNCP87s7EntrySetDOSInfo(&access, buNameSpace,
               (nuint8) 0, suSrchAttr, luModifyDOSMask, &dosInfo2, &cPath));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setinfo.c,v 1.7 1994/09/26 17:50:00 rebekah Exp $
*/

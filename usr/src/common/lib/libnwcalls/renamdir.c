/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:renamdir.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwnamspc.h"
#include "nwserver.h"

/*manpage*NWRenameDirectory*************************************************
SYNTAX:  NWCCODE N_API NWRenameDirectory
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         oldName,
            pnstr8         newName
         )

REMARKS:

ARGS:

INCLUDE: nwdirect.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 04  Rename or Move A File or Subdirectory
         22 15  Rename Directory

CHANGES: 16 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWRenameDirectory
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         oldName,
   pnstr8         newName
)
{
   pnstr8 tmpName;
   nuint16 serverVer;
   NWNCPCompPath2 compPath2;
   NWCCODE ccode;
   NW_IDX idxStruct;
   nuint8 nameSpace;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   /*  must strip any path info from the new directory name */
   for(tmpName = &newName[NWCStrLen(newName)];
      (*tmpName != '\\' && *tmpName != ':' && tmpName > newName);
      tmpName = NWPrevChar(newName, tmpName));

   if(*tmpName == '\\' || *tmpName == ':')
      tmpName++;

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110)
   {
      pnuint8 old;
      nstr8 temp[256];

      nameSpace = (nuint8) __NWGetCurNS(conn, dirHandle, oldName);

      NWCStrCpy(temp, oldName);

      for(old = (pnuint8)&temp[NWCStrLen((char *)temp)];
         (*old != '\\' && *old != ':' && old > (pnuint8)temp);
         old = (pnuint8)NWPrevChar(temp, (char *)old));

      if(*old == '\\' || *old == ':')
         old++;

      NWCStrCpy((char *)old, (char *)tmpName);
      tmpName = temp;

      if(!NWGetDirectoryBase(conn, dirHandle, tmpName, nameSpace,
                             &idxStruct))
         return(0x8992);

      compPath2.luSrcDirBase = (nuint32) dirHandle;
      compPath2.luDstDirBase = (nuint32) dirHandle;
      compPath2.buSrcHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      compPath2.buDstHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      NWNCPPackCompPath2( -1, oldName, nameSpace, -1,
               tmpName, nameSpace, &compPath2, (nflag32) 0);

      ccode = (NWCCODE)NWNCP87s4RenameMoveEntry(&access, nameSpace, (nuint8) 0,
               (nuint16) FA_DIRECTORY, &compPath2);
   }
   else
   {
      nstr8 tDirName[260], tname[260];

      NWCStrCpy(tDirName, oldName);
      NWConvertToSpecChar(tDirName);

      NWCStrCpy(tname, tmpName);
      NWConvertToSpecChar(tname);

      ccode = (NWCCODE)NWNCP22s15RenameDir(&access, (nuint8) dirHandle,
            (nuint8) NWCStrLen (tDirName), tDirName,
            (nuint8) NWCStrLen (tname), tname);

   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/renamdir.c,v 1.8 1994/09/26 17:49:04 rebekah Exp $
*/


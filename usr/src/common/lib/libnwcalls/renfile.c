/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:renfile.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwnamspc.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwfile.h"

/*manpage*NWRenameFile******************************************************
SYNTAX:  NWCCODE N_API NWRenameFile
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   oldDirHandle,
            pnstr8         oldFileName,
            nuint8         searchAttrs,
            NWDIR_HANDLE   newDirHandle,
            pnstr8         newFileName
         )

REMARKS: 69 -- -> This call allows a client to rename a file.  The source directory (where the
         file resides) and the target directory (where the renamed file will be
         deposited) do not need to be the same directory.  This call can be used to
         move a file from one directory to another.  However, the two directories
         must reside on the same server volume. This call will not move a file from
         one volume to another.

         The client must have file modification privileges in both the source and the
         target directories.  The rename attempt will fail if the file is being used
         by other clients or if the target name already exists in the target directory.
         Wildcard renaming is supported.

         This call is replaced by the NetWare 386 v3.11 call Rename or Move A File or
         Subdirectory (0x2222  87 04).

         87 04 -> This is a NetWare 386 v3.11 call that replaces the earlier call, Rename File
         (0x2222  69  --).

         The SearchAttributes field is explained in more detail in the Introduction
         to Directory Services.

         The SrcNWHandlePathS1 and DstNWHandlePathS2 structures are the same as the
         NWHandlePathStructure except that the PathInfo array is not contained in
         these structures.  This difference is because of space limitations.

         The SrcDstPathStrings array contains the source component path if present,
         and then the destination component path if present.  The component count in
         the SrcNWHandlePathS1 & DstNWHandlePathS2 is used to tell how many components
         for each respectively.

         The RenameFlag is set to 0 or 1 as explained below.

            bit 0 is set:    we will succeed on a rename to my old name.
            bit 1 is set:    we will set the compatibilite mode bit when renaming.

ARGS: >  conn,
      >  oldDirHandle,
      >  oldFileName,
      >  searchAttrs,
      >  newDirHandle,
      >  newFileName

INCLUDE: nwfile.h

RETURN:  0x0000  Successful
         0x8987  Create Filename Error
         0x898B  No Rename Privileges
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8991  Some Names Exist
         0x8992  All Names Exist
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899A  Rename Across Volume
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 04  Rename or Move A File or Subdirectory
         69 --  Rename File

CHANGES: 31 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWRenameFile
(
  NWCONN_HANDLE conn,
  NWDIR_HANDLE  oldDirHandle,
  pnstr8        oldFileName,
  nuint8        searchAttrs,
  NWDIR_HANDLE  newDirHandle,
  pnstr8        newFileName
)
{
   nuint16 serverVer;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      nuint8 oldNameSpace, newNameSpace;
      nuint16 srchAttrs;
      NWNCPCompPath2 compPath2;

      oldNameSpace =(nuint8) __NWGetCurNS(conn, oldDirHandle, oldFileName);
      newNameSpace =(nuint8) __NWGetCurNS(conn, newDirHandle, newFileName);

      compPath2.luSrcDirBase = (nuint32) oldDirHandle;
      compPath2.luDstDirBase = (nuint32) newDirHandle;
      compPath2.buSrcHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      compPath2.buDstHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      NWNCPPackCompPath2( -1, oldFileName, oldNameSpace, -1,
            newFileName, newNameSpace, &compPath2, (nflag32) 0);

      srchAttrs = (nuint16) searchAttrs;

      return ((NWCCODE) NWNCP87s4RenameMoveEntry(&access, oldNameSpace,
                       (nuint8) 0, srchAttrs, &compPath2));
   }
   else
   {
      nuint8 fileNameLen, newFileNameLen;
      nstr8  tempOldName[260], tempNewName[260];

      NWCStrCpy(&tempOldName[0],oldFileName);
      NWConvertToSpecChar(tempOldName);
      fileNameLen = (nuint8)NWCStrLen(tempOldName);

      NWCStrCpy(&tempNewName[0],newFileName);
      NWConvertToSpecChar(tempNewName);
      newFileNameLen = (nuint8)NWCStrLen(tempNewName);

      return ((NWCCODE) NWNCP69FileRename(&access, (nuint8) oldDirHandle,
                           searchAttrs, fileNameLen, tempOldName,
                           (nuint8) newDirHandle, newFileNameLen,
                           tempNewName));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/renfile.c,v 1.7 1994/09/26 17:49:08 rebekah Exp $
*/

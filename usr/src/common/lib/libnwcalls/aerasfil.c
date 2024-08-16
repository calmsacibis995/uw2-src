/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:aerasfil.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwfile.h"
#include "nwclocal.h"
#include "nwnamspc.h"
#include "nwserver.h"

#ifdef N_PLAT_OS2
#include "nwundoc.h"
#include "nwdirect.h"
#include <stdio.h>
#include <time.h>
#include "nwlocale.h"
#endif

/*manpage*NWIntEraseFiles***************************************************
SYNTAX:  NWCCODE N_API NWIntEraseFiles
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint8         searchAttrs,
            nuint16        augmentFlag
         );

REMARKS: Erases the specified files from the file server.

ARGS:  > conn
       > dirHandle
       > path
       > searchAttrs
         File attributes to use in finding the file
       > augmentFlag
         if set to USE_NW_WILD_MATCH wild card characters are augmented.

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 08  Delete A File or Subdirectory
         68 --  Erase File

CHANGES: 3 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
   Copyright (c) 1993 by Novell, Inc. All rights reserved.
****************************************************************************/
NWCCODE N_API NWIntEraseFiles
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         searchAttrs,
   nuint16        augmentFlag
)
{
#ifdef N_PLAT_OS2
   NW_FILE_INFO2 fileInfo;
   nstr8 saveChar;
   nuint8 seqNum[9];
   pnstr8 pathPtr;
   nuint16 fileCount;
#endif

   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = NWGetFileServerVersion(conn, &serverVer);
   if(ccode)
      return (ccode);

   if(serverVer >= 3110)
   {
      NWNCPCompPath cPath;

#ifdef N_PLAT_OS2
      nuint8  nameSpace;

      nameSpace = (nuint8)__NWGetCurNS(conn, dirHandle, path);

      if (nameSpace == NW_NS_OS2 && WildCardCheck((pnuint8) path))
      {
         for (pathPtr = (pnuint8)&path[NWCStrLen(path)];
               (pathPtr != (pnuint8)path) &&
               (*pathPtr != ':') &&
               (*pathPtr != '\\') &&
               (*pathPtr != '/');
               NWPrevChar(path, pathPtr));

         pathPtr = NWNextChar(pathPtr);

         for (pathPtr = (pnuint8)&path[NWCStrLen(path)];
               (pathPtr != (pnuint8)path) &&
               (*pathPtr != ':') &&
               (*pathPtr != '\\') &&
               (*pathPtr != '/');
               NWPrevChar(path, pathPtr));

         if(*pathPtr == ':')
            pathPtr = NWNextChar(pathPtr);

         saveChar = *pathPtr;
         *pathPtr = 0;

         ccode = NWAllocTemporaryDirectoryHandle(conn, dirHandle,
            path, &dirHandle, NULL);

         *pathPtr = saveChar;
         if (ccode)
            return (ccode);

         if (*pathPtr == '\\' || *pathPtr == '/')
            pathPtr = NWNextChar(pathPtr);

         for (ccode = fileCount = 0, *(pnuint32)seqNum = (nuint32)-1L;
              ccode == 0; fileCount++)
         {
            ccode = NWScanFileInformation2(conn, dirHandle, (pnstr8)pathPtr,
                        searchAttrs, seqNum, &fileInfo);
            if (ccode != 0)
            {
               NWDeallocateDirectoryHandle(conn, dirHandle);
               return ((fileCount != 0) ? 0 : ccode);
            }

            cPath.luDirBase = (nuint32) dirHandle;
            cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
            NWNCPPackCompPath(-1, fileInfo.fileName, -1, &cPath,
               augmentFlag);

            ccode = (NWCCODE) NWNCP87s8DelEntry(&access, cPath.buNamSpc,
               (nuint8) 0, (nuint16) searchAttrs, &cPath);
         }
      }
      else
#endif
      {
    /* This is the normal code path, when the server supports
     * wildcards in the name space, pull this code out of
     * the else statement.
     */

         cPath.luDirBase = (nuint32) dirHandle;
         cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
         NWNCPPackCompPath(-1, path, -1, &cPath, augmentFlag);

         ccode = (NWCCODE)NWNCP87s8DelEntry(&access, cPath.buNamSpc,
            (nuint8) 0, (nuint16) searchAttrs, &cPath);
      }
   }
   else
   {
      nstr8 abstrTempPath[256];

      NWCStrCpy(abstrTempPath, path);
      NWConvertAndAugment(abstrTempPath, augmentFlag);

      ccode = (NWCCODE)NWNCP68FileErase(&access, dirHandle, searchAttrs,
         (nuint8) NWCStrLen(abstrTempPath), abstrTempPath);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/aerasfil.c,v 1.7 1994/09/26 17:43:51 rebekah Exp $
*/


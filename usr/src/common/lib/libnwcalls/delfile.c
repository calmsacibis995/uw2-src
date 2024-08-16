/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:delfile.c	1.7"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"

#include "ncpfile.h"
#include "nwnamspc.h"
#include "nwfile.h"

/*manpage*NWDeleteNSEntry***************************************************
SYNTAX:  NWCCODE N_API NWDeleteNSEntry
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint8         searchAttr,
            pnstr8         path
         );

REMARKS: Erases the specified files from the file server.

ARGS: >  conn
      >  dirHandle
      >  path
      >  searchAttr
         File attributes to use in finding the file
      >  suAugmentFlag
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
NWCCODE N_API NWDeleteNSEntry
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         buNameSpace,
   nuint16        searchAttr
)
{
   NW_FILE_INFO2 fileInfo;
   nstr8 saveChar, seqNum[9];
   pnstr8 pathPtr;
   nuint16 suFileCount;
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

      if(buNameSpace != NW_NS_DOS && WildCardCheck((pnuint8) path))
      {
         for(pathPtr = &path[NWCStrLen(path)];
            (pathPtr != path) &&
            (*pathPtr != ':') &&
            (*pathPtr != '\\') &&
            (*pathPtr != '/');
            pathPtr = (pnstr8)NWPrevChar(path, pathPtr));

         pathPtr = (pnstr8)NWNextChar(pathPtr);

         for(pathPtr = &path[NWCStrLen(path)];
            (pathPtr != path) &&
            (*pathPtr != ':') &&
            (*pathPtr != '\\') &&
            (*pathPtr != '/');
            pathPtr = (pnstr8)NWPrevChar(path, pathPtr));

         if(*pathPtr == ':')
            pathPtr = (pnstr8)NWNextChar(pathPtr);

         saveChar = *pathPtr;
         *pathPtr = 0;

         ccode = NWAllocTemporaryDirectoryHandle(conn, dirHandle,
                        path, &dirHandle, NULL);
         if(ccode)
            return (ccode);

         *pathPtr = saveChar;

         if(*pathPtr == '\\' || *pathPtr == '/')
            pathPtr = (pnstr8)NWNextChar(pathPtr);

         for(suFileCount = 0, *(pnuint32)seqNum = (nuint32)-1L;; suFileCount++)
         {
            ccode = NWScanFileInformation2(conn,
                      dirHandle,  pathPtr, searchAttr,
                      (pnuint8)seqNum, &fileInfo);
            if(ccode != 0)
            {
               NWDeallocateDirectoryHandle(conn, dirHandle);
               return ((suFileCount != 0) ? 0 : ccode);
            }

            cPath.luDirBase = (nuint32) dirHandle;
            cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
            NWNCPPackCompPath(-1, fileInfo.fileName, buNameSpace, &cPath, 0);

            ccode = (NWCCODE)NWNCP87s8DelEntry(&access,
                                               cPath.buNamSpc,
                                               (nuint8)0,
                                               (nuint16)searchAttr,
                                               &cPath);
			if(ccode)
				return(ccode);
         }
      }
      else
      {
	 /* This is the normal code path, when the server supports
	  * wildcards in the name space, pull this code out of
	  * the else statement.
	  */

	 cPath.luDirBase = (nuint32) dirHandle;
	 cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
	 NWNCPPackCompPath(-1, path, buNameSpace, &cPath, 0);

	 ccode = (NWCCODE)NWNCP87s8DelEntry(&access,
                                      cPath.buNamSpc,
                                      (nuint8)0,
                                      (nuint16)searchAttr,
                                      &cPath);
      }
   }
   else
   {
      nstr8 tempPath[256];

      NWCStrCpy(tempPath, path);
      NWConvertAndAugment(tempPath, 0);

      ccode = (NWCCODE)NWNCP68FileErase(&access,
                                        dirHandle,
                                        searchAttr,
                                        (nuint8)NWCStrLen(tempPath),
                                        tempPath);
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/delfile.c,v 1.9 1994/09/26 17:45:07 rebekah Exp $
*/

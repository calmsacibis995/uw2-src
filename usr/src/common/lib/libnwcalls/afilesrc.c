/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:afilesrc.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwncp.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwfile.h"
#include "nwclocal.h"

/*manpage*NWFileSearchInitialize********************************************
SYNTAX:  NWCCODE N_API NWFileSearchInitialize
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            pnuint8        volNum,
            pnuint16       dirID,
            pnuint16       iterHnd,
            pnuint8        accessRights
         );

REMARKS: This function is used initialize a search for a file(s) on a server.

         If the directory handle is not known, then a value of 0 should be
         passed in. In the abscence of the directory handle, the path needs
         to specify the volume as well.  Wild cards are not allowed in the
         path for this call as the directory to set up to search must be 
         specified.

         The sequence number should initially be set to -1.

ARGS: >  conn
      >  dirHandle
      >  path
      <  volNum
      <  dirID
      <  iterHnd
      <  accessRights

INCLUDE: nwfile.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 14 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWFileSearchInitialize
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint8        volNum,
   pnuint16       dirID,
   pnuint16       iterHnd,
   pnuint8        accessRights
)
{

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWNCP62ScanFirst(&access, 
                                    dirHandle,
                                    (nuint8)NWCStrLen(path), 
                                    path,
                                    volNum,
                                    dirID, 
                                    iterHnd,
                                    accessRights));
}

/*manpage*NWIntFileSearchContinue*******************************************
SYNTAX:  NWCCODE N_API NWIntFileSearchContinue
         (
            NWCONN_HANDLE  conn,
            nuint8         volNum,
            nuint16        dirID,
            nuint16        searchContext,
            nuint8         searchAttrs,
            pnstr8         searchPath,
            pnstr8         returnBuffer,
            nuint16        augmentFlag
         )

REMARKS: Performs the actual search initialized with
         NWIntFileSearchInitialize.

         This function can be called iteratively to retrieve all directory
         entries matching the specified searchPath.

         The search path can contain wild cards.  Wild card matching is
         done using the matching methoid defined by the application. This
         is accomplished by passing the augmentFlag set to either
         USE_NW_WILD_MATCH or USER_DOS_WILD_MATCH.

         This function returns two different search structures depending on
         whether the match is a directory or a file:

         typedef struct SEARCH_FILE_INFO
         {
            nuint16 sequenceNumber;
            nuint16 reserved;
            nstr8   fileName[15];
            nuint8  fileAttributes;
            nuint8  fileMode;
            nuint32 fileLength;
            nuint16 createDate;
            nuint16 accessDate;
            nuint16 updateDate;
            nuint16 updateTime;
         } SEARCH_FILE_INFO;

         typedef struct SEARCH_DIR_INFO
         {
            nuint16 sequenceNumber;
            nuint16 reserved1;
            nstr8   directoryName[15];
            nuint8  directoryAttributes;
            nuint8  directoryAccessRights;
            nuint16 createDate;
            nuint16 createTime;
            nuint32 owningObjectID;
            nuint16 reserved2;
            nuint16 directoryStamp;
         } SEARCH_DIR_INFO;

         It is the applications responsibility to determine the type of
         match, or to limit the search to Files, or Directories Only.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     63 --  File Search Continue

CHANGES:  9 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWIntFileSearchContinue
(
   NWCONN_HANDLE  conn,
   nuint8         volNum,
   nuint16        dirID,
   nuint16        searchContext,
   nuint8         searchAttrs,
   pnstr8         searchPath,
   pnuint8        retBuf,
   nuint16        augmentFlag
)
{
   NWCCODE ccode;
   nstr8 srchPath[256];
   NWNCPSrchInfo tempInfo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(srchPath, searchPath);
   NWConvertAndAugment(srchPath, augmentFlag);

   if((ccode = (NWCCODE)NWNCP63ScanNext(&access, volNum, dirID,
               searchAttrs,(nuint8)NWCStrLen(srchPath),
               srchPath, &searchContext, &tempInfo)) == 0)
   {
      NCopy16(&retBuf[0], &searchContext);
      if(tempInfo.d.suDirStamp == 0xD1D1)
      {
         NCopy16(&retBuf[2], &tempInfo.d.suReserved1);
         NWCMemMove(&retBuf[4], tempInfo.d.abstrDirName, (nuint) 14);
         retBuf[18] = 0x00;  /* ensure null-termination of dir name */
         retBuf[19] = tempInfo.d.buDirAttributes;
         retBuf[20] = tempInfo.d.buDirAccessRights;
         NCopy16(&retBuf[21], &tempInfo.d.suCreateDate);
         NCopy16(&retBuf[23], &tempInfo.d.suCreateTime);
         NCopySwap32(&retBuf[25], &tempInfo.d.luOwningObjectID);
         NCopy16(&retBuf[29], &tempInfo.d.suReserved2);
         NCopy16(&retBuf[31], &tempInfo.d.suDirStamp);
      }
      else
      {
         NCopy16(&retBuf[2], &tempInfo.f.suReserved);
         NWCMemMove(&retBuf[4], tempInfo.f.abstrFileName, (nuint) 14);
         retBuf[18] = 0x00;  /* ensure null-termination of file name */
         retBuf[19] = tempInfo.f.buAttrs;
         retBuf[20] = tempInfo.f.buExeType;
         NCopy32(&retBuf[21], &tempInfo.f.luSize);
         NCopy16(&retBuf[25], &tempInfo.f.suCreationDate);
         NCopy16(&retBuf[27], &tempInfo.f.suAccessedDate);
         NCopy16(&retBuf[29], &tempInfo.f.suModifiedDate);
         NCopy16(&retBuf[31], &tempInfo.f.suModifiedTime);
      }
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/afilesrc.c,v 1.7 1994/09/26 17:43:54 rebekah Exp $
*/

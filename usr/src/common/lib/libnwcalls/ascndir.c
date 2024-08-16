/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ascndir.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwdirect.h"
#include "nwclocal.h"

/*manpage*NWIntScanDirectoryInformation*************************************
SYNTAX:  NWCCODE N_API NWIntScanDirectoryInformation
         (
              NWCONN_HANDLE conn,
              NWDIR_HANDLE dirHandle,
              pnstr8       srchPath,
              pnuint16     iterHandle,
              pnstr8       dirName,
              pnuint32     dirDateTime,
              pnuint32     ownerID,
              pnuint8      rightsMask,
              nuint16      augmentFlag
         )

REMARKS: Returns directory information for a directory
         specified by the server connection ID, NetWare directory handle,
         and directory path.

         The searchDirPath parameter can contain wildcard characters.

         The sequenceNumber parameter indicates the subdirectory number where
         the search for matching subdirectories will begin.  Initially, the
         sequenceNumber parameter should be zero.  The value returned here is
         then used for subsequent calls.

         The subdirectory number is an internal network number indicating
         the actual subdirectory slot of the directory.

ARGS: >  conn
      >  dirHandle
      >  srchPath
         Pointer to an absolute directory path (or one relative  to
         the directory handle) and search pattern (optional)

      <  iterHandle
         Pointer to the sequence number (initialized to 0 for the
         first call) to use for subsequent calls

      <  dirName (optional)
         Pointer to the name of the found directory (256-bytes)

      <  dirDateTime (optional)
         Pointer to the creation date and time of the directory

      <  ownerID (optional)
         Pointer to the bindery object ID of the directory's owner

      <  rightsMask (optional)
         Pointer to the maximum rights mask of the found directory

      >  augmentFlag
         if set to USE_NW_WILD_MATCH all wild card characters will be augmented.

INCLUDE: nwdirect.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 02  Scan Directory Information

CHANGES: 7 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved.
****************************************************************************/
NWCCODE N_API NWIntScanDirectoryInformation
(
   NWCONN_HANDLE conn,
   nuint8        dirHandle,
   pnstr8        srchPath,
   pnuint16      iterHandle,
   pnstr8        dirName,
   pnuint32      dirDateTime,
   pnuint32      ownerID,
   pnuint8       rightsMask,
   nuint16       augmentFlag
)
{
   NWCCODE ccode;
   nuint8 tempSrchPath[256];
   nuint16 creationDate, creationTime, tmpIterHnd;
   nuint8 pathLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(srchPath)
   {
      NWCStrCpy(&tempSrchPath[0], srchPath);
      NWConvertAndAugment(tempSrchPath, augmentFlag);
      pathLen = (nuint8)NWCStrLen(tempSrchPath);
   }
   else
      pathLen = 0;

   if (iterHandle && *iterHandle != 0)
      tmpIterHnd = NSwap16(*iterHandle);
   else
      tmpIterHnd = (nuint16) 1;

   if ((ccode = (NWCCODE)NWNCP22s2ScanDirInfo(&access, dirHandle, pathLen,
            tempSrchPath, &tmpIterHnd, dirName,
            &creationDate, &creationTime,
            ownerID, rightsMask, NULL)) == 0)
   {
      if (iterHandle)
         *iterHandle = NSwap16(tmpIterHnd + 1);

      /* this must be swapped anyway, so I just reversed time and date */
      if (dirDateTime)
         *dirDateTime = NMake32(creationTime, creationDate);

      if (ownerID)
         *ownerID = NSwap32(*ownerID);
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ascndir.c,v 1.8 1994/09/26 17:44:00 rebekah Exp $
*/


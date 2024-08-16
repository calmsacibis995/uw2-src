/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ocnsent.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwserver.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwnamspc.h"

/*manpage*NWOpenNSEntry******************************************************
SYNTAX:  NWCCODE N_API NWOpenNSEntry
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint8         buNameSpace,
            nuint8         buDataStream,
            pnstr8         pbstrFilePath,
            NW_NS_OPEN NWPTR pOpenInfo,
            NWFILE_HANDLE NWPTR fileHandle
         );

REMARKS:

ARGS:  > conn
       > dirHandle
       > buNameSpace
       > buDataStream
       > pbstrFilePath
      <> pOpenInfo
         Structure containing the information needed to create the entry.
         The results of a successful open/create will be returned in this
         structure.

         typedef struct
         {
            > nuint8  openCreateMode;
            > nuint16 searchAttributes;
              nuint32 reserved;
            > nuint32 createAttributes;
            > nuint16 accessRights;
           <  nuint32 NetWareHandle;
           <  nuint8  openCreateAction;
         } NW_NS_OPENCREATE

           > openCreateMode
             specifies whether to create, replace or open an entry
             (directories can only be created)
             uses the OC_MODE_ constants in nwnamspc.h

           > searchAttributes
              uses the SA_ constants in nwnamspc.h

           > createAttributes
             what attributes to set in the DOS name space. Uses the A_
             prefixed constants in nwnamspc.h

           > desiredAccessRights
             what access rights are desired. AR_READ_ONLY and/or
             AR_WRITE_ONLY MUST be used. If neither of these are used, this
             function will set both.  uses the AR_ constants in nwnamspc.h

          <  NetWareHandle
             a four byte netware handle

          <  openCreateAction
             the result of a successful open/create. Can be: no action
             taken, file open, file/subdirectory created, file replaced.
             uses the OC_ACTION_ constants in nwnamspc.h

      <  fileHandle
         OS file handle. When creating subdirectories this will return zero.

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 01  Open Create File or SubDirectory

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWOpenNSEntry
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint8         buNameSpace,
   nuint8         buDataStream,
   pnstr8         pbstrFilePath,
   NW_NS_OPEN NWPTR pOpenInfo,
   NWFILE_HANDLE NWPTR fileHandle
)
{
   nuint16 suServerVer;
   NWCCODE ccode;
   NWNCPCompPath cPath;
   NWNCPEntryStruct eInfo;
   nuint8  abuTempHandle[4];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if (!(pOpenInfo->accessRights & 3))
      pOpenInfo->accessRights |= 3;

   cPath.luDirBase = (nuint32) dirHandle;
   cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
   NWNCPPackCompPath(-1, pbstrFilePath, -1, &cPath, 0);

   if ((ccode = NWGetFileServerVersion(conn, &suServerVer)) != 0)
      return (ccode);

   if (suServerVer >= 4000)
   {
      ccode = (NWCCODE) NWNCP87s30OpenCreateFileOrDir(&access, buNameSpace,
                  buDataStream, pOpenInfo->openCreateMode, (nuint8) 0,
                  pOpenInfo->searchAttributes, (nuint16) 0, IM_SIZE,
                  pOpenInfo->createAttributes, pOpenInfo->accessRights,
                  &cPath, abuTempHandle, &pOpenInfo->openCreateAction,
                  NULL, &eInfo);

      if (ccode != 0)
         return (ccode);
   }
   else
   {
      ccode = (NWCCODE) NWNCP87s1EntryOpenCreate(&access, buNameSpace,
                  pOpenInfo->openCreateMode, pOpenInfo->searchAttributes, IM_SIZE,
                  pOpenInfo->createAttributes, pOpenInfo->accessRights,
                  &cPath, abuTempHandle, &pOpenInfo->openCreateAction,
                  NULL, &eInfo);

      if (ccode != 0)
         return (ccode);
   }

   NCopy32(&pOpenInfo->NetWareHandle, abuTempHandle);

   if((!(pOpenInfo->createAttributes & A_DIRECTORY)) && fileHandle)
   {
      if (pOpenInfo->NetWareHandle != 0)
      {
         ccode = NWConvertHandle(conn, (nuint8) pOpenInfo->accessRights,
                     abuTempHandle, 4, eInfo.luDataStreamSize, fileHandle);
         if(ccode)
         {
            nuint8   abuSixByteHandle[6];

            _NWConvert4ByteTo6ByteHandle(abuTempHandle, abuSixByteHandle);

            NWNCP66FileClose(&access, 0, abuSixByteHandle);
         }
      }
      else
         *fileHandle = 0;
   }

   return (ccode);
}

NWCCODE N_API NWOpenCreateNSEntry
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint8         buNameSpace,
   pnstr8         pbstrFilePath,
   NW_NS_OPENCREATE NWPTR pOpenInfoCreate,
   NWFILE_HANDLE NWPTR fileHandle
)
{
  return NWOpenNSEntry(conn, dirHandle, buNameSpace, 0, pbstrFilePath,
            pOpenInfoCreate, fileHandle);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ocnsent.c,v 1.7 1994/09/26 17:48:25 rebekah Exp $
*/

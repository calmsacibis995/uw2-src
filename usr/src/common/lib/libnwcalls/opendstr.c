/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:opendstr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwundoc.h"
#include "nwnamspc.h"
#include "nwmisc.h"

/*manpage*NWOpenDataStream**************************************************
SYNTAX:  NWCCODE N_API NWOpenDataStream
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8   pstrFileName,
            nuint16  suDataStream,
            nuint16  suAttributes,
            nuint16  suAccessMode,
            pnuint32 pluNWHandle,
            NWFILE_HANDLE NWPTR fileHandle
         )

REMARKS: Gets a NetWare file handle to a data stream.

         Currently there are 3 data streams defined for NetWare,

            0 - DOS data stream
            1 - MAC resource fork
            3 - FTAM data stream

         All name spaces can access their "main" data stream through the
         DOS data stream.  For example, the MAC data fork can be accessed
         through this data stream.  It would probably be impractical to
         access the data for the DOS data stream through this call, since
         that stream is available through the "open" functions for the
         particular client OS.

         This function exists mainly to allow access to the "unreachable"
         streams.

         For DOS the NetWare shell currently offers a function to get a DOS
         handle from a NetWare handle.

ARGS: >  conn
      >  dirHandle
      >  pstrFileName

      >  dataStream
         The data stream to open.

      >  attributes
         Attributes to use in searching for the file to open.

      >  accessMode
         The modes to use in opening the file.

      <  NetWareHandle (optional)
         Pointer returning a 4 byte NetWare handle to the data stream.

      <  fileHandle
         Pointer returning a DOS NetWare handle to the data stream.

INCLUDE: nwnamspc.h

RETURN:  0x0000  Successful
         0x8980  Lock Failure
         0x8982  No Open Privileges
         0x8990  Read-only Access To Volume
         0x89BE  Invalid Data Stream
         0x89FF  No Files Found

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 49  Open Data Stream

CHANGES: 13 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWOpenDataStream
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         pstrFileName,
   nuint16        suDataStream,
   nuint16        suAttrs,
   nuint16        suAccessMode,
   pnuint32       pluNWHandle,
   NWFILE_HANDLE  NWPTR fileHandle
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;
   nuint8 buNamSpc;
   nuint8 buFileNameLen;
   nuint32 luTempNWHandle;   /* just used as 4 byte mem space..
                             ..declared as long so we can check != 0 */

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   buFileNameLen = (nuint8)NWCStrLen(pstrFileName);

   if((ccode = (NWCCODE) NWNCP22s49OpenDataStream(&access,
         (nuint8) suDataStream, dirHandle, (nuint8) suAttrs,
         (nuint8) suAccessMode, buFileNameLen, pstrFileName,
         (pnuint8) &luTempNWHandle)) != 0)
   {
      return (ccode);
   }

   buNamSpc = (nuint8)__NWGetCurNS(conn, dirHandle, pstrFileName);

   if((ccode = NWGetNSEntryInfo(conn, dirHandle, pstrFileName, buNamSpc,
                  (nuint8)suDataStream, (nuint16)0x8000, (nuint32)IM_SIZE, &entryInfo)) != 0)
   {
      return (ccode);
   }

   if(pluNWHandle)
      *pluNWHandle = luTempNWHandle;

   if(luTempNWHandle != 0)
   {
      if((ccode = NWConvertHandle(conn, (nuint8) suAccessMode,
                  (pnuint8) &luTempNWHandle, (nuint16) 4,
                  entryInfo.dataStreamSize, fileHandle)) != 0)
      {
         nuint8   abuSixByteHandle[6];

         _NWConvert4ByteTo6ByteHandle((pnuint8)&luTempNWHandle, abuSixByteHandle);

         NWNCP66FileClose(&access, 0, abuSixByteHandle);
      }
   }
   else
      *fileHandle = 0;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/opendstr.c,v 1.7 1994/09/26 17:48:27 rebekah Exp $
*/

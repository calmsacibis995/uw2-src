/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:tmphand2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwcaldef.h"
#include "nwnamspc.h"
#include "nwserver.h"
#include "ncpfile.h"

/*manpage*NWAllocTempNSDirHandle2*******************************************
SYNTAX:  NWCCODE N_API NWAllocTempNSDirHandle2
         (
            NWCONN_HANDLE conn,
            nuint8  buDirHandle,
            pnstr8  pbstrPath,
            nuint8  buNameSpace,
            pnuint8 pbuNewDirHandle,
            nuint8  buNewNameSpace
         )

REMARKS: Asssigns a temporary directory handle for the given name space.

         The directory handles allocated by this call are automatically
         deallocated when the task terminates, or by a call to
         NWDeallocateDirectoryHandle.

ARGS: >  conn
      >  buDirHandle
      >  pbstrPath
      >  buNameSpace
      <  pbuNewDirHandle
      >  buNewNameSpace

INCLUDE: nwnamspc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 12  Allocate Short Directory Handle

CHANGES: 22 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAllocTempNSDirHandle2
(
   NWCONN_HANDLE conn,
   nuint8  buDirHandle,
   pnstr8  pbstrPath,
   nuint8  buNameSpace,
   pnuint8 pbuNewDirHandle,
   nuint8  buNewNameSpace
)
{
   NWCCODE ccode;
   nuint16 suServerVer;
   NWNCPCompPath cPath;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &suServerVer)) != 0)
      return ccode;

   if(suServerVer >= 3200)
   {
      cPath.luDirBase    = (nuint32) buDirHandle;
      cPath.buHandleFlag = (nuint8)  NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, pbstrPath, -1, &cPath, (nflag32) 0);

      ccode = (NWCCODE) NWNCP87s12AllocDirHandle(&access, buNameSpace,
                  buNewNameSpace, (nuint16) 0x8001, &cPath, pbuNewDirHandle,
                  NULL, NULL);
   }
   else
   {
      NW_ENTRY_INFO entryInfo;

      if((ccode = NWGetNSEntryInfo(conn, buDirHandle, pbstrPath, buNameSpace,
                                   buNewNameSpace, 0x8000, IM_DIRECTORY,
                                   &entryInfo)) != 0)
         return ccode;

      cPath.luDirBase    = (nuint32) entryInfo.dirEntNum;
      cPath.buVolNum     = (nuint8)  entryInfo.volNumber;
      cPath.buHandleFlag = (nuint8)  NWNCP_COMPPATH_USE_DIRBASE;
      NWNCPPackCompPath(-1, NULL, -1, &cPath, (nflag32) 0);

      ccode = (NWCCODE) NWNCP87s12AllocDirHandle(&access, buNewNameSpace,
                  (nuint8) 0, (nuint16) 0x0001, &cPath, pbuNewDirHandle, NULL,
                  NULL);
   }

   if(ccode)
      *pbuNewDirHandle = (nuint8) 0;

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/tmphand2.c,v 1.7 1994/09/26 17:50:15 rebekah Exp $
*/

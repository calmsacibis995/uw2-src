/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:openfile.c	1.5"
#include "nwclient.h"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#if defined N_PLAT_MSW
#include <windows.h>
#endif

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwerror.h"

/*manpage*NWOpenFile*******************************************************
SYNTAX:  NWCCODE N_API NWOpenFile(
            NWCONN_HANDLE conn,
            NWDIR_HANDLE	dirHandle,
            pnstr8		fileName,
            nuint16       searchAttributes,
            nuint8        accessRights,
            pnuint8       pbuNWHandle)

REMARKS: Open a file, get a 6 byte netware handle

INCLUDE: nwmisc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     66 Close File

CHANGES: 24 Aug 1992 - written - jwoodbur
         27 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWOpenFile(
  NWCONN_HANDLE   conn,
  NWDIR_HANDLE	   dirHandle,
  pnstr8	         fileName,
  nuint16         searchAttributes,
  nuint8          accessRights,
  NWFILE_HANDLE   NWPTR pFileHandle)
{
   NWCCODE        ccode;
   NWNCPFileInfo	fileInfo;
   nuint8         abuNWHandle[6];
   nuint8			nameLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   nameLen = (nuint8)NWCStrLen((char *)fileName);

   ccode = NWNCP76FileOpen(&access, (nuint8)dirHandle,
                     (nuint8)searchAttributes, accessRights, nameLen, fileName,
                     abuNWHandle, &fileInfo);

   if (ccode == 0)
   {
      ccode = NWConvertHandle(conn, accessRights, abuNWHandle, 6, 0, pFileHandle);
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/openfile.c,v 1.6 1994/09/26 17:48:29 rebekah Exp $
*/


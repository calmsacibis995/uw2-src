/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:purgdel.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwnamspc.h"
#include "nwdel.h"

/*manpage*NWPurgeDeletedFile************************************************
SYNTAX:  NWCCODE N_API NWPurgeDeletedFile
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint32        iterHandle,
            nuint32        volNum,
            nuint32        dirBase,
            pnstr8         fileName
         );

REMARKS: Permanently deletes recoverable files from a file server. For 2.x
         servers, all salvageable files, on all volNums on the specified file
         server are purged. For 3.x servers, only the specified file is purged.

         For 3.x servers, this call is used in connection with the
         NWScanForDeletedFiles30 function.  The sequence, volNum, and dirBase
         are returned by the Scan function, and should NOT be modified prior to
         calling this function.

         Valid parameters for each platform:

         2.x
            - conn

         3.0, 3.1
            - conn
            - dirHandle
            - iterHandle
            - fileName

         3.11
            - conn
            - dirHandle
            - iterHandle
            - volNum
            - dirBase

ARGS: >  conn
      >  dirHandle
      >  iterHandle
         Sequence number returned by NWScanForDeletedFiles.

      >  volNum
         volNum number returned by NWScanForDeletedFiles.

      >  dirBase
         dirBase number returned by NWScanForDeletedFiles.

      >  fileName
         Name of the file to purge.

INCLUDE: nwdel.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 29  Purge Salvageable File
         22 16  Purge Erased Files (old)
         87 18  Purge Salvageable File

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWPurgeDeletedFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint32        iterHandle,
   nuint32        volNum,
   nuint32        dirBase,
   pnstr8         fileName
)
{
   NWCCODE ccode;
   nuint16 server;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &server)) != 0)
      return ccode;

   if(server >= 3110)
   {
      nuint8 buNamSpc;

      buNamSpc = (nuint8) __NWGetCurNS(conn, dirHandle, fileName);

      return ((NWCCODE) NWNCP87s18DelPurge(&access, buNamSpc, (nuint8) 0,
                  iterHandle, volNum, dirBase));
   }
   else if(server >= 3000)
   {
      nstr8 file[300];

      NWCStrCpy(file, fileName);
      NWConvertToSpecChar(file);

      return ((NWCCODE) NWNCP22s29DelPurge(&access, (nuint8) dirHandle,
                  iterHandle));
   }
   else
   {
      return ((NWCCODE) NWNCP22s16DelPurge(&access));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/purgdel.c,v 1.7 1994/09/26 17:48:41 rebekah Exp $
*/

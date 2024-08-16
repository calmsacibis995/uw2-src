/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:recdel.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwdel.h"

/*manpage*NWRecoverDeletedFile**********************************************
SYNTAX:  NWCCODE N_API NWRecoverDeletedFile
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint32        iterHandle,
            nuint32        volNum,
            nuint32        dirBase,
            pnstr8         delFileName,
            pnstr8         rcvrFileName
         );

REMARKS:
         Recover a deleted file from the file server.  This function replaces
         NWRecoverSalvageableFile, and provides compatibility for the 2.x,
         and 3.x NetWare platforms.  Because of the compatibility issue, the
         path parameter previously used for NWRecoverSalvageableFile is no
         longer supported.

         For the 3.11 servers, the recovery is performed one file at a time,
         and the API can also recover the deleted file and give it a new
         name.  This feature was added to help alleviate problems when
         recovering a file, and a new file with the same name already exists.

         For 2.x server, we have the problem of not being able to scan for
         the deleted file.  In this case, the dirHandle parameter must
         contain a handle pointing to the directory containing the deleted
         file.  If a file with the same name already exists in that same
         directory, the server will give the recovered file an new name and
         return it to the caller.   NOTE:  because of earlier support for 14
         character names in NetWare, both of these buffers must be at least
         15 bytes long.  Also note that on 2.x servers, the deleteFileName is
         returned (rather than passed in) by the server, and this buffer
         cannot be null.

         FOR 2.X SERVERS ONLY THE LAST DELETED ENTRY IS RECOVERABLE.  Also
         this file is recoverable only until a client attempts another file
         erase, or file create request.

         The NWScanForDeletedFiles function returns all necessary information
         to be passed into this function.

         Valid parameters for each platform:

         2.x
            - conn
            - dirHandle
            - deletedFileName (return)
            - recoverFileName (return)

         3.0, 3.1
            - conn
            - dirHandle
            - sequence
            - recoverFileName (passed in)

         3.11
            - conn
            - dirHandle
            - sequence
            - volume
            - dirBase
            - recoverFileName

ARGS: >  conn
      >  dirHandle
      >  iterHandle
      >  volNum
      >  dirBase
      <  delFileName
      <> rcvrFileName (see above)

INCLUDE: nwdel.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 17  Recover Erased File (old)
         22 28  Recover Salvageable File
         87 17  Recover Salvageable File

CHANGES: 16 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWRecoverDeletedFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint32        iterHandle,
   nuint32        volNum,
   nuint32        dirBase,
   pnstr8         delFileName,
   pnstr8         rcvrFileName
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(!delFileName || !rcvrFileName)
      return (0x89FF);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      nuint8 buNamSpc;

      buNamSpc = (nuint8) __NWGetCurNS(conn, dirHandle, delFileName);

      return ((NWCCODE) NWNCP87s17DelRecover(&access, buNamSpc,
                  (nuint8) 0, iterHandle, volNum, dirBase,
                  (nuint8) NWCStrLen(rcvrFileName), rcvrFileName));
   }
   else if(serverVer >= 3000)
   {
      nstr8 pbstrTempName[260];

      NWCStrCpy(pbstrTempName, rcvrFileName);
      NWConvertToSpecChar(pbstrTempName);

      return ((NWCCODE) NWNCP22s28DelRecover(&access, (nuint8) dirHandle,
                  iterHandle, (nuint8) NWCStrLen(pbstrTempName),
                  pbstrTempName));
   }
   else
   {
      return ((NWCCODE) NWNCP22s17DelRecover(&access, (nuint8) dirHandle,
                  (nuint8) 0, NULL, delFileName, rcvrFileName));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/recdel.c,v 1.7 1994/09/26 17:48:54 rebekah Exp $
*/

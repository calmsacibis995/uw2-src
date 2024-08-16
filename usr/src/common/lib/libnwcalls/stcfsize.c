/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:stcfsize.c	1.4"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwerror.h"
#include "ncpfile.h"

/*manpage*NWSetCompressedFileSize*********************************************
SYNTAX:  NWCCODE N_API NWSetCompressedFileSize
         (
            NWCONN_HANDLE  conn,
            nuint32        fileHandle,
            nuint32        reqFileSize,
            pnuint32       resFileSize
         )

REMARKS: Attempts to set the "logical" file size for a compressed file.
         The logical file size is the "true" size of the file as reported
         by the client operating systems.  When a file is compressed, it
         "shrinks" in physical size, but its logical size should remain
         the same.  In cases where the client "forces" the creation of a
         compressed file (by opening a file in compressed mode), it needs
         to tell the NetWare OS what the actual size of the file is, this
         function is used for that purpose.


ARGS: >  conn
         if fileHandle contains a NetWare handle, this should contain the
         connection handle of the associated server. If NETX is running
         and a DOS file handle is passed, this must also contain a valid
         connection ID. In all other circumstances, this parameter is
         ignored.

      >  fileHandle
         an OS or NetWare file handle

      >  reqFileSize
         The requested file size.

      <  resFileSize
         The size actually assigned by the OS.

INCLUDE: nwfile.h

RETURN:  0x8801  INVALID_CONNECTION
         0x8988  INVALID_FILE_HANDLE

SERVER:  4.10

CLIENT:  DOS WIN OS2 NT

SEE:     n/a

NCP:     90 12  Set Compressed File Size

CHANGES: 23 Sep 1993 - written - anevarez
         10 Dec 1993 - NWNCP Enabled - alim
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetCompressedFileSize
(
   NWCONN_HANDLE  conn,
   nuint32        fileHandle,
   nuint32        reqFileSize,
   pnuint32       resFileSize
)
{
   NWCONN_HANDLE  conn2;
   NWCCODE        ccode;
   nuint8         tempHandleB6[6];
   NWCDeclareAccess(access);

   if((ccode = NWConvertFileHandle( (int)fileHandle, 4,
         (pnuint8) &fileHandle, &conn2)) != 0)
   {
     if(ccode == INVALID_CONNECTION)
       conn2 = conn;
     else
       return ccode;
   }

   NWCSetConn(access, conn2);

   tempHandleB6[0] =
   tempHandleB6[1] = 0;
   NCopyLoHi32(&tempHandleB6[2], &fileHandle);

   return ((NWCCODE) NWNCP90s12SetCompFileSize(&access, tempHandleB6,
      reqFileSize, NULL, resFileSize));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/stcfsize.c,v 1.6 1994/09/26 17:50:07 rebekah Exp $
*/

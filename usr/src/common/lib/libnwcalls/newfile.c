/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:newfile.c	1.4"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwerror.h"

/*manpage*NWSetFilePos*********************************************
SYNTAX:  NWCCODE N_API NWSetFilePos
         (
            NWFILE_HANDLE  fileHandle,
            nuint          mode,
            nuint32        filePos
         )

REMARKS:

ARGS: >

INCLUDE: nwfile.h

RETURN:  0x8988  INVALID_FILE_HANDLE

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     n/a

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetFilePos
(
   NWFILE_HANDLE  fileHandle,
   nuint          mode,
   nuint32        filePos
)
{
#ifdef   N_PLAT_UNIX
   NWCCODE              ccode = 0;
   UNIX_NWFILE_STRUCT   *unixHandle;
   nuint32              endOfFile;

   unixHandle = (UNIX_NWFILE_STRUCT  *)fileHandle;

   switch (mode)
   {
      case SEEK_FROM_BEGINNING:
         unixHandle->luOffset = filePos;
         break;

      case SEEK_FROM_CURRENT_OFFSET:
         unixHandle->luOffset = filePos + unixHandle->luOffset;
         break;

      case SEEK_FROM_END:
         ccode = NWGetEOF(fileHandle, &endOfFile);
         unixHandle->luOffset = filePos + endOfFile;
         break;
   }

   return(ccode);
#else
#error   Not defined for this platform
#endif
}

NWCCODE N_API NWGetFilePos
(
   NWFILE_HANDLE  fileHandle,
   pnuint32       filePos
)
{
#ifdef   N_PLAT_UNIX
   UNIX_NWFILE_STRUCT   *unixHandle;

   unixHandle = (UNIX_NWFILE_STRUCT  *)fileHandle;

   *filePos = unixHandle->luOffset;

   return(0);
#else
#error   Not defined for this platform
#endif
}

NWCCODE N_API NWCommitFile
(
   NWFILE_HANDLE  fileHandle
)
{
#ifdef   N_PLAT_UNIX
   return(0);
#else
#error   Not defined for this platform
#endif
}

NWCCODE N_API NWGetEOF
(
   NWFILE_HANDLE  fileHandle,
   pnuint32       EOF
)
{
#ifdef   N_PLAT_UNIX

   NWCCODE        ccode;
   NWCONN_HANDLE  conn;
   nuint8         NWHandle[6];
   NWCDeclareAccess(access);

   ccode = NWConvertFileHandle(fileHandle, 6, NWHandle, &conn);
   if (ccode)
      return(ccode);

   NWCSetConn(access, conn);
   ccode = NWNCP71FileGetSize(&access, 0, NWHandle, EOF);

   return(ccode);
#else
#error   Not defined for this platform
#endif
}

NWCCODE N_API NWSetEOF
(
   NWFILE_HANDLE  fileHandle,
   nuint32        EOF
)
{
#ifdef   N_PLAT_UNIX
   NWCCODE  ccode;
   nuint8   dummyBuf[1];

   ccode = NWSetFilePos(fileHandle, SEEK_FROM_BEGINNING, EOF);
   if (ccode)
      return(ccode);

   ccode = NWWriteFile(fileHandle, 0, dummyBuf);

   return(ccode);
#else
#error   Not defined for this platform
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/newfile.c,v 1.4 1994/09/26 17:48:18 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:servcopy.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwerror.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwfile.h"

/*manpage*NWFileServerFileCopy**********************************************
SYNTAX:  NWCCODE N_API NWFileServerFileCopy
         (
            NWFILE_HANDLE  srcFileHandle,
            NWFILE_HANDLE  dstFileHandle,
            nuint32        srcOffset,
            nuint32        dstOffset,
            nuint32        bytesToCopy,
            pnuint32       bytesCopied
         );

REMARKS: Copies a file from a source to a destination on the same file
         server. The copy will take place entirely on the target server
         resulting in fast results.

         The files used in this operation must have been created and opened
         prior to calling this function.

ARGS: >  srcFileHandle
         Handle for the src file

      >  dstFileHandle
         Handle for the dstination file

      >  srcOffset
         Start of the data in the src file

      >  dstOffset
         Start of the data in the dstination file

      >  bytesToCopy
         The number of bytes to copy

      <  bytesCopied (optional)
         Pointer to the actual number of bytes written

INCLUDE: nwfile.h

RETURN:  NOT_SAME_CONNECTION
           If the file handles do not reside on the same server

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     74 --  Copy From One File To Another

CHANGES: 19 Jun 1993 - modified - jwoodbur
           src/dstFileHandles changed to type NWFILE_HANDLE to make function
           portable. Also, NWConvertFileHandle is used to convert handle
           in OS/2, Windows and VLM also making function more portable.
         3 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWFileServerFileCopy
(
   NWFILE_HANDLE  srcFileHandle,
   NWFILE_HANDLE  dstFileHandle,
   nuint32        srcOffset,
   nuint32        dstOffset,
   nuint32        bytesToCopy,
   pnuint32       bytesCopied
)
{
#if defined(N_PLAT_DOS)
   if (_NWShellIsLoaded != _NETX_COM)
   {
#endif
      NWCCODE ccode;
      NWCONN_HANDLE srcConn, dstConn;
      nuint8 srcNWHandle[6], dstNWHandle[6];
      nuint32 bucket;
      pnuint32 bytesCopiedPtr;
      NWCDeclareAccess(access);

      if ((ccode = NWConvertFileHandle((NWFILE_HANDLEINT) srcFileHandle, 6,
                     srcNWHandle, &srcConn)) != 0)
         return (ccode);

      if ((ccode = NWConvertFileHandle((NWFILE_HANDLEINT) dstFileHandle, 6,
                     dstNWHandle, &dstConn)) != 0)
         return (ccode);

      if (srcConn != dstConn)
         return NOT_SAME_CONNECTION;

      NWCSetConn(access, srcConn);

      bytesCopiedPtr = bytesCopied ? bytesCopied : &bucket;

      ccode = (NWCCODE) NWNCP74FileCopy(&access, 0, srcNWHandle, dstNWHandle,
                  srcOffset, dstOffset, bytesToCopy,
                  bytesCopiedPtr);

      return (ccode);

#if defined(N_PLAT_DOS)
   }
   else
   {
      REGISTERS regs;
      struct tempStruct
      {
         nuint16 srcFileHandle;
         nuint16 dstFileHandle;
         nuint32 srcOffset;
         nuint32 dstOffset;
         nuint32 bytesToCopy;
      } info;

      if (srcFileHandle & 0x8000 || dstFileHandle & 0x8000)
         return(6);

      info.srcFileHandle   = srcFileHandle;
      info.dstFileHandle   = dstFileHandle;
      info.srcOffset   = srcOffset;
      info.dstOffset   = dstOffset;
      info.bytesToCopy = bytesToCopy;

      regs.b.ah          = 0xf3;
      regs.p.replyBuffer = &info;
      NWShellRequest(&regs, USE_ES);

      if (regs.b.al)
         return ((nuint16) (regs.b.al | 0x8900));

      if (bytesCopied)
         *bytesCopied = (nuint32) regs.w.cx | ((nuint32) regs.w.dx << 16);
   }

   return (0);

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/servcopy.c,v 1.7 1994/09/26 17:49:44 rebekah Exp $
*/

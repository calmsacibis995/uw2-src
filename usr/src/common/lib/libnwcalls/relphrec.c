/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:relphrec.c	1.5"
#ifdef N_PLAT_OS2
#include <os2def.h>
#include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwundoc.h"
#define  FILE_LOCKS_ONLY    /* only use the locks part of nwfile.h */
#include "nwmisc.h"

/*manpage*NWReleasePhysicalRecord*******************************************
SYNTAX:  NWCCODE N_API NWReleasePhysicalRecord
         (
            NWFILE_HANDLE fileHandle,
            nuint32  recStartOffset,
            nuint32  recSize
         )

REMARKS: This call releases a byte range previously locked by the calling client.
         The released byte range will remain in the client's logged data block
         table and will be re-locked by subsequent calls to Lock Physical Record
         Set (function 110).

         Released byte ranges must match locked byte ranges.  Releasing only a portion
         of a previously locked byte range is not allowed.  If the byte range being
         released overlaps any other byte range lock(s) still in effect, all but the
         overlapped bytes will be released.


ARGS: >  fileHandle
      >  recStartOffset
      >  recSize

INCLUDE: nwfile.h

RETURN:  0x0000  Successful
         0x89FF  Unlock Error

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     28 --  Release Physical Record

CHANGES: 2 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWReleasePhysicalRecord
(
   NWFILE_HANDLE fileHandle,
   nuint32  recStartOffset,
   nuint32  recSize
)
{
   NWCONN_HANDLE conn;
   NWCCODE ccode;
   nuint8 NWHandle[6];
   NWCDeclareAccess(access);

#ifdef N_PLAT_DOS
   if(_NWShellIsLoaded == _NETX_COM)
   {
      REGISTERS regs;

      if(fileHandle & 0x8000)
         return(6);

      regs.b.ah = 0xbe;
      regs.w.bx = fileHandle;
      regs.w.cx = (nuint16)(recStartOffset >> 16);
      regs.w.dx = (nuint16)recStartOffset;
      regs.w.si = (nuint16)(recSize >> 16);
      regs.w.di = (nuint16)recSize;
      NWShellRequest(&regs, 0);
      return(regs.b.al ? (nuint16) regs.b.al | 0x8900 : 0);
   }
#endif

   if((ccode = NWConvertFileHandle((NWFILE_HANDLEINT) fileHandle,
                                 6, NWHandle, &conn)) != 0)
      return ccode;

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP28SyncRelPhyRec(&access, (nuint8) 0, NWHandle,
               recStartOffset, recSize);

#ifdef N_PLAT_OS2
   if(ccode)
      return ccode;

   return NWClearLockSemList(0);
#else
   return ccode;
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/relphrec.c,v 1.7 1994/09/26 17:49:00 rebekah Exp $
*/

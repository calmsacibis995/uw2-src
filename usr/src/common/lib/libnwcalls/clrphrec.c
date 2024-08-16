/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clrphrec.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of file*/
#include "nwfile.h"
#include "nwmisc.h"

/*manpage*NWClearPhysicalRecord*********************************************
SYNTAX:  NWCCODE N_API NWClearPhysicalRecord
         (
            NWFILE_HANDLE  fileHandle,
            nuint32        recStartOffset,
            nuint32        recSize
         );

REMARKS: Clears a lock on a physical record, and removes it from the log
         table.

         A physical record lock, as opposed to a logical lock, is the
         actual lock of a specified record relative to a physical file.
         When a record is locked, it is also entered into a log table.
         Records are allowed to be locked only if all records in the log
         table are available for locking.  This is done to avoid deadlock.

ARGS: >  fileHandle
      >  recStartOffset
      >  recSize

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     30 --  Clear Physical Record

CHANGES: 22 Jun 1993 - rewrote - jwoodbur
           Now calls NWConvertFileHandle and is subsequently much more
           portable
         13 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWClearPhysicalRecord
(
   NWFILE_HANDLE  fileHandle,
   nuint32        recStartOffset,
   nuint32        recSize
)
{
   NWCCODE ccode;
   nuint8 NWHandle[6];
   NWCONN_HANDLE conn;
   NWCDeclareAccess(access);

#ifdef N_PLAT_DOS
   if (_NWShellIsLoaded == _NETX_COM)
   {
      REGISTERS regs;

      if(fileHandle & 0x8000)
         return (6);

      regs.b.ah = 0xbe;
      regs.w.bx = fileHandle;
      regs.w.cx = (nuint16) (recStartOffset >> 16);
      regs.w.dx = (nuint16) recStartOffset;
      regs.w.si = (nuint16) (recSize >> 16);
      regs.w.di = (nuint16) recSize;
      NWShellRequest(&regs, 0);

      return (regs.b.al ? (nuint16) (regs.b.al | 0x8900) : 0);
   }
#endif

   if ((ccode = NWConvertFileHandle((NWFILE_HANDLEINT) fileHandle,
                  (nuint) 6, NWHandle, &conn)) != 0)
      return (ccode);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP30SyncClrPhyRec(&access, 0, NWHandle, recStartOffset,
               recSize);

#ifdef N_PLAT_OS2
   if(ccode)
      return (ccode);

   return (NWClearLockSemList(0));
#else
   return (ccode);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clrphrec.c,v 1.7 1994/09/26 17:44:34 rebekah Exp $
*/

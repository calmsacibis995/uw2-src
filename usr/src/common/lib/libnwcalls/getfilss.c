/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getfilss.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileSystemStats**********************************************
SYNTAX:  NWCCODE N_API NWGetFileSystemStats
         (
            NWCONN_HANDLE conn,
            FILESYS_STATS N_FAR * statBuffer
         );

REMARKS:

ARGS: >  conn
      <  statBuffer

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 212  Get File System Statistics

CHANGES: 17 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileSystemStats
(
   NWCONN_HANDLE        conn,
   FILESYS_STATS N_FAR *  statBuffer
)
{
   NWCCODE ccode;
   NWNCPFSStats stats;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   NWCMemSet((nptr) statBuffer, 0x00, (nuint) sizeof(statBuffer));

   if ((ccode = (NWCCODE)NWNCP23s212GetFileSysStats(&access, &stats)) == 0)
   {
      statBuffer->systemElapsedTime    = stats.luSysIntervalMarker;
      statBuffer->maxOpenFiles         = stats.suConfigMaxOpenFiles;
      statBuffer->maxFilesOpened       = stats.suActualMaxOpenFiles;
      statBuffer->currOpenFiles        = stats.suCurrOpenFiles;
      statBuffer->totalFilesOpened     = stats.luTotalFilesOpened;
      statBuffer->totalReadRequests    = stats.luTotalReadReqs;
      statBuffer->totalWriteRequests   = stats.luTotalWriteReqs;
      statBuffer->currChangedFATSectors = stats.suCurrChangedFATs;
      statBuffer->totalChangedFATSectors = stats.luTotalChangedFATs;
      statBuffer->FATWriteErrors       = stats.suFATWriteErrors;
      statBuffer->fatalFATWriteErrors  = stats.suFatalFATWriteErrors;
      statBuffer->FATScanErrors        = stats.suFATScanErrors;
      statBuffer->maxIndexFilesOpened  = stats.suActualMaxIndexedFiles;
      statBuffer->currOpenIndexedFiles = stats.suActiveIndexedFiles;
      statBuffer->attachedIndexFiles   = stats.suAttachedIndexedFiles;
      statBuffer->availableIndexFiles  = stats.suAvailableIndexedFiles;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getfilss.c,v 1.7 1994/09/26 17:46:01 rebekah Exp $
*/

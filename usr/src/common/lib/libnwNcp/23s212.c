/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s212.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s212GetFileSysStats*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s212GetFileSysStats
         (
            pNWAccess pAccess,
            pNWNCPFSStats pFileSysStats,
         );

REMARKS: Returns statistics about a server's file system.  The call can be
         used repeatedly to get updated information.  The following information
         is returned:

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Configured Max Open Files indicates the number of files the server can
         open simultaneously.

         Actual Max Open Files indicates the number of files open
         simultaneously since the server was brought up.

         Current Open Files contains the number of files the server has open.
         This value exceeds the previously recorded Actual Max Open Files count.
         Current Open Files reflects both files that clients have open and any
         internal files, such as the bindery, that the file server has open.

         Total Files Opened contains the number of files the server has opened
         since the server was brought up.  If this number reaches 0xFFFFFFFF,
         it wraps back to zero.

         Total Read Requests contains the number of read requests the server
         has received since it was brought up.  If this number reaches
         0xFFFFFFFF, it wraps back to zero.

         Total Write Requests contains the number of write requests the server
         has received since it was brought up.  If this number reaches
         0xFFFFFFFF, it wraps back to zero.

         Current Changed FATs contains the number of current FAT sectors the
         file system has modified.  FAT sectors are modified when files are
         extended or truncated.

         Total Changed FATs contains the number of FAT sectors the file system
         has modified since it was brought up.

         FAT Write Errors contains the number of disk write errors that have
         occurred in writing FAT sectors to the disk.  All FAT sectors are
         duplicated on the disk, ensuring that a single disk failure will not result
         in the loss of the FAT tables.  If a FAT sector is lost, its duplicate is
         automatically used.  When a disk write error occurs in both the original
         and the duplicate of a FAT sector, a fatal error occurs.

         Fatal FAT Write Errors contains the number of disk write errors that
         occurred in both the original and duplicate copy of a FAT sector.  Fatal
         FAT write errors occur because the file system cannot recover the
         information required to determine where a given file resides on a
         volume.  However, since a copy of the FAT table is kept in memory, the
         file server continues to function.

         FAT Scan Errors contains the number of times an internally
         inconsistent state existed in the file system.

         Actual Max Indexed Files contains the number of indexed files active
         simultaneously in the server since it was brought up.

         Active Indexed Files contains the count of files that are currently active,
         open, and indexed.

         Attached Indexed Files contains the count of indexed files ready for
         indexing but not ready for use.  The attached index files are not
         currently open, but they have indexes built in memory.  These indexes
         are reused if the attached file is reopened, or rebuilt for other files if
         needed.

         Available Indexed Files contains the count of file indexes that are
         available for use.

ARGS: <> pAccess,
      <  pFileSysStats,

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  NetWare 286 v2.1 to v2.15

CLIENT:  DOS OS2 WIN

SEE:

NCP:     23 212  Get File System Statistics

CHANGES: 17 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s212GetFileSysStats
(
   pNWAccess    pAccess,
   pNWNCPFSStats pFileSysStats
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        212)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)          42)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REP_LEN, NULL);
   if (lCode == 0)
   {
      NCopyHiLo32(&pFileSysStats->luSysIntervalMarker, &abuReply[0]);
      NCopyHiLo16(&pFileSysStats->suConfigMaxOpenFiles, &abuReply[4]);
      NCopyHiLo16(&pFileSysStats->suActualMaxOpenFiles, &abuReply[6]);
      NCopyHiLo16(&pFileSysStats->suCurrOpenFiles, &abuReply[8]);
      NCopyHiLo32(&pFileSysStats->luTotalFilesOpened, &abuReply[10]);
      NCopyHiLo32(&pFileSysStats->luTotalReadReqs, &abuReply[14]);
      NCopyHiLo32(&pFileSysStats->luTotalWriteReqs, &abuReply[18]);
      NCopyHiLo16(&pFileSysStats->suCurrChangedFATs, &abuReply[22]);
      NCopyHiLo32(&pFileSysStats->luTotalChangedFATs, &abuReply[24]);
      NCopyHiLo16(&pFileSysStats->suFATWriteErrors, &abuReply[28]);
      NCopyHiLo16(&pFileSysStats->suFatalFATWriteErrors, &abuReply[30]);
      NCopyHiLo16(&pFileSysStats->suFATScanErrors, &abuReply[32]);
      NCopyHiLo16(&pFileSysStats->suActualMaxIndexedFiles, &abuReply[34]);
      NCopyHiLo16(&pFileSysStats->suActiveIndexedFiles, &abuReply[36]);
      NCopyHiLo16(&pFileSysStats->suAttachedIndexedFiles, &abuReply[38]);
      NCopyHiLo16(&pFileSysStats->suAvailableIndexedFiles, &abuReply[40]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s212.c,v 1.7 1994/09/26 17:36:09 rebekah Exp $
*/

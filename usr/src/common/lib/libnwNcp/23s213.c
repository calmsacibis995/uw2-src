/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s213.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP23s213GetTrackingStats*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s213GetTrackingStats
         (
            pNWAccess  pAccess,
            pNWNCPTTSStats pStats,
         );

REMARKS: Returns statistical information about a file server's transaction
         tracking system.

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Transaction Tracking Supported indicates whether the server supports
         transaction tracking.  If this field contains a zero, the file server is not
         an SFT file server, and the contents of all the other fields returned by
         this function will be undefined.

         Transaction Tracking Enabled indicates whether the server's transaction
         tracking system is enabled.  Transaction tracking is disabled when the
         volume on which the transaction files reside is filled or when a call to
         Disable Transaction Tracking (function 23, subfuctnion 207) is made.

         Transaction Volume Number identifies the transaction's volume in the
         Volume Table maintained by the file server.  The Volume Table contains
         information about each volume on the file server.  A file server running
         NetWare v2.1 and above can accommodate up to 32 volumes (0..31).

         Configured Max Simultaneous Transactions indicates the maximum
         number of transactions that the server can track simultaneously.

         Actual Max Simultaneous Transactions indicates the maximum number
         of transactions that have occurred simultaneously since the server was
         brought up.

         Current Transaction Count indicates the number of transactions in
         progress.

         Total Transactions Performed indicates the total transactions performed
         by the server since it was brought up.  When this value reaches
         0xFFFFFFFF, it wraps back to zero.

         Total Write Transactions Performed indicates the total number of
         transactions that required the file server to track file changes.  If a
         workstation requests a transaction but does not actually modify (write) a
         file, then the transaction tracking software ignores the transaction.

         Total Transactions Backed Out indicates the total number of
         transactions that the transaction tracking system has backed out since
         the file server was brought up.  Backouts occur if a workstation requests
         a backout or if a workstation fails during a transaction.  The value in
         this field includes backout requests the file server could not perform
         because transaction tracking was disabled.

         Total Unfilled Backout Requests indicates the number of transaction
         backout requests that failed because transaction tracking was disabled.

         Transaction Disk Space indicates the number of disk blocks being used
         by the transaction tracking software (1 block = 4,096 bytes).

         Transaction FAT Allocations indicates the number of blocks that have
         been allocated for FATs of files being tracked since the server was
         brought up (1 block = 4,096 bytes).

         Transaction File Size Changes indicates the number of times files being
         tracked changed their size within a transaction since the server was
         brought up.

         Transaction Files Truncated indicates the number of times files being
         tracked have been truncated within a transaction since the server was
         brought up.

         The following information is repeated the number of times indicated by
         Number Of Entries.

         Connection Number indicates which logical connection is involved in
         a transaction.

         Task Number indicates which task within a logical connection is
         involved in a transaction.

ARGS: <> pAccess
      <  pStats

INCLUDE: ncptts.h

RETURN:  0x8900   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 207  Disable Transaction Tracking
         23 208  Enable Transaction Tracking

NCP:     23 213  Get Transaction Tracking Statistics

CHANGES: 18 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s213GetTrackingStats
(
   pNWAccess  pAccess,
   pNWNCPTTSStats pStats
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        213)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)         513)

   NWRCODE rcode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   rcode = NWCRequestSingle(pAccess, NCP_FUNCTION , abuReq, NCP_REQ_LEN,
               abuReply, NCP_REP_LEN, NULL);
   if (rcode == 0)
   {
      nint i, j;

      NCopyHiLo32(&pStats->luSysElapsedTime, &abuReply[0]);
      pStats->buSupported = abuReply[4];
      pStats->buEnabled = abuReply[5];
      NCopyHiLo16(&pStats->suVolumeNum, &abuReply[6]);
      NCopyHiLo16(&pStats->suMaxOpenTrans, &abuReply[8]);
      NCopyHiLo16(&pStats->suMaxTransOpened, &abuReply[10]);
      NCopyHiLo16(&pStats->suCurrTransOpen, &abuReply[12]);
      NCopyHiLo32(&pStats->luTotalTrans, &abuReply[14]);
      NCopyHiLo32(&pStats->luTotalWrites, &abuReply[18]);
      NCopyHiLo32(&pStats->luTotalBackouts, &abuReply[22]);
      NCopyHiLo16(&pStats->suUnfilledBackouts, &abuReply[26]);
      NCopyHiLo16(&pStats->suDiskBlocksInUse, &abuReply[28]);
      NCopyHiLo32(&pStats->luFATAllocations, &abuReply[30]);
      NCopyHiLo32(&pStats->luFileSizeChanges, &abuReply[34]);
      NCopyHiLo32(&pStats->luFilesTruncated, &abuReply[38]);
      pStats->buNumTransactions = abuReply[42];

      for (i = 0, j = 43; i < (nint) pStats->buNumTransactions; i++, j += 2)
      {
         pStats->connTask[i].buConnNumber = abuReply[j];
         pStats->connTask[i].buTaskNumber = abuReply[j+1];
      }
   }

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s213.c,v 1.7 1994/09/26 17:36:11 rebekah Exp $
*/

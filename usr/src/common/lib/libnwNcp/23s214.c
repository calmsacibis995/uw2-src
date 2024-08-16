/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s214.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s214ReadDiskCacheStats**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s214ReadDiskCacheStats
         (
            pNWAccess pAccess,
            pNWNCPDskCacheStats pStats,
         );

REMARKS: This call returns statistical information about a file server's caching.  A
         client must have console operator rights to make this call.

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Cache Buffer Count indicates the number of cache buffers in the server.

         Cache Buffer Size indicates the number of bytes in a cache buffer.

         Dirty Cache Buffers indicates the number of cache buffers in use.

         Cache Read Requests indicates the number of times the cache software
         received a request to write data to the disk.

         Cache Write Requests indicates the number of times the cache software
         received a request to write data to the disk.

         Cache Hits indicates the number of times cache requests were serviced
         from existing cache blocks.

         Cache Misses indicates the number of times cache requests could not be
         serviced from existing cache blocks.

         Physical Read Requests indicates the number of times the cache
         software issued a physical read request to a disk driver.  (A physical
         read request reads in as much data as the cache block holds.)

         Physical Write Requests indicates the number of times the cache
         software issued a physical write request to a disk driver.

         Physical Read Errors indicates the number of times the cache software
         received an error from the disk driver on a disk read request.

         Physical Write Errors indicates the number of times the cache software
         received an error from the disk driver on a disk write request.

         Cache Get Requests indicates the number of times the cache software
         received a request to read information from the disk.

         Cache Full Write Requests indicates the number of times the cache
         software was requested to write information to disk that exactly filled
         one or more sectors.

         Cache Partial Write Requests indicates the number of times the cache
         software was requested to write information to disk that did not exactly
         fill a sector.  (Partial write requests require a disk preread.)

         Background Dirty Writes indicates the number of times a cache block
         that was written to disk was completely filled with information.  (The
         whole cache block was written.)

         Background Aged Writes indicates the number of times the background
         disk write process wrote a partially filled cache block to disk.  (The cache
         block was written to disk because the block had not been accessed for a
         significant period of time.)

         Total Cache Writes indicates the total number of cache buffers written
         to disk.

         Cache Allocations indicates the number of times a cache block was
         allocated for use.

         Thrashing Count indicates the number of times a cache block was not
         available when a cache block allocation was requested.

         LRU Block Was Dirty indicates the number of times the Least-Recently-
         Used cache block allocation algorithm reclaimed a dirty cache block.

         Read Beyond Write indicates the number of times a file read request
         was received for data not yet written to disk (due to file write requests
         that had not yet filled the cache block).  (This requires a disk preread.)

         Fragment Write Occurred indicates the number of times a dirty cache
         block contained noncontiguous sectors of information to be written, and
         the skipped sectors were not preread from the disk.  (Multiple disk
         writes were issued to write out the cache buffer.)

         Cache Hit On Unavailable Block indicates the number of times a cache
         request could be serviced from an available cache block but the cache
         buffer could not be used because it was in the process of being written to
         or read from disk.

         Cache Block Scrapped contains the number of times a cache block was
         scrapped.  The following describes why a cache block is scrapped:

         A process is put to sleep because it needs a cache block and the least-
         recently-used (LRU) cache block had to be written to disk before it could
         be reused.  When the process is awakened after the LRU cache block has
         been written and freed, the process discovers that while it was asleep a
         second process has allocated a different cache block into which it (the
         second process) has read the information needed by the first (sleeping)
         process.  Now, in order for the first process to access the information in
         the cache block allocated by the second process, the first process must
         free ("scrap") its cache block.


ARGS: <> pAccess
      <  pStats

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 214  Read Disk Cache Statistics

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s214ReadDiskCacheStats
(
   pNWAccess             pAccess,
   pNWNCPDskCacheStats  pStats
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        214)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)          78)

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
      NCopyHiLo32(&pStats->luSysElapsedTime, &abuReply[0]);
      NCopyHiLo16(&pStats->suCacheBufCount, &abuReply[4]);
      NCopyHiLo16(&pStats->suCacheBufSize, &abuReply[6]);
      NCopyHiLo16(&pStats->suDirtyCacheBufs, &abuReply[8]);
      NCopyHiLo32(&pStats->luCacheReadReqs, &abuReply[10]);
      NCopyHiLo32(&pStats->luCacheWriteReqs, &abuReply[14]);
      NCopyHiLo32(&pStats->luCacheHits, &abuReply[18]);
      NCopyHiLo32(&pStats->luCacheMisses, &abuReply[22]);
      NCopyHiLo32(&pStats->luPhysReadReqs, &abuReply[26]);
      NCopyHiLo32(&pStats->luPhysWriteReqs, &abuReply[30]);
      NCopyHiLo16(&pStats->suPhysReadErrors, &abuReply[34]);
      NCopyHiLo16(&pStats->suPhysWriteErrors, &abuReply[36]);
      NCopyHiLo32(&pStats->luCacheGetReqs, &abuReply[38]);
      NCopyHiLo32(&pStats->luCacheFullWriteReqs, &abuReply[42]);
      NCopyHiLo32(&pStats->luCachePartialWriteReqs, &abuReply[46]);
      NCopyHiLo32(&pStats->luBackgroundDirtyWrites, &abuReply[50]);
      NCopyHiLo32(&pStats->luBackgroundAgedWrites, &abuReply[54]);
      NCopyHiLo32(&pStats->luTotCacheWrites, &abuReply[58]);
      NCopyHiLo32(&pStats->luCacheAllocs, &abuReply[62]);
      NCopyHiLo16(&pStats->suThrashingCount, &abuReply[66]);
      NCopyHiLo16(&pStats->suLRUBlockWasDirtyCount, &abuReply[68]);
      NCopyHiLo16(&pStats->suReadBeyondWriteCount, &abuReply[70]);
      NCopyHiLo16(&pStats->suFragmentedWriteCount, &abuReply[72]);
      NCopyHiLo16(&pStats->suCacheHitOnUnavailCount, &abuReply[74]);
      NCopyHiLo16(&pStats->suCacheBlockScrappedCount, &abuReply[76]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s214.c,v 1.7 1994/09/26 17:36:12 rebekah Exp $
*/

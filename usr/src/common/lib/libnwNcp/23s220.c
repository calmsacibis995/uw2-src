/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s220.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s220GetConnsUsingAFile*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s220GetConnsUsingAFile
         (
            pNWAccess                pAccess,
            pnuint16                psuIterHnd,
            nuint8                  buDirHandle,
            nuint8                  buPathLen,
            pnstr8                  pbstrPath,
            pnuint16                psuUseCnt,
            pnuint16                psuOpenCnt,
            pnuint16                psuOpenForRead,
            pnuint16                psuOpenForWrite,
            pnuint16                psuDenyReadCnt,
            pnuint16                psuDenyWriteCnt,
            pnuint8                 pbuLocked,
            pnuint8                 pbuNumRecs,
            pNWNCPConnUsingFile2x   pConnsB70
         );

REMARKS: This call returns all logical connections using the file specified by
         Directory Handle and File Path.  The call must be repeated until Next
         Request Record (returned in the reply message) is 0.

         Use Count indicates the number of tasks that have opened or logged the
         file.

         Open Count indicates the number of tasks that have the file open.

         Open For Read Count indicates the number of logical connections that
         have the file open for reading.

         Open For Write Count indicates the number of logical connections that
         have the file open for writing.

         Deny Read Count indicates the number of logical connections that have
         denied other connections the right the read from the file.

         Deny Write Count indicates the number of logical connections that have
         denied other connections the right to write to the file.

         Next Request Record is the value that Last Record Seen must be set to
         in the next call.

         Locked indicates whether the file is locked exclusively (0 = not locked
         exclusively).

         The following information is repeated Number Of Records times.  There
         is one record for every connection that has used or has tried to use the
         specified file.

         Connection Number indicates the logical connection number of the
         workstation using the file.

         Task Number indicates the task number using the file.

         Lock Type contains bit flags indicating the file's lock status:

               7 6 5 4 3 2 1 0
               x x x x x x x 1  Locked
               x x x x x x 1 x  Open shareable
               x x x x x 1 x x  Logged
               x x x x 1 x x x  Open normal
               x 1 x x x x x x  TTS holding lock
               1 x x x x x x x  Transaction flag set on this file

         Access Control contains the bit flags indicating the connection's
         access rights for the file as follows:

               7 6 5 4 3 2 1 0
               x x x x x x x 1  Open for read by this client
               x x x x x x 1 x  Open for write by this client
               x x x x x 1 x x  Deny read requests from other stations
               x x x x 1 x x x  Deny write requests from other stations
               x x x 1 x x x x  File detached
               x x 1 x x x x x  TTS holding detach
               x 1 x x x x x x  TTS holding open

         Lock Flag indicates the type of lock on the file as follows:

               0x00             Not locked
               0xFE             Locked by a file lock
               0xFF             Locked by Begin Share File Set

ARGS: <> pAccess
      <> psuIterHnd
      >  buDirHandle
      >  buPathLen
      >  pbstrPath
      <  psuUseCnt (optional)
      <  psuOpenCnt (optional)
      <  psuOpenForRead (optional)
      <  psuOpenForWrite (optional)
      <  psuDenyReadCnt (optional)
      <  psuDenyWriteCnt (optional)
      <  pbuLocked (optional)
      <  pbuNumRecs (optional)
      <  pConnsB70 (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 220  Get Connections Using A File

CHANGES: 31 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
    Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s220GetConnsUsingAFile
(
   pNWAccess                pAccess,
   pnuint16                psuIterHnd,
   nuint8                  buDirHandle,
   nuint8                  buPathLen,
   pnstr8                  pbstrPath,
   pnuint16                psuUseCnt,
   pnuint16                psuOpenCnt,
   pnuint16                psuOpenForRead,
   pnuint16                psuOpenForWrite,
   pnuint16                psuDenyReadCnt,
   pnuint16                psuDenyWriteCnt,
   pnuint8                 pbuLocked,
   pnuint8                 pbuNumRecs,
   pNWNCPConnUsingFile2x   pConnsB70
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        220)
   #define NCP_STRUCT_LEN     ((nuint16) (buPathLen + 5))
   #define NCP_REQ_LEN        ((nuint)           7)
   #define NCP_REP_LEN        ((nuint)         436)
   #define NCP_REQ_FRAGS      ((nuint)           2)
   #define NCP_REP_FRAGS      ((nuint)           1)

   NWRCODE rcode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], psuIterHnd);
   abuReq[5] = buDirHandle;
   abuReq[6] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   rcode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, replyFrag, NULL);
   if (rcode == 0)
   {
      nint i;
      nuint8 buTemp;

      if (psuUseCnt)
         NCopyHiLo16(psuUseCnt, &abuReply[0]);
      if (psuOpenCnt)
         NCopyHiLo16(psuOpenCnt, &abuReply[2]);
      if (psuOpenForRead)
         NCopyHiLo16(psuOpenForRead, &abuReply[4]);
      if (psuOpenForWrite)
         NCopyHiLo16(psuOpenForWrite, &abuReply[6]);
      if (psuDenyReadCnt)
         NCopyHiLo16(psuDenyReadCnt, &abuReply[8]);
      if (psuDenyWriteCnt)
         NCopyHiLo16(psuDenyWriteCnt, &abuReply[10]);
      if (psuIterHnd)
         NCopyHiLo16(psuIterHnd, &abuReply[12]);
      if (pbuLocked)
         *pbuLocked = abuReply[14];

      buTemp = abuReply[15];
      if (pbuNumRecs)
         *pbuNumRecs = buTemp;

      if (pConnsB70)
      {
         nint iIdx = 16;

         for (i = 0; i < (nint)buTemp; i++, iIdx += 6)
         {
            NCopyHiLo16(&pConnsB70[i].suConnNum, &abuReply[iIdx+0]);
            pConnsB70[i].buTaskNum  = abuReply[iIdx+2];
            pConnsB70[i].buLockType = abuReply[iIdx+3];
            pConnsB70[i].buAccessControl = abuReply[iIdx+4];
            pConnsB70[i].buLockFlag = abuReply[iIdx+5];
         }
      }
   }

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s220.c,v 1.7 1994/09/26 17:36:24 rebekah Exp $
*/

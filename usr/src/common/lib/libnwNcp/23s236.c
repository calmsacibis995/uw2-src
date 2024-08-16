/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s236.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s236GetConnsUsingAFile*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s236GetConnsUsingAFile
         (
            pNWAccess pAccess,
            nuint8   buDataStream,
            nuint8   buVolNum,
            nuint32  luDirEntry,
            pnuint16 psuIterHnd,
            pnuint16 psuUseCnt,
            pnuint16 psuOpenCnt,
            pnuint16 psuOpenForRead,
            pnuint16 psuOpenForWrite,
            pnuint16 psuDenyReadCnt,
            pnuint16 psuDenyWriteCnt,
            pnuint8  pbuLocked,
            pnuint8  pbuForkCount,
            pnuint16 psuNumRecs,
            pNWNCPConnUsingFile3x pConnsB70
         );

REMARKS:
         This call returns all logical connections using the file specified by
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

         This routine replaces the NetWare 286 v2.1 NCP Get Connections Using
         A File (0x2222  23  220).

ARGS: <> pAccess
      >  buDataStream
      >  buVolNum
      >  luDirEntry
      <> psuIterHnd
      <  psuUseCnt (optional)
      <  psuOpenCnt (optional)
      <  psuOpenForRead (optional)
      <  psuOpenForWrite (optional)
      <  psuDenyReadCnt (optional)
      <  psuDenyWriteCnt (optional)
      <  pbuLocked (optional)
      <  pbuForkCount (optional)
      <  psuNumRecs (optional)
      <  pConnsB70 (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 220  Get Connections Using A File (old)
         23 234  Get Connection's Task Information
         23 235  Get Connection's Open Files

NCP:     23 236  Get Connections Using A File

CHANGES: 31 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s236GetConnsUsingAFile
(
   pNWAccess pAccess,
   nuint8   buDataStream,
   nuint8   buVolNum,
   nuint32  luDirEntry,
   pnuint16 psuIterHnd,
   pnuint16 psuUseCnt,
   pnuint16 psuOpenCnt,
   pnuint16 psuOpenForRead,
   pnuint16 psuOpenForWrite,
   pnuint16 psuDenyReadCnt,
   pnuint16 psuDenyWriteCnt,
   pnuint8  pbuLocked,
   pnuint8  pbuForkCount,
   pnuint16 psuNumRecs,
   pNWNCPConnUsingFile3x pConnsB70
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        236)
   #define NCP_STRUCT_LEN     ((nuint16)         9)
   #define NCP_REQ_LEN        ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN        ((nuint)         508)

   NWRCODE rcode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDataStream;
   abuReq[4] = buVolNum;
   NCopyHiLo32(&abuReq[5], &luDirEntry);
   NCopyHiLo16(&abuReq[9], psuIterHnd);

   rcode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REP_LEN, NULL);
   if (rcode == 0)
   {
      nint i;
      nuint16 suTemp;

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

      NCopyHiLo16(psuIterHnd, &abuReply[12]);

      if (pbuLocked)
         *pbuLocked = abuReply[14];

      if (pbuForkCount)
         *pbuForkCount = abuReply[15];

      NCopyHiLo16(&suTemp, &abuReply[16]);
      if (psuNumRecs)
         *psuNumRecs = suTemp;

      if (pConnsB70)
      {
         nint iIdx = 18;

         for (i = 0; i < (nint) suTemp; i++, iIdx += 7)
         {
            NCopyHiLo16(&pConnsB70[i].suConnNum, &abuReply[iIdx+0]);
            NCopyHiLo16(&pConnsB70[i].suTaskNum, &abuReply[iIdx+2]);
            pConnsB70[i].buLockType = abuReply[iIdx+4];
            pConnsB70[i].buAccessControl = abuReply[iIdx+5];
            pConnsB70[i].buLockFlag = abuReply[iIdx+6];
         }
      }
   }

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s236.c,v 1.7 1994/09/26 17:36:48 rebekah Exp $
*/

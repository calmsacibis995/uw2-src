/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s221.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s221GtPhyRecLocksConFl**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s221GtPhyRecLocksConFl
         (
            pNWAccess pAccess,
            nuint16  suTargetConnNum,
            pnuint16 psuIterHnd,
            nuint8   buVolNum,
            nuint26  suDirID,
            pnstr8   pbstrFileName,
            pnuint8  pbuNumOfLocks,
            pnuint8  pbuReserved,
            pNWNCPPhysRecLocksByFile2x pPhysRecLocks,
         );

REMARKS: This call returns a logical connection's physical record locks within a file.
         This call can only be made by a client with console operator rights.

         Target Connection Number indicates the logical connection number of
         the client using the file.

         Last Record Seen specifies the last record for which information is being
         returned.  On the first call, Last Record Seen must be set to zero.
         Subsequent calls should set Last Record Seen to the value in Next
         Request Record returned in the previous reply message.  If the value in
         Next Request Record is zero, the file server has passed all information to
         the requesting client.

         Volume Number identifies the volume on which the file exists.

         Directory ID is obtained from the File Search Initialize NCP and
         indicates the file path that is relative to this directory.

         File Name names the file for which information is being requested.

         Next Request Record contains the value to be placed in Last Record
         Seen for the next iteration of this call.  If Next Request Record is zero,
         all information has been returned.

         Physical Record Lock Count indicates the number of physical record
         locks.

         Number Of Records indicates the number of records to follow.

         The following information is repeated the number of times indicated by
         the value in Number Of Records.

            Task Number contains the number of the task that is using the file.

            Lock Status contains the bit flags indicating the file's lock status:

                  7 6 5 4 3 2 1 0
                  x x x x x x x 1  Locked exclusive
                  x x x x x x 1 x  Locked shareable
                  x x x x x 1 x x  Logged
                  x 1 x x x x x x  Lock is held by TTS

            Record Start is the byte offset (where the record begins) within the
            file.

            Record End is the byte offset (where the record ends) within the file.

         This client must have console operator rights to make this call.


ARGS: <> pAccess
      >  suTargetConnNum
      <> psuIterHnd
      >  buVolNum
      >  suDirID
      >  pbstrFileName
      <  pbuNumOfLocks
      <  pbuReserved (optional)
      <  pPhysRecLocks (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights
         0x89FD   Bad Station Number
         0x89FF   No Files Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 237  Get Physical Record Locks By Connection And File

NCP:     23 221  Get Physical Record Locks By Connection And File

CHANGES: 7 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s221GtPhyRecLocksConFl
(
   pNWAccess          pAccess,
   nuint16           suTargetConnNum,
   pnuint16          psuIterHnd,
   nuint8            buVolNum,
   nuint16           suDirID,
   pnstr8            pbstrFileName,
   pnuint8           pbuNumOfLocks,
   pnuint8           pbuReserved,
   pNWNCPPhysRecLocksByFile2x pPhysRecLocks
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        221)
   #define NCP_STRUCT_LEN     ((nuint16)        22)
   #define NCP_REQ_LEN        ((nuint)          24)
   #define NCP_REP_LEN1       ((nuint)           4)
   #define NCP_REP_LEN2       ((nuint)         510)
   #define NCP_REQ_FRAGS      ((nuint)           1)
   #define NCP_REP_FRAGS      ((nuint)           2)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN1],
           abuLockInfo[NCP_REP_LEN2];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suTargetConnNum);
   NCopyLoHi16(&abuReq[5], psuIterHnd);
   abuReq[7] = buVolNum;
   NCopyHiLo16(&abuReq[8], &suDirID);
   NWCMemMove(&abuReq[10], pbstrFileName, (nuint) 14);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN1;

   replyFrag[1].pAddr = abuLockInfo;
   replyFrag[1].uLen  = NCP_REP_LEN2;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,NCP_REP_FRAGS,
         replyFrag, NULL);
   if (lCode == 0)
   {
      nint i, n;

      NCopyLoHi16(&abuReply[0], psuIterHnd);
      *pbuNumOfLocks = abuReply[2];
      if (pbuReserved)
         *pbuReserved   = abuReply[3];

      if (pPhysRecLocks)
      {
         for (i = 0; i < (nint)*pbuNumOfLocks; i++)
         {
            n = i*10;

            pPhysRecLocks[i].buTaskNum  = abuLockInfo[n];
            pPhysRecLocks[i].buLockType = abuLockInfo[n+1];

            NCopyHiLo32(&pPhysRecLocks[i].luRecStart, &abuLockInfo[n+2]);
            NCopyHiLo32(&pPhysRecLocks[i].luRecEnd, &abuLockInfo[n+6]);
         }
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s221.c,v 1.7 1994/09/26 17:36:26 rebekah Exp $
*/

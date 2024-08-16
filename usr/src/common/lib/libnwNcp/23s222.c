/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s222.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s222GetPhyRecLocksFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s222GetPhyRecLocksFile
         (
            pNWAccess pAccess,
            pnuint16 psuIterHnd,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrFilePath,
            pnuint8  pbuNumOfLocks,
            pnuint8  pbuReserved,
            pNWNCPPhysRecLocks2x pPhysRecLocks,
         );

REMARKS: This call returns a list of all physical records that are locked in a file.

         Last Record Seen indicates the last record for which information is being
         returned.  On the first call, Last Record Seen must be set to zero.
         Subsequent calls should use the value returned by the file server in
         Next Request Record.  If the value in the Next Request Record field is
         zero, the file server has returned all information to the client.

         Directory Handle indicates the file path that is relative to this directory.

         Path Length indicates the length of File Path.

         File Path is the path and name of the file for which information is being
         requested.

         Next Request Record contains the value to be placed in the Last Record
         Seen field for the next iteration of this call.  If Next Request Record is
         zero, all information has been returned.

         Physical Record Lock Count is the number of physical record locks.

         The following information is repeated the number of times indicated by
         the value in Number Of Records.

            Task Number contains the number of the task that is using the file.

            Logged Count indicates the number of tasks that have the record
         logged.

            Shareable Lock Count indicates the number of tasks that have the
            record locked shareable.

            Record Start is the byte offset (where the record begins) within the
         file.

            Record End is the byte offset (where the record ends) within the file.

            Logical Connection Number indicates which logical connection has
            the record locked exclusively.

            Task Number contains the task number within the logical connection
            that has the record locked exclusively.

            Lock Type is a flag indicating the type of lock on the file:

                  0x00             Not locked
                  0xFE             Locked by a file lock
                  0xFF             Locked by a begin share file set

         A client must have console operator rights to make this call.


ARGS: <> pAccess
      <> psuIterHnd
      >  buDirHandle
      >  buPathLen
      >  pbstrFilePath
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

SEE:

NCP:     23 222  Get Physical Record Locks By File

CHANGES: 7 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s222GetPhyRecLocksFile
(
   pNWAccess       pAccess,
   pnuint16       psuIterHnd,
   nuint8         buDirHandle,
   nuint8         buPathLen,
   pnstr8         pbstrFilePath,
   pnuint8        pbuNumOfLocks,
   pnuint8        pbuReserved,
   pNWNCPPhysRecLocks2x pPhysRecLocks
)
{
   #define NCP_FUNCTION       ((nuint)    23)
   #define NCP_SUBFUNCTION    ((nuint8)  222)
   #define NCP_STRUCT_LEN     ((nuint16) (5 + buPathLen))
   #define NCP_REQ_LEN        ((nuint)     7)
   #define NCP_REP_LEN        ((nuint)     4)
   #define NCP_REQ_FRAGS      ((nuint)     2)
   #define NCP_REP_FRAGS      ((nuint)     2)
   #define NCP_MAX_RECLOCKS   ((nuint)   512)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN],
               abuRecLocks[NCP_MAX_RECLOCKS];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], psuIterHnd);
   abuReq[5] = (nuint8) buDirHandle;
   abuReq[6] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrFilePath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = abuRecLocks;
   replyFrag[1].uLen  = NCP_MAX_RECLOCKS;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
         replyFrag, NULL);
   if (lCode == 0)
   {
      nint i, n;

      NCopyLoHi16(&abuReply[0], psuIterHnd);
      *pbuNumOfLocks = abuReply[2];
      if (pbuReserved)
         *pbuReserved = abuReply[3];

      if (pPhysRecLocks)
      {
         for (i = 0; i < (nint)*pbuNumOfLocks; i++)
         {
            n = i*16;

            NCopyLoHi16(&pPhysRecLocks[i].suLoggedCount, &abuRecLocks[n]);
            NCopyLoHi16(&pPhysRecLocks[i].suShareableLockCount,
               &abuRecLocks[n+2]);
            NCopyLoHi32(&pPhysRecLocks[i].luRecStart, &abuRecLocks[n+4]);
            NCopyLoHi32(&pPhysRecLocks[i].luRecEnd, &abuRecLocks[n+8]);
            NCopyLoHi16(&pPhysRecLocks[i].suLogConnNum, &abuRecLocks[n+12]);
            pPhysRecLocks[i].buTaskNum = abuRecLocks[n+14];
            pPhysRecLocks[i].buLockType = abuRecLocks[n+15];
         }
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s222.c,v 1.7 1994/09/26 17:36:27 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s219.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s219GetConnOpenFiles***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s219GetConnOpenFiles
         (
            pNWAccess pAccess,
            nuint16  suConnNum,
            pnuint16 psuIterHnd,
            pnuint8  pbuNumRecs,
            pNWNCPConnOpenFiles2x pConnOpenFileInfo
         );

REMARKS: This is an old NetWare 286 v2.1 call that has been replaced by the
         NetWare 386 v3.0 call, Get Connection's Open Files (0x2222  23  235).

         This v2.1 call returns information about the files currently open by the
         specified Connection Number.  The call should be made iteratively

         The following information is repeated Number Of Records times.

            Task Number indicates the task number within the workstation that
            has the file open.

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

            Volume Number identifies the file's volume in a Volume Table on the
            file server.  The Volume Table contains information about each file
            server volume.

            Directory Entry contains the file path relative to this directory.  This
            value is not a directory handle.

            File Name is the null-terminated filename.

ARGS: <> pAccess
      >  suConnNum
      <> psuIterHnd
      <  pbuNumRecs
      <  pConnOpenFileInfo

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights
         0x89FD   Bad Station Number

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 235  Get Connection's Open Files

NCP:     23 219  Get Connection's Open Files (old)

CHANGES: 2 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s219GetConnOpenFiles
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint8  pbuNumRecs,
   pNWNCPConnOpenFiles2x pConnOpenFileInfo
)
{
   #define NCP_FUNCTION       ((nuint)     23)
   #define NCP_SUBFUNCTION    ((nuint8)   219)
   #define NCP_STRUCT_LEN     ((nuint16)    5)
   #define MAX_INFO_RECS      ((nuint)     22)
   #define SIZEOF_INFO        ((nuint)     21)
   #define MAX_INFO_SIZE      ((nuint) (MAX_INFO_RECS*SIZEOF_INFO))
   #define NCP_REQ_LEN        ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN        ((nuint) (3+MAX_INFO_SIZE))

   NWRCODE rcode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suConnNum);
   NCopyHiLo16(&abuReq[5], psuIterHnd);

   rcode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
            NCP_REP_LEN, NULL);
   if (rcode == 0)
   {
      nint i, j;

      NCopyHiLo16(psuIterHnd, &abuReply[0]);
      *pbuNumRecs = abuReply[2];

      for (i = 0, j = 3; i < (nint) *pbuNumRecs; i++, j += SIZEOF_INFO)
      {
         pConnOpenFileInfo[i].buTaskNum = abuReply[j];
         pConnOpenFileInfo[i].buLockType = abuReply[j+1];
         pConnOpenFileInfo[i].buAccessControl = abuReply[j+2];
         pConnOpenFileInfo[i].buLockFlag = abuReply[j+3];
         pConnOpenFileInfo[i].buVolNum = abuReply[j+4];
         NCopyHiLo16(&pConnOpenFileInfo[i].suDirEntry, &abuReply[j+5]);
         NWCMemMove(pConnOpenFileInfo[i].pbstrFileName, &abuReply[j+7], 14);
      }
   }

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s219.c,v 1.7 1994/09/26 17:36:21 rebekah Exp $
*/

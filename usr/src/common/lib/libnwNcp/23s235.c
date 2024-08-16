/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s235.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s235GetConnOpenFiles***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s235GetConnOpenFiles
         (
            pNWAccess pAccess,
            nuint16  suConnNum,
            pnuint16 psuIterHnd,
            pnuint16 psuNumRecs,
            pnuint8  pbuConnOpenFileData
         );

REMARKS: This call returns information about the files currently open by the
         specified Connection Number.  The call should be made iteratively

         The following information is repeated NumberOfRecords times.

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

         This routine replaces the NetWare 286 v2.1 NCP Get Connection's Open
         Files (0x2222  23  219).

ARGS: <> pAccess
      >  suConnNum
      <> psuIterHnd
      <  psuNumRecs
      <  pbuConnOpenFileData

INCLUDE: ncpserve.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 219  Get Connection's Open Files (old)
         23 234  Get Connection Task Information
         23 236  Get Connections Using A File

NCP:     23 235  Get Connection's Open Files

CHANGES: 2 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s235GetConnOpenFiles
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumRecs,
   pnuint8  pbuConnOpenFileData
)
{
   #define NCP_FUNCTION             ((nuint)          23)
   #define NCP_SUBFUNCTION          ((nuint8)        235)
   #define NCP_STRUCT_LEN           ((nuint16)         5)
   #define NCP_REQ_LEN              ((nuint)           7)
   #define NCP_REP_LEN              ((nuint)           4)
   #define NCP_INFO_LEN             ((nuint)         512)
   #define NCP_REQ_FRAGS            ((nuint)           1)
   #define NCP_REPLY_FRAGS          ((nuint)           2)

   NWRCODE rcode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], &suConnNum);
   NCopyLoHi16(&abuReq[5], psuIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = pbuConnOpenFileData;
   replyFrag[1].uLen  = NCP_INFO_LEN;

   rcode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
            NCP_REPLY_FRAGS, replyFrag, NULL);
   if (rcode == 0)
   {
      nint    i, j;
      nuint16 suTemp;
      nuint32 luTemp;

      NCopyLoHi16(psuIterHnd, &abuReply[0]);
      NCopyLoHi16(psuNumRecs, &abuReply[2]);

      for (i = j = 0; i < (nint) *psuNumRecs; i++)
      {
         NCopyLoHi16(&suTemp, &pbuConnOpenFileData[j]);  /* suTaskNum */
         NCopy16(&pbuConnOpenFileData[j], &suTemp);

         NCopyLoHi32(&luTemp, &pbuConnOpenFileData[j+6]);  /* luParentDirEntry */
         NCopy32(&pbuConnOpenFileData[j+6], &luTemp);

         NCopyLoHi32(&luTemp, &pbuConnOpenFileData[j+10]);  /* luDirEntry */
         NCopy32(&pbuConnOpenFileData[j+10], &luTemp);

         j += 17 + pbuConnOpenFileData[j+16];  /* add on the struct length */
      }
   }

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s235.c,v 1.7 1994/09/26 17:36:46 rebekah Exp $
*/

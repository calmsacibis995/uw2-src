/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s229.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s229GetConnUsageStats**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s229GetConnUsageStats
         (
            pNWAccess pAccess,
            nuint16  suConnNum,
            pnuint32 pluSysIntervalMarker,
            pnuint8  pbuBytesReadB6,
            pnuint8  pbuBytesWrittenB6,
            pnuint32 pluTotalReqPackets,
         )

REMARKS: This call allows a client to get usage statistics for the client's own logical
         connection.  Console operator rights are required to get usage statistics
         for connections other than the client's.

         System Interval Marker indicates how long the server has been up.
         This field is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  (There are 18.2 ticks per second, or one tick every
         0.54945054 of a second.)  When this field reaches 0xFFFFFFFF, it wraps
         back to zero.

         Bytes Read is the number of bytes the station has read on the specified
         Connection Number since the connection was created.

         Bytes Written is the number of bytes the station has written on the
         specified Connection Number since the connection was created.

         Bytes Read and Bytes Written are 6-byte low-high numbers.

ARGS: <> pAccess
      >  suConnNum
      <  pluSysIntervalMarker
      <  pbuBytesReadB6
      <  pbuBytesWrittenB6
      <  pluTotalReqPackets

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights
         0x89FD   Bad Station Number

SERVER:  2.1

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 229  Get Connection Usage Statistics

CHANGES: 20 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s229GetConnUsageStats
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint32 pluSysIntervalMarker,
   pnuint8  pbuBytesReadB6,
   pnuint8  pbuBytesWrittenB6,
   pnuint32 pluTotalReqPackets
)
{
   #define NCP_FUNCTION       ((nuint)              23)
   #define NCP_SUBFUNCTION    ((nuint8)            229)
   #define NCP_STRUCT_LEN     ((nuint16)             3)
   #define NCP_REQ_LEN        ((nuint)               5)
   #define NCP_REP_LEN        ((nuint)              20)
   #define NCP_REQ_FRAGS      ((nuint)               1)
   #define NCP_REP_FRAGS      ((nuint)               1)

   nint32   lCode;
   nint i;
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suConnNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      if(pluSysIntervalMarker)
         NCopyLoHi32(pluSysIntervalMarker, &abuReply[0]);
      if(pbuBytesReadB6)
      {
         for(i=0; i < 6; i++)
            pbuBytesReadB6[i] = abuReply[i+4];
      }
      if(pbuBytesWrittenB6)
      {
         for(i=0; i < 6; i++)
            pbuBytesWrittenB6[i] = abuReply[i+10];
      }
      if(pluTotalReqPackets)
         NCopyLoHi32(pluTotalReqPackets, &abuReply[16]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s229.c,v 1.8 1994/09/26 17:36:35 rebekah Exp $
*/

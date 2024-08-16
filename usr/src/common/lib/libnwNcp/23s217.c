/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s217.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s217GetDiskChannelStats**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s217GetDiskChannelStats
         (
            pNWAccess pAccess,
            nuint8   buDiskChannelNum,
            pNWNCPDiskChannelStats pStats,
         );

REMARKS: This call allows a client to get the disk channel statistics for the
         specified Disk Channel Number.  A client must have OPERATOR rights
         in order to make this call.

         System Interval Marker indicates how long the server has been up.
         This field is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Channel State indicates the state of the disk channel:

            0x00  Channel is running
            0x01  Channel is stopping
            0x02  Channel is stopped
            0x03  Channel is not functional

         Channel Synchronization State indicates the control state of the disk
         channel:

            0x00  Channel is not being used
            0x02  NetWare is using the channel; no one else wants it
            0x04  NetWare is using the channel; someone else wants it
            0x06  Someone else is using the channel; NetWare does not need it
            0x08  Someone else is using the channel; NetWare needs it
            0x0A  Someone else has released the channel; NetWare should use
         it

         Software Driver Type contains a number indicating which type of disk
         driver software that is installed in the disk channel.

         Software Major Version Number indicates the major version of the disk
         driver software installed on the disk channel.

         Software Minor Version Number indicates the minor version of the disk
         driver software installed on the disk channel.

         Software Description contains the null-terminated string describing the
         disk driver software.

         IO Addresses Used contains the two addresses the disk driver uses to
         control the disk channel.

         Shared Memory Addresses contains the two shared memory addresses
         (offsets).

         Interrupt Numbers Used contains the two interrupt numbers the disk
         driver uses to communicate with the disk channel.

         DMA Channels Used lists the DMA controllers used by the disk driver
         to control the disk channel.

         Configuration Description is a null-terminated string containing the
         channel's current IO driver configuration.


ARGS: <> pAccess
      >  buDiskChannelNum
      <  pStats

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 217  Get Disk Channel Statistics

CHANGES: 8 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s217GetDiskChannelStats
(
   pNWAccess                pAccess,
   nuint8                  buDiskChannelNum,
   pNWNCPDiskChannelStats  pStats
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        217)
   #define NCP_STRUCT_LEN     ((nuint16)         2)
   #define NCP_REQ_LEN        ((nuint)           4)
   #define NCP_REP_LEN        ((nuint)         184)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = (nuint8) buDiskChannelNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REP_LEN, NULL);
   if (lCode == 0)
   {
      NCopyHiLo32(&pStats->luSysElapsedTime, &abuReply[0]);
      NCopyHiLo16(&pStats->suChannelState, &abuReply[4]);
      NCopyHiLo16(&pStats->suChannelSyncState, &abuReply[6]);
      pStats->buDrvType = abuReply[8];
      pStats->buDrvMajorVer = abuReply[9];
      pStats->buDrvMinorVer = abuReply[10];
      NWCMemMove(pStats->abuDrvDescr, &abuReply[11], (nuint) 65);
      NCopyHiLo16(&pStats->suIOAddr1, &abuReply[76]);
      NCopyHiLo16(&pStats->suIOAddr1Size, &abuReply[78]);
      NCopyHiLo16(&pStats->suIOAddr2, &abuReply[80]);
      NCopyHiLo16(&pStats->suIOAddr2Size, &abuReply[82]);
      pStats->abuSharedMem1Seg[0] = abuReply[84];
      pStats->abuSharedMem1Seg[1] = abuReply[85];
      pStats->abuSharedMem1Seg[2] = abuReply[86];
      NCopyHiLo16(&pStats->suSharedMem1Ofs, &abuReply[87]);
      pStats->abuSharedMem2Seg[0] = abuReply[89];
      pStats->abuSharedMem2Seg[1] = abuReply[90];
      pStats->abuSharedMem2Seg[2] = abuReply[91];
      NCopyHiLo16(&pStats->suSharedMem2Ofs, &abuReply[92]);
      pStats->buInterrupt1Used = abuReply[94];
      pStats->buInterrupt1 = abuReply[95];
      pStats->buInterrupt2Used = abuReply[96];
      pStats->buInterrupt2 = abuReply[97];
      pStats->buDMAChannel1Used = abuReply[98];
      pStats->buDMAChannel1 = abuReply[99];
      pStats->buDMAChannel2Used = abuReply[100];
      pStats->buDMAChannel2 = abuReply[101];
      pStats->buFlagBits = abuReply[102];
      pStats->buReserved = abuReply[103];
      NWCMemMove(pStats->abuConfigDescr, &abuReply[104], (nuint) 80);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s217.c,v 1.7 1994/09/26 17:36:17 rebekah Exp $
*/

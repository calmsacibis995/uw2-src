/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s125.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s125ReadQCurrStatus****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s125ReadQCurrStatus
         (
            pNWAccess pAccess,
            nuint32  luInQueueID,
            pnuint32 pluOutQueueID,
            pnuint32 pluQueueStatus,
            pnuint32 pluCurrentEntries,
            pnuint32 pluCurrentServers,
            pnuint32 pluServerIDList,
            pnuint32 pluServerStationList
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Read
         Queue Current Status (0x2222  23  102).  This new NCP allows the use of the
         high connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to Queue
         NCPs for information on both the old and new job structures.

         The ServerIDList & ServerStationList fields are arrays.  The number of
         entries in both arrays are determined by the field CurrentServers.

ARGS: <> pAccess
       > luInQueueID
      <  pluOutQueueID (optional)
      <  pluQueueStatus (optional)
      <  pluCurrentEntries (optional)
      <  pluCurrentServers (optional)
      <  pluServerIDList (optional)
      <  pluServerStationList (optional)

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 102  Read Queue Current Status

NCP:     23 125  Read Queue Current Status

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s125ReadQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   pnuint32 pluOutQueueID,
   pnuint32 pluQueueStatus,
   pnuint32 pluCurrentEntries,
   pnuint32 pluCurrentServers,
   pnuint32 pluServerIDList,
   pnuint32 pluServerStationList
)
{
   #define NCP_FUNCTION     ((nuint)   23)
   #define NCP_SUBFUNCTION  ((nuint8) 125)
   #define NCP_STRUCT_LEN   ((nuint)    5)
   #define NCP_REQ_LEN      ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN      ((nuint)  216)

   nint32  lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen =  NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], (pnuint8)&luInQueueID);

   lCode = NWCRequestSingle(pAccess,
                            (nuint)NCP_FUNCTION,
                            abuReq,
                            (nuint)NCP_REQ_LEN,
                            abuReply,
                            (nuint)NCP_REP_LEN,
                            NULL);

   if (lCode == 0)
   {
      nint    i, iOfs;
      nuint32 luTempCurrServers;

      if (pluOutQueueID)
         NCopyHiLo32(pluOutQueueID, (pnuint32)&abuReply[0]);
      if (pluQueueStatus)
         NCopyLoHi32(pluQueueStatus, (pnuint32)&abuReply[4]);
      if (pluCurrentEntries)
         NCopyLoHi32(pluCurrentEntries, (pnuint32)&abuReply[8]);

      NCopyLoHi32(&luTempCurrServers, (pnuint32)&abuReply[12]);
      if(pluCurrentServers)
         *pluCurrentServers = luTempCurrServers;

      if(pluServerIDList)
      {
         for (i = 0, iOfs = 16; i < (nint) luTempCurrServers; i++, iOfs += 4)
            NCopyHiLo32(&pluServerIDList[i], (pnuint32)&abuReply[iOfs]);
      }

      if (pluServerStationList)
      {
         iOfs = (nint) (16 + 4*luTempCurrServers);

         for (i = 0; i < (nint) luTempCurrServers; i++, iOfs += 4)
            NCopyLoHi32(&pluServerStationList[i], &abuReply[iOfs]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s125.c,v 1.7 1994/09/26 17:35:22 rebekah Exp $
*/

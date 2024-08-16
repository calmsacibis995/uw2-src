/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s14.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s14GetDiskUtilization**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP23s14GetDiskUtilization
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luTrusteeID,
            pnuint8  pbuRepVolNum,
            pnuint32 pluRepTrusteeID,
            pnuint16 psuDirCnt,
            pnuint16 psuFileCnt,
            pnuint16 psuClusterCnt,
         );

REMARKS: This call allows a client to determine how much physical space the
         specified Trustee ID is using on the given volume.

         Directory Count indicates how many directories on the volume are
         owned by the Trustee ID.

         File Count indicates how many files on the volume are owned by the
         Trustee ID.

         Cluster Count indicates how many physical volume clusters the trustee's
         files occupy on the volume.  To map a cluster count to a count of the
         number of bytes used, see Get Volume Info With Number (function 18)
         in the "Directory Services" section.

         Clients who are supervisor equivalent can make this call for any object.
         Clients that do not have supervisor rights can make this call only for the
         object used when logging in.


ARGS: <> pAccess
      >  buVolNum
      >  luTrusteeID
      <  pbuRepVolNum      (optional)
      <  pluRepTrusteeID   (optional)
      <  psuDirCnt         (optional)
      <  psuFileCnt        (optional)
      <  psuClusterCnt     (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x8998   Disk Map Error
         0x89A1   Directory I/O Error
         0x89F2   No Object Read

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23  14  Get Disk Utilization

CHANGES: 13 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s14GetDiskUtilization
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luTrusteeID,
   pnuint8  pbuRepVolNum,
   pnuint32 pluRepTrusteeID,
   pnuint16 psuDirCnt,
   pnuint16 psuFileCnt,
   pnuint16 psuClusterCnt
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 14)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 11)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[REQ_LEN], abuRep[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4],&luTrusteeID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);

   if (lCode ==0)
   {
      if(pbuRepVolNum)
         *pbuRepVolNum = abuRep[0];

      if(pluRepTrusteeID)
         NCopyHiLo32(pluRepTrusteeID, &abuRep[1]);

      if(psuDirCnt)
         NCopyHiLo16(psuDirCnt, &abuRep[5]);

      if(psuFileCnt)
         NCopyHiLo16(psuFileCnt, &abuRep[7]);

      if(psuClusterCnt)
         NCopyHiLo16(psuClusterCnt, &abuRep[9]);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s14.c,v 1.7 1994/09/26 17:35:38 rebekah Exp $
*/

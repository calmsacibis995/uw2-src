/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s33.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s33GetVolSegmentList***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s33GetVolSegmentList
         (
            pNWAccess                 pAccess,
            nuint32                  luVolNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumSegments,
            pNWNCPFSEVolSegment      pListB41,
         )

REMARKS:

ARGS: <> pAccess
      >  luVolNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluNumSegments
      <  pListB41

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 33  Get Volume Segment List

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s33GetVolSegmentList
(
   pNWAccess                 pAccess,
   nuint32                  luVolNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumSegments,
   pNWNCPFSEVolSegment      pListB42
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 33)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_LIST_LEN    ((nuint) 504)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 12)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint8 abuStructList[MAX_LIST_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luVolNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = abuStructList;
   replyFrag[1].uLen  = MAX_LIST_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i;

      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(pluNumSegments, &abuReply[8]);

      for (i = 0; i < (nint)*pluNumSegments; i++)
      {
         NCopyLoHi32(&pListB42[i].luDeviceNum, &abuStructList[i*12]);
         NCopyLoHi32(&pListB42[i].luOffset,    &abuStructList[i*12+4]);
         NCopyLoHi32(&pListB42[i].luSize,      &abuStructList[i*12+8]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s33.c,v 1.7 1994/09/26 17:32:27 rebekah Exp $
*/

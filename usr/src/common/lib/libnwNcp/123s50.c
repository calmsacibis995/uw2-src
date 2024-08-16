/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s50.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s50GetRouterAndSapInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s50GetRouterAndSapInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluRIPSocketNum,
            pnuint32                 pluRouterDownFlag,
            pnuint32                 pluTrackOnFlag,
            pnuint32                 pluExtRouterActiveFlag,
            pnuint32                 pluSapSocketNum,
            pnuint32                 pluReplyNearestServerFlag,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluRIPSocketNum (optional)
      <  pluRouterDownFlag (optional)
      <  pluTrackOnFlag (optional)
      <  pluExtRouterActiveFlag (optional)
      <  pluSapSocketNum (optional)
      <  pluReplyNearestServerFlag (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 50  Get General Router And SAP Information

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s50GetRouterAndSapInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluRIPSocketNum,
   pnuint32                 pluRouterDownFlag,
   pnuint32                 pluTrackOnFlag,
   pnuint32                 pluExtRouterActiveFlag,
   pnuint32                 pluSapSocketNum,
   pnuint32                 pluReplyNearestServerFlag
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 50)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 32)

   nint32   lCode;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pluRIPSocketNum)
         NCopyLoHi32(pluRIPSocketNum, &abuReply[8]);
      if (pluRouterDownFlag)
         NCopyLoHi32(pluRouterDownFlag, &abuReply[12]);
      if (pluTrackOnFlag)
         NCopyLoHi32(pluTrackOnFlag, &abuReply[16]);
      if (pluExtRouterActiveFlag)
         NCopyLoHi32(pluExtRouterActiveFlag, &abuReply[20]);
      if (pluSapSocketNum)
         NCopyLoHi32(pluSapSocketNum, &abuReply[24]);
      if (pluReplyNearestServerFlag)
         NCopyLoHi32(pluReplyNearestServerFlag, &abuReply[28]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s50.c,v 1.7 1994/09/26 17:32:44 rebekah Exp $
*/

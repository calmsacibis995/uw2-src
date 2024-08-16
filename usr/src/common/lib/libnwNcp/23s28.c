/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s28.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s28GetStationLoggedInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s28GetStationLoggedInfo
         (
            pNWAccess pAccess,
            nuint32  luTargetConnNum,
            pnuint32 pluUserID,
            pnuint16 psuUserType,
            pnuint8  pbuUserNameB48,
            pnuint8  pbuLoginTimeB7,
            pnuint8  pbuReserved,
         );

REMARKS: This is a NetWare 386 v3.11 call that replaces the earlier call Get
         Station's Logged Info (0x2222  23  22).  This new NCP allows the use of
         the high connection byte in the Request/Reply header of the packet.  A
         new job structure has also been defined for this new NCP.  See
         Introduction to Queue NCPs for information on both the old and new job
         structure.

ARGS: <> pAccess
      >  luTargetConnNum
      <  pluUserID (optional)
      <  psuUserType (optional)
      <  pbuUserNameB48 (optional)
      <  pbuLoginTimeB7 (optional)
      <  pbuReserved (optional)

INCLUDE: ncpconn.h

RETURN:  0x0000 SUCCESSFUL

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 22  Get Station's Logged Info

NCP:     23 28  Get Station's Logged Info

CHANGES: 15 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s28GetStationLoggedInfo
(
   pNWAccess pAccess,
   nuint32  luTargetConnNum,
   pnuint32 pluUsrID,
   pnuint16 psuUsrType,
   pnuint8  pbuUsrNameB48,
   pnuint8  pbuLoginTimeB7,
   pnuint8  pbuReserved
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 28)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 62)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nint    i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyLoHi32(&abuReq[3], &luTargetConnNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if(pluUsrID)
         NCopyHiLo32(pluUsrID, &abuReply[0]);

      if(psuUsrType)
         NCopyHiLo16(psuUsrType, &abuReply[4]);

      if(pbuUsrNameB48)
      {
         for (i = 0; i < 48; i++)
            pbuUsrNameB48[i] = abuReply[i + 6];
      }

      if(pbuLoginTimeB7)
      {
         for (i = 0; i < 7; i++)
            pbuLoginTimeB7[i] = abuReply[i + 54];
      }

      if(pbuReserved)
         *pbuReserved = abuReply[61];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s28.c,v 1.7 1994/09/26 17:37:06 rebekah Exp $
*/

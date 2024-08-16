/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s26.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s26GetInternetAddr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s26GetInternetAddr
         (
            pNWAccess       pAccess,
            nuint32        luTargetConn,
            pNWNCPNetAddr  pNetAddr,
            pnuint8        pbuConnType,
         );

REMARKS: This is a NetWare 386 v3.11 call that replaces the earlier call Get
         Internet Address (0x2222  23  19).  This new NCP allows the use of the
         high connection byte in the Request/Reply header of the packet.  A new
         job structure has also been defined for this new NCP.  See Introduction
         to Queue NCPs for information on both the old and new job structure.

         The NetworkAddressStruc field contains the Network Address of the
         target connection number.  The following is the definition of the
         structure:

         struct NetWorkAddressStruct {
                        nuint8  NetworkAddress[4];
                        nuint8  NetworkNodeAddress[6];
                        nuint16  NetworkSocket;
                     };

         The ConnectionType field describes the type of connection the target has.
         The different types are as follows:

                     0 - not in use
                     2 - NCP
                     3 - AFP

ARGS: <> pAccess
      >  luTargetConn
      <  pNetAddr (optional)
      <  pbuConnType (optional)

INCLUDE: ncpconn.h

RETURN:  0x0000 SUCCESSFUL

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 19  Get Internet Address

NCP:     23 26  Get Internet Address

CHANGES: 15 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s26GetInternetAddr
(
   pNWAccess       pAccess,
   nuint32        luTargetConn,
   pNWNCPNetAddr  pNetAddr,
   pnuint8        pbuConnType
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 26)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 13)

   nint32   lCode;
   nuint16  suNCPLen;
   nuint8   abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nint     i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luTargetConn);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if (pNetAddr)
      {
         for(i = 0; i < 4; i++)
            pNetAddr->abuNetAddr[i] = abuReply[i];

         for(i = 0; i < 6; i++)
            pNetAddr->abuNetNodeAddr[i] = abuReply[i+4];

         NCopyLoHi16(&pNetAddr->suNetSocket, &abuReply[10]);
      }

      if(pbuConnType)
         *pbuConnType = abuReply[12];
   }
   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s26.c,v 1.7 1994/09/26 17:37:03 rebekah Exp $
*/

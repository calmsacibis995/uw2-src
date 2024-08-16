/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s150.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpacct.h"

/*manpage*NWNCP23s150GetAccountStatus**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s150GetAccountStatus
         (
            pNWAccess         pAccess,
            nuint16          suClientType,
            nuint8           buClientNameLen,
            pnstr8           pbstrClientName,
            pnint32          plAccBalance,
            pnint32          plCreditLim,
            pnuint8          pbuReservedB120,
            pNWNCPHoldsInfo  pHolderInfo,
         )

REMARKS: This call allows a server to get an account status at the time of the
         request.  No audit record is generated.  The fields in the reply are the
         same as those described for the ACCOUNT_BALANCE and
         ACCOUNT_HOLDS properties.  Refer to the introduction at the
         beginning of this chapter.

         If the requesting server's object ID is not listed in the
         ACCOUNT_SERVERS property of the file server's object, the No
         Account Privileges (192) completion code is returned.  If the requesting
         (user) object has no ACCOUNT_BALANCE property, the  No Account
         Balance (193) completion code is returned.  Other errors are shown in
         the Completion Code section.

ARGS: <> pAccess
      >  suClientType
      >  buClientNameLen
      >  pbstrClientName
      <  plAccBalance (optional)
      <  plCreditLim (optional)
      <  pbuReservedB120 (optional)
      <  pHolderInfo (optional)

INCLUDE: ncpacct.h

RETURN:  0x0000    Successful
         0x8996    Server Out Of Memory
         0x89C0    No Account Privileges
         0x89C1    No Account Balance
         0x89C4    Account Disabled
         0x89E8    Write To Group
         0x89EA    No Such Member
         0x89EB    Property Not Set Property
         0x89EC    No Such Set
         0x89FC    No Such Object
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  2.2 3.11

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 150  Current Account Status

CHANGES: 21 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s150GetAccountStatus
(
   pNWAccess        pAccess,
   nuint16         suClientType,
   nuint8          buClientNameLen,
   pnstr8          pbstrClientName,
   pnint32         plAccBalance,
   pnint32         plCreditLim,
   pnuint8         pbuReservedB120,
   pNWNCPHoldsInfo pHolderInfo
)
{
   #define NCP_FUNCTION       ((nuint)     23)
   #define NCP_SUBFUNCTION    ((nuint8)   150)
   #define NCP_STRUCT_LEN     ((nuint16) (4 + buClientNameLen))
   #define NCP_REQ_LEN        ((nuint)      6)
   #define NCP_REP_LEN        ((nuint)    256)
   #define NCP_REQ_FRAGS      ((nuint)      2)
   #define NCP_REP_FRAGS      ((nuint)      1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nint i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suClientType);
   abuReq[5] = buClientNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrClientName;
   reqFrag[1].uLen  = buClientNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if(plAccBalance)
         NCopyHiLo32(plAccBalance, &abuReply[0]);
      if(plCreditLim)
         NCopyHiLo32(plCreditLim, &abuReply[4]);

      if(pbuReservedB120)
      {
         for(i = 0; i < 120; i++)
            pbuReservedB120[i] = abuReply[i+8];
      }

      if(pHolderInfo)
      {
         for(i = 0; i < 16; i++)
         {
            NCopyHiLo32(&pHolderInfo[i].luHolderID, &abuReply[(i*8) + 128]);
            NCopyHiLo32(&pHolderInfo[i].lAmount, &abuReply[(i*8) + 4 + 128]);
         }
      }
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s150.c,v 1.7 1994/09/26 17:35:41 rebekah Exp $
*/

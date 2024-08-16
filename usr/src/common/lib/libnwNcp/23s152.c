/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s152.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpacct.h"

/*manpage*NWNCP23s152SubmitAccountHold**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s152SubmitAccountHold
         (
            pNWAccess   pAccess,
            nint32     lAmount,
            nuint16    suClientType,
            nuint8     buClientNameLen,
            pnstr8     pbstrClientName,
         )

REMARKS: This call allows a server to hold an amount, pending a
         subsequent charge to the client's account.  The
         amount, along with the requesting server's object ID, is
         recorded in the object's ACCOUNT_HOLDS property.  If
         the requesting server has an outstanding hold, the new
         hold amount is added to that server's outstanding hold
         amount.

         A server can back out a hold (in the event that the
         service is not provided) by calling Submit Account
         Hold with the additive inverse of the amount originally
         held.  A server can also choose to partially back out a
         hold by submitting a smaller negative amount.  To
         clear all holds, a server can submit a hold with a the
         amount set to 0 (zero).

         If service is provided, the hold should be cleared by
         specifying the original hold amount plus the charge
         amount in the Submit Account Charge request.

         If the requesting server's object ID is not listed in the
         ACCOUNT_SERVERS property of the file server's
         object, the No Account Privileges (192)completion code
         is returned.  If the object specified does not have an
         ACCOUNT_BALANCE property, the No Account
         Balance (193) completion code is returned.  If the
         current balance minus all holds (including the
         requested hold) is less than the permissible amount,
         the call fails, and the Credit Limit Exceeded (194)
         completion code is returned.  If too many other servers
         (16) have outstanding holds on the user's account, the
         call fails, and the Too Many Holds (195) completion
         code is returned.

ARGS: <> pAccess
      >  lAmount
      >  suClientType
      >  buClientNameLen
      >  pbstrClientName

INCLUDE: ncpacct.h

RETURN:  0x0000    Successful
         0x8901    Out Of Disk Space
         0x8988    Invalid File Handle
         0x8994    No Write Privileges
         0x8996    Server Out Of Memory
         0x89A2    I/O Lock Error
         0x89C0    No Account Privileges
         0x89C1    No Account Balance
         0x89C2    Credit Limit Exceeded
         0x89C3    Too Many Holds
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

NCP:     23 152  Submit Account Hold

CHANGES: 21 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s152SubmitAccountHold
(
   pNWAccess   pAccess,
   nint32     lAmount,
   nuint16    suClientType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName
)
{
   #define NCP_FUNCTION       ((nuint) 23)
   #define NCP_SUBFUNCTION    ((nuint8) 152)
   #define NCP_STRUCT_LEN     ((nuint16) (8 + buClientNameLen))
   #define NCP_REQ_LEN        ((nuint) 10)
   #define NCP_REQ_FRAGS      ((nuint) 2)
   #define NCP_REP_FRAGS      ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo32(&abuReq[3], &lAmount);
   NCopyHiLo16(&abuReq[7], &suClientType);
   abuReq[9] = buClientNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrClientName;
   reqFrag[1].uLen  = buClientNameLen;

   return ((NWRCODE) NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s152.c,v 1.7 1994/09/26 17:35:44 rebekah Exp $
*/

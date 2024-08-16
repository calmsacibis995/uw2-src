/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s151.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpacct.h"

/*manpage*NWNCP23s151SubmitAccountCharge**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s151SubmitAccountCharge
         (
            pNWAccess   pAccess,
            nuint16    suServiceType,
            nint32     lChargeAmt,
            nint32     lHoldCancelAmt,
            nuint16    suClientType,
            nuint16    suCommentType,
            nuint8     buClientNameLen,
            pnstr8     pbstrClientName,
            nuint8     buCommentLen,
            pnstr8     pbstrComment,
         )

REMARKS: This call allows a server to submit a charge to an account.  The charge is
         subtracted from the object's account balance, and an audit record is
         generated.  The Hold Cancel Amount is the same as the amount
         specified in the previous corresponding Submit Account Hold call.  If no
         Submit Account Hold call was made prior to providing the service, the
         Hold Cancel Amount is zero.  A Hold Cancel Amount of zero has no
         effect on other outstanding holds by the server.

         If the requesting server is not listed in the ACCOUNT_SERVERS
         property of the file server's object, the No Account Privileges (192)
         completion code is returned.  If the object specified has no
         ACCOUNT_BALANCE property, the No Account Balance (193)
         completion code is returned.  In either case, the account is not charged
         even though an entry is recorded in the audit file.  If the object's balance
         falls below its lowest
         permissible limit, the Credit Limit Exceeded (194) completion code is
         returned.

         An audit record is generated when a charge is submitted and the
         ACCOUNT_SERVERS property of the file server object exists.  This
         record is generated even if an non-zero completion code is returned by
         the Submit Account Charge call.

ARGS: <> pAccess
      >  suServiceType
      >  lChargeAmt,
      >  lHoldCancelAmt,
      >  suClientType
      >  suCommentType
      >  buClientNameLen
      >  pbstrClientName
      >  buCommentLen
      >  pbstrComment

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
         0x89C4    Account Disabled
         0x89E8    Write To Group
         0x89EA    No Such Member
         0x89EB    Property Not Set Property
         0x89EC    No Such Set
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  2.2 3.11

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 151  Account Charge

CHANGES: 21 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s151SubmitAccountCharge
(
   pNWAccess   pAccess,
   nuint16    suServiceType,
   nint32     lChargeAmt,
   nint32     lHoldCancelAmt,
   nuint16    suClientType,
   nuint16    suCommentType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName,
   nuint8     buCommentLen,
   pnstr8     pbstrComment
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 151)
   #define NCP_STRUCT_LEN  ((nuint16) (17 + buClientNameLen + buCommentLen))
   #define NCP_REQ_LEN     ((nuint) 18)
   #define NCP_REQ_FRAGS   ((nuint) 4)
   #define NCP_REP_FRAGS   ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suServiceType);
   NCopyHiLo32(&abuReq[5], &lChargeAmt);
   NCopyHiLo32(&abuReq[9], &lHoldCancelAmt);
   NCopyHiLo16(&abuReq[13], &suClientType);
   NCopyHiLo16(&abuReq[15], &suCommentType);
   abuReq[17] = buClientNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrClientName;
   reqFrag[1].uLen  = buClientNameLen;

   reqFrag[2].pAddr = &buCommentLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrComment;
   reqFrag[3].uLen  = buCommentLen;

   return ((NWRCODE) NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s151.c,v 1.7 1994/09/26 17:35:43 rebekah Exp $
*/

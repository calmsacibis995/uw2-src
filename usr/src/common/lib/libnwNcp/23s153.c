/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s153.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpacct.h"

/*manpage*NWNCP23s153SubmitAccountNote**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s153SubmitAccountNote
         (
            pNWAccess   pAccess,
            nuint16    suServiceType,
            nuint16    suClientType,
            nuint16    suCommentType,
            nuint8     buClientNameLen,
            pnstr8     pbstrClientName,
            nuint8     buCommentLen,
            pnstr8     pbstrComment,
         )

REMARKS: This call allows a server to record a note about a client's account
         activities in the audit file.  For example, the file server uses this call to
         record a login time.

         If the requesting server is not listed in the ACCOUNT_SERVERS
         property of the file server's object, the No Account Privileges (192)
         completion code is returned.  In this case, no audit record is generated.

ARGS: <> pAccess
      >  suServiceType
      >  suClientType
      >  suCommentType
      >  buClientNameLen
      >  pbstrClientName
      >  buCommentLen
      >  pbstrComment

INCLUDE: ncpacct.h

RETURN:  0x0000    Successful
         0x8901    Out Of Disk Space
         0x8996    Server Out Of Memory
         0x89C0    No Account Privileges
         0x89C1    No Account Balance
         0x89C4    Account Disabled
         0x89E8    Write To Group
         0x89EA    No Such Member
         0x89EB    Property Not Set Property
         0x89EC    No Such Set
         0x89F0    Illegal Wildcard
         0x89FC    No Such Object
         0x89FF    Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 153  Submit Account Note

CHANGES: 22 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s153SubmitAccountNote
(
   pNWAccess   pAccess,
   nuint16    suServiceType,
   nuint16    suClientType,
   nuint16    suCommentType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName,
   nuint8     buCommentLen,
   pnstr8     pbstrComment
)
{
   #define NCP_FUNCTION       ((nuint)    23)
   #define NCP_SUBFUNCTION    ((nuint8)  153)
   #define NCP_STRUCT_LEN     ((nuint16) (9 + buClientNameLen + buCommentLen))
   #define NCP_REQ_LEN        ((nuint)    10)
   #define NCP_REQ_FRAGS      ((nuint)     4)
   #define NCP_REP_FRAGS      ((nuint)     0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suServiceType);
   NCopyHiLo16(&abuReq[5], &suClientType);
   NCopyHiLo16(&abuReq[7], &suCommentType);
   abuReq[9] = buClientNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrClientName;
   reqFrag[1].uLen  = buClientNameLen;

   reqFrag[2].pAddr = &buCommentLen;
   reqFrag[2].uLen  = 1;

   reqFrag[3].pAddr = pbstrComment;
   reqFrag[3].uLen  = buCommentLen;

   return ((NWRCODE) NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, NULL, NULL));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s153.c,v 1.7 1994/09/26 17:35:45 rebekah Exp $
*/

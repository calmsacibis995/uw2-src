/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s19.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s19GetInternetAddr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s19GetInternetAddr
         (
            pNWAccess pAccess,
            nuint8   buTargetConn,
            pnuint8  pbuNetAddrB12,
         )

REMARKS: This call returns the physical network address of the client workstation,
         given the workstation's logical connection number.  This call can be used
         to determine the address of any client that has a connection on the
         network.  Typically this call would precede the setting up of a direct
         network dialogue between two clients.  The format of the Network
         Address field is network-dependent.

         This call is replaced by the NetWare 386 v3.11 call Get Internet Address

         (0x2222  23  26).

ARGS: <> pAccess
      >  buTargetConn
      <  pbuNetAddrB12

INCLUDE: ncpconn.h

RETURN:  0x0000    Successful
         0x89FF    Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 21  Get Object Connection List
         23 26  Get Internet Address

NCP:     23 19  Get Internet Address

CHANGES: 15 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s19GetInternetAddr
(
   pNWAccess pAccess,
   nuint8   buTargetConn,
   pnuint8  pbuNetAddrB12
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 19)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 12)

   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   abuReq[3] = buTargetConn;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               pbuNetAddrB12, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s19.c,v 1.7 1994/09/26 17:35:51 rebekah Exp $
*/

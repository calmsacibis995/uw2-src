/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s19.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s19NSGetInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s19NSGetInfo
         (
            pNWAccess pAccess,
            nuint8   buSrcNamSpc,
            nuint8   buDstNamSpc,
            nuint8   buReserved,
            nuint8   buVolNum,
            nuint32  luDirBase,
            nuint32  luNSInfoBitMask,
            pnuint8  pbuNSSpecificInfoB512
         );

REMARKS: Gets specific name space information. Note also that 1) this call is
         passed to the name space NLM and 2) this call is an expensive time
         user on the server.

ARGS: <> pAccess
       > buSrcNamSpc
       > buDstNamSpc
       > buReserved
       > buVolNum
       > luDirBase
       > luNSInfoBitMask
      <  pbuNSSpecificInfoB512

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 47  Get Name Space Information
         87 25  Set NS Information

NCP:     87 19  Get NS Information

CHANGES: 16 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s19NSGetInfo
(
   pNWAccess pAccess,
   nuint8   buSrcNamSpc,
   nuint8   buDstNamSpc,
   nuint8   buReserved,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luNSInfoBitMask,
   pnuint8  pbuNSSpecificInfoB512
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 19)
   #define REQ_LEN         ((nuint) 13)
   #define INFO_STRUCT_SIZE ((nuint) 512)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buSrcNamSpc;
   abuReq[2] = buDstNamSpc;
   abuReq[3] = buReserved;
   abuReq[4] = buVolNum;

   NCopyLoHi32(&abuReq[5], &luDirBase);
   NCopyLoHi32(&abuReq[9], &luNSInfoBitMask);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               pbuNSSpecificInfoB512, INFO_STRUCT_SIZE, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s19.c,v 1.7 1994/09/26 17:39:21 rebekah Exp $
*/

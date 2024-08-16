/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s8.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s8DelEntry**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s8DelEntry
         (
            pNWAccess       pAccess,
            nuint8         buNamSpc,
            nuint8         buReserved,
            nuint16        suSrchAttrs,
            pNWNCPCompPath pCompPath,
         );

REMARKS:

ARGS: <> pAccess
      >  buNamSpc
      >  buReserved
      >  suSrchAttrs
      >  pCompPath

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 11  Delete Directory

NCP:     87 08  Delete A File or SubDirectory

CHANGES: 3 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s8DelEntry
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suSrchAttrs,
   pNWNCPCompPath pCompPath
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 8)
   #define REQ_LEN         ((nuint) 5)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buReserved;
   NCopyLoHi16(&abuReq[3], &suSrchAttrs);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = (nuint) pCompPath->suPackedLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s8.c,v 1.7 1994/09/26 17:39:44 rebekah Exp $
*/

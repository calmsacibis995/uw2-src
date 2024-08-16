/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s18.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s18DelPurge**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s18DelPurge
         (
            pNWAccess pAccess,
            nuint8   buNamSpc,
            nuint8   buReserved,
            nuint32  luIterHnd,
            nuint32  luVolNum,
            nuint32  luDirBase,
         );

REMARKS: Purges entries found through the ScanSalvageableFiles function.

         The NWHandlePathStruct must point to a SubDirectory path. No file
         names or wild cards are allowed when using this call to search for
         salvageable files or subdirectories.

         The SearchSequence field is the value that was used in the Scan
         Salvageable Files request.

ARGS: <> pAccess
      >  buNamSpc
      >  buReserved
      >  luIterHnd
      >  luVolNum
      >  luDirBase

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 29  Purge Salvageable File
         23 206 Purge All Erased Files

NCP:     87 18  Purge Salvageable File

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s18DelPurge
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buReserved,
   nuint32  luIterHnd,
   nuint32  luVolNum,
   nuint32  luDirBase
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 18)
   #define REQ_LEN         ((nuint) 15)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[15];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buReserved;
   NCopyLoHi32(&abuReq[3], &luIterHnd);
   NCopyLoHi32(&abuReq[7], &luVolNum);
   NCopyLoHi32(&abuReq[11], &luDirBase);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s18.c,v 1.7 1994/09/26 17:39:20 rebekah Exp $
*/

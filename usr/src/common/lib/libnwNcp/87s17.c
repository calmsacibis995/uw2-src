/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s17.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s17DelRecover**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s17DelRecover
         (
            pNWAccess pAccess,
            nuint8   buNamSpc,
            nuint8   buReserved,
            nuint32  luIterHnd,
            nuint32  luVolNum,
            nuint32  luDirBase,
            nuint8   buNewFileNameLen,
            pnstr8   pbstrNewFileName,
         );

REMARKS: Recovers a file or subdirectory entry found through the
         ScanSalvageableFiles function.

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
      >  buNewFileNameLen
      >  pbstrNewFileName

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 28  Recover Salvageable File

NCP:     87 17  Recover Salvageable File

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s17DelRecover
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buReserved,
   nuint32  luIterHnd,
   nuint32  luVolNum,
   nuint32  luDirBase,
   nuint8   buNewFileNameLen,
   pnstr8   pbstrNewFileName
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 17)
   #define REQ_LEN         ((nuint) 16)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[16];
   NWCFrag reqFrag[2];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buReserved;
   NCopyLoHi32(&abuReq[3], &luIterHnd);
   NCopyLoHi32(&abuReq[7], &luVolNum);
   NCopyLoHi32(&abuReq[11], &luDirBase);
   abuReq[15] = buNewFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrNewFileName;
   reqFrag[1].uLen  = buNewFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s17.c,v 1.7 1994/09/26 17:39:18 rebekah Exp $
*/

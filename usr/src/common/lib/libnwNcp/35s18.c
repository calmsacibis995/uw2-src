/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s18.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s18AFPGetDOSNameEntryID**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s18AFPGetDOSNameEntryID
         (
            pNWAccess    pAccess,
            nuint8      buVolNum,
            nuint32     luAFPEntryID,
            pnuint8     pbuDOSPathLen,
            pnstr8      pbstrDOSPath,
         );

REMARKS: Obtain the DOS directory path corresponding to a 32-bit Macintosh
         file or directory entry ID.

         The Mac Directory Entry ID is the unique 32-bit AFP identifier of the file
         or directory.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      <  pbuDOSPathLen
      <  pbstrDOSPath

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8989  No Search Privilege
         0x8996  Server Out of Memory
         0x8996  Target Not a Subdirectory
         0x89BF  Invalid Name Space

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN

SEE:     35 12  AFP Get Entry ID From Path Name
         35 04  AFP Get Entry ID From Name

NCP:     35 18  AFP Get DOS Name From Entry ID

CHANGES: 20 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s18AFPGetDOSNameEntryID
(
   pNWAccess    pAccess,
   nuint8      buVolNum,
   nuint32     luAFPEntryID,
   pnuint8     pbuDOSPathLen,
   pnstr8      pbstrDOSPath
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 18)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define DOS_PATH_LEN    ((nuint) 255)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPEntryID);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = pbuDOSPathLen;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbstrDOSPath;
   replyFrag[1].uLen  = DOS_PATH_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s18.c,v 1.7 1994/09/26 17:38:20 rebekah Exp $
*/

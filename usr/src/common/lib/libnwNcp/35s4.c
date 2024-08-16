/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP34s4AFPGetEntryIDFromName************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s4AFPGetEntryIDFromName
         (
            pNWAccess pAccess,
            nuint8 buVolNum,
            nuint32 luAFPEntryID,
            nuint8 buPathLen,
            pnstr8 pbstrPath,
            pnuint32 pluAFPEntryID
         );

REMARKS: Return the 32-bit AFP ID number of the specified directory
         or file (long name).  The ID of the root directory is 1L.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  buPathLen,
      >  pbstrPath
      <  pluAFPEntryID

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN

SEE:     35 18  AFP Get DOS Name From Entry ID

NCP:     35 04  AFP Get Entry ID From Name

CHANGES: 19 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s4AFPGetEntryIDFromName
(
   pNWAccess pAccess,
   nuint8 buVolNum,
   nuint32 luAFPEntryID,
   nuint8 buPathLen,
   pnstr8 pbstrPath,
   pnuint32 pluAFPEntryID
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define NCP_STRUCT_LEN  ((nuint16) (7 + buPathLen))
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   NWRCODE ccode;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPEntryID);
   abuReq[8] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   if ((ccode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL)) == 0)
   {
      NCopyHiLo32(pluAFPEntryID, &abuReply[0]);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s4.c,v 1.7 1994/09/26 17:38:25 rebekah Exp $
*/

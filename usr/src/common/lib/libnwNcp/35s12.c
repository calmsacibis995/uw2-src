/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s12.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWAFPGetEntryIDFromPathName***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s12AFPGEntryIDFrmPathNm
         (
            pNWAccess pAccess,
            pnuint8  pbuNWDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint32 pluAFPEntryID,
         )

REMARKS: Converts a NetWare style path into a 32-bit Macintosh
         file or directory entry ID.

         Converts a length specified string containing a NetWare-style path
         specification into a unique 32-bit AFP File or Directory ID. The
         Directory Base and Path specifications are given in Netware "short
         name" format. The AFP Base ID is the unique 32-bit identifier of
         the file or directory.

ARGS: <> pAccess
      >  pbuNWDirHandle
      >  buPathLen
      >  pbstrPath
      <  pluAFPEntryID

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     35 18  AFP Get DOS Name From Entry ID

NCP:     35 12  AFP Get Entry ID From Path Name

CHANGES: 19 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s12AFPGEntryIDFrmPathNm
(
   pNWAccess pAccess,
   nuint8   buNWDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluAFPEntryID
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 12)
   #define NCP_STRUCT_LEN  ((nuint16) (3 + buPathLen))
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8 abuBuf[REQ_LEN], abuRep[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen    = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuBuf[0],&suNCPLen);
   abuBuf[2]   = NCP_SUBFUNCTION;
   abuBuf[3]   = buNWDirHandle;
   abuBuf[4]   = buPathLen;

   reqFrag[0].pAddr  = abuBuf;
   reqFrag[0].uLen   = REQ_LEN;

   reqFrag[1].pAddr  = pbstrPath;
   reqFrag[1].uLen   = buPathLen;

   replyFrag[0].pAddr= abuRep;
   replyFrag[0].uLen = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyHiLo32(pluAFPEntryID, &abuRep[0]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s12.c,v 1.7 1994/09/26 17:38:12 rebekah Exp $
*/

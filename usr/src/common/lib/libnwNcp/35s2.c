/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s2AFPCreateFile*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s2AFPCreateFile
         (
            pNWAccess    pAccess,
            nuint8      buVolNum,
            nuint32     luAFPEntryID,
            nuint8      buDelExisting,
            pnuint8     pbuFinderInfoB32,
            nuint8      buPathLen,
            pnstr8      pbstrPath,
            pnuint32    pluNewAFPEntryID
         );

REMARKS: Create a file with an AFP filename.  The resulting file is not
         opened.  The file is created as a normal read-write file with system and
         hidden bits cleared.

         The PathModString field is in AFP long-name format.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  buDelExisting
      >  pbuFinderInfoB32
      >  buPathLen
      >  pbstrPath
      <  pluNewAFPEntryID

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8983  Hard I/O Error
         0x8984  No Create Privileges
         0x8987  Create Filename Error
         0x8988  Invalid File Handle
         0x898A  No Delete Privileges
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899E  Bad File Name
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure, File Exists Error, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN

SEE:     35 03  AFP Delete
         35 14  AFP 2.0 Create File

NCP:     35 02  AFP Create File

CHANGES: 20 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s2AFPCreateFile
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buDelExisting,
   pnuint8  pbuFinderInfoB32,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluNewAFPEntryID
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define NCP_STRUCT_LEN  ((nuint16) (40 + buPathLen))
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 1)

   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   NWRCODE ccode;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPEntryID);
   abuReq[8] = (nuint8) buDelExisting;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuFinderInfoB32;
   reqFrag[1].uLen  = FINDER_INFO_LEN;

   reqFrag[2].pAddr = &buPathLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrPath;
   reqFrag[3].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   if ((ccode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
         REPLY_FRAGS, replyFrag, NULL)) == 0)
   {
      NCopyHiLo32(pluNewAFPEntryID, &abuReply[0]);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s2.c,v 1.7 1994/09/26 17:38:22 rebekah Exp $
*/

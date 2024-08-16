/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s8.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWAFPOpenFileFork*************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s8AFPOpenFileFork
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPEntryID,
            nuint8   buResFork,
            nuint8   buAccessMode,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint32 pluAFPEntryID,
            pnuint32 pluForkLen,
            pnuint8  pbuNWDirHandleB6
         )

REMARKS: This call opens an AFP file fork (data fork or resource fork).  If a
         nonexistent file fork is specified, a file fork will automatically be
         created and opened.

         The Access Mode field contains the following bits:

            0x01  Read Access
            0x02  Write Access
            0x04  Deny Read Access
            0x08  Deny Write Access
            0x10  Compatibility Mode Bit (should be set)

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  buResFork
      >  buAccessMode
      >  buPathLen
      >  pbstrPath
      <  pluAFPEntryID (optional)
      <  pluForkLen (optional)
      <  pbuNWDirHandleB6 (optional)


INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8994  No Write Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure, Lock Error, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 08  AFP Open File Fork

CHANGES: 27 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s8AFPOpenFileFork
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buResFork,
   nuint8   buAccessMode,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluAFPEntryID,
   pnuint32 pluForkLen,
   pnuint8  pbuNWDirHandleB6
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 8)
   #define NCP_STRUCT_LEN  ((nuint16) (9 + buPathLen))
   #define REQ_LEN         ((nuint) 11)
   #define REPLY_LEN       ((nuint) 14)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWRCODE ccode;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(abuReq, &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPEntryID);
   abuReq[8] = buResFork;
   abuReq[9] = buAccessMode;
   abuReq[10] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   if ((ccode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL)) == 0)
   {
      if (pluAFPEntryID)
         NCopyHiLo32(pluAFPEntryID, &abuReply[0]);
      if (pluForkLen)
         NCopyHiLo32(pluForkLen, &abuReply[4]);
      if (pbuNWDirHandleB6)
         NWCMemMove(pbuNWDirHandleB6, &abuReply[8], 6);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s8.c,v 1.7 1994/09/26 17:38:29 rebekah Exp $
*/

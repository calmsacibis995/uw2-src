/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s11.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWAFPAllocTemporaryDirHandle**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s11AFPAllocTempDirHan
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPEntryID,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuNWDirHandle,
            pnuint8  pbuAccessRights,
         )

REMARKS: This call maps a NetWare directory handle to an AFP directory.

         The NetWare Access Rights field is a 1-byte mask that returns the
         effective rights that the calling station has in the target directory.
         This byte can be a combination of the following bits:
                        0x01  Read
                        0x02  Write
                        0x04  Open
                        0x08  Create
                        0x10  Delete
                        0x20  Parental
                        0x40  Search
                        0x80  Modify

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  buPathLen
      >  pbstrPath
      <  pbuNWDirHandle
      <  pbuAccessRights (optional)

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899D  No Directory Handles
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 11  AFP Alloc Temporary Directory Handle

CHANGES: 18 Aug 1993 - written - dromrell
         26 Oct 1993 - eliminated zeroing on error - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s11AFPAllocTempDirHan
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNWDirHandle,
   pnuint8  pbuAccessRights
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 11)
   #define NCP_STRUCT_LEN  ((nuint16) (7+buPathLen))
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 2)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   NWCFrag  reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8   abuReq[REQ_LEN], abuRep[REPLY_LEN];
   nuint16  suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4],&luAFPEntryID);
   abuReq[8] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      *pbuNWDirHandle = abuRep[0];

      if(pbuAccessRights)
         *pbuAccessRights = abuRep[1];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s11.c,v 1.7 1994/09/26 17:38:10 rebekah Exp $
*/

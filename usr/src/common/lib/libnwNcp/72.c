/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:72.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP72FileRead**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP72FileRead
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuNWHandleB6,
            nuint32  luStartingOffset,
            nuint16  suBytesToRead,
            pnuint16 psuBytesActuallyRead,
            pnuint8  pbuDataB512,
         );

REMARKS: This function reads a block of bytes from a file starting at a specified
         file offset.  If the end-of-file is encountered before the read request
         is satisfied, the server returns a Successful Completion Code, but Bytes
         Actually Read (returned by the server) will contain a count less than
         Bytes To Read (specified by the client).

         This function will fail if the calling client does not have read access
         privileges to the indicated file, or if a portion of the targeted byte
         block is locked for use by some other client.

         Clients using this function are constrained by the current negotiated file
         buffer size (see Negotiate Buffer Size (function 33).  A client cannot
         request more bytes of data than will fit in the currently negotiated buffer
         size.  Furthermore, the client cannot request a data block that straddles a
         buffer-size boundary in the file.  Thus, if the current buffer size were 512
         bytes and the client wished to read 1000 bytes starting at file offset 500,
         the client must issue three read requests.  The first request would read 12
         bytes starting at offset 500.  The second request would read 512 bytes
         starting at offset 512.  The third request would read 476 bytes starting at
         offset 1024.

         If a client requests a block of data that starts on an odd-byte boundary
         within the file, the first byte of the data field returned by the server
         will contain garbage; the actual data from the file will start in the second
         byte of the data block.

ARGS: <> pAccess
      >  buReserved
      >  pbuNWHandleB6
      >  luStartingOffset
      >  suBytesToRead
      <  psuBytesActuallyRead
      <  pbuDataB512

INCLUDE: ncpfile.h

RETURN:  0x0000    Successful
         0x8988    Invalid File Handle
         0x8993    No Read Privileges
         0x89FF    I/O Bound Error

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     72 --  Read From A File

CHANGES: 4 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP72FileRead
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   nuint32  luStartingOffset,
   nuint16  suBytesToRead,
   pnuint16 psuBytesActuallyRead,
   pnuint8  pbuDataB512
)
{
   #define NCP_FUNCTION    ((nuint) 72)
   #define REQ_LEN         ((nuint) 13)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32  lCode;
   nuint8  abuReq[REQ_LEN];
   nint    i;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = buReserved;
   for (i = 0; i < (nuint) 6; i++)
      abuReq[1 + i] = pbuNWHandleB6[i];
   NCopyHiLo32(&abuReq[7], &luStartingOffset);
   NCopyHiLo16(&abuReq[11], &suBytesToRead);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = psuBytesActuallyRead;
   replyFrag[0].uLen  = (nuint) 2;

   replyFrag[1].pAddr = pbuDataB512;
   replyFrag[1].uLen  = (nuint) suBytesToRead;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag, REPLY_FRAGS,
            replyFrag, NULL);

   if (lCode == 0)
   {
      *psuBytesActuallyRead = NSwapHiLo16(*psuBytesActuallyRead);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/72.c,v 1.8 1994/09/27 22:16:47 rebekah Exp $
*/

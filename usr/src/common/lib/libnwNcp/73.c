/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:73.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP73FileWrite**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP73FileWrite
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuFileHandleB6,
            nuint32  luStartingByteOffset,
            nuint16  suBytesToWrite,
            pnuint8  pbuData
         );

REMARKS: This call writes a block of data to a file.  This request will fail if the
         client does not have Write access to the indicated file, or if some portion
         of the targeted byte block is locked for use by a different client.

         Clients using this call are constrained by the current negotiated file
         buffer size (see Negotiate Buffer Size, 0x2222  33  --).  A client cannot
         write more bytes of data than will fit in the currently negotiated buffer
         size.  In addition, the client cannot write a data block that straddles a
         buffer-size 4K boundary in the file.  Thus if the current buffer size were
         4,096 bytes and the client wished to write 4200 bytes starting at file offset
         4000, the client must issue three write requests.  The first request would
         write 96 bytes starting at offset 4,000.  The second request would write
         4,096 bytes starting at offset 4,096.  The third request would write 8 bytes
         starting at offset 8,192.

ARGS: <> pAccess
      >  buReserved
      >  pbuFileHandleB6
      >  luStartingByteOffset
      >  suBytesToWrite
      >  pbuData

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8988  Invalid File Handle
         0x8994  No Write Privileges
         0x8995  File Detached
         0x89A2  IO Lock Error
         0x89FF  I/O Bound Error

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     33 --  Negotiate Buffer Size

NCP:     73 --  Write To A File

CHANGES: 4 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP73FileWrite
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuFileHandleB6,
   nuint32  luStartingByteOffset,
   nuint16  suBytesToWrite,
   pnuint8  pbuData
)
{
   #define NCP_FUNCTION    ((nuint) 73)
   #define REQ_LEN         ((nuint) 13)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8  abuReq[REQ_LEN];
   nint    i;
   NWCFrag reqFrag[REQ_FRAGS];

   abuReq[0] = buReserved;
   for (i = 0; i < (nuint) 6; i++)
      abuReq[1 + i] = pbuFileHandleB6[i];
   NCopyHiLo32(&abuReq[7], &luStartingByteOffset);
   NCopyHiLo16(&abuReq[11], &suBytesToWrite);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuData;
   reqFrag[1].uLen  = suBytesToWrite;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/73.c,v 1.7 1994/09/26 17:38:56 rebekah Exp $
*/

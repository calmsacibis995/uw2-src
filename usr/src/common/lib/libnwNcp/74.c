/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:74.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP74FileCopy**********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP74FileCopy
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuSrcNWHandleB6,
            pnuint8  pbuDstNWHandleB6,
            nuint32  luSrcOffset,
            nuint32  luDstOffset,
            nuint32  luBytesToCopy,
            pnuint32 pluActualBytesCopied
         );

REMARKS: Copies a file from a source to a destination on the same file
         server. The copy will take place entirely on the target server
         resulting in fast results.

         The files used in this operation must have been created and opened
         prior to calling this function.


         This call allows data from one file on a file server to be rapidly
         transferred to another file on the same file server.  This removes the need
         for a client to copy data from one file to another by reading the data from
         the file server across the network to the client's workstation and then
         writing the same data back to the file server across the network.

         The client must have previously opened both files and must have file Read
         privileges for the source file and file Write privileges for the destination
         file.  The server transfers the specified amount of information from the
         source to the destination file starting at the offsets given by the client.
         If the end of the source file is encountered before the specified number of
         bytes have been transferred, the transfer stops with a Successful Completion
         Code, but the number of bytes actually transferred will be less than the
         number of bytes the client requested transferred.

ARGS: <> pAccess
      >  buReserved
      >  pbuSrcNWHandleB6
      >  pbuDstNWHandleB6
      >  luSrcOffset
      >  luDstOffset
      >  luBytesToCopy
      <  pluActualBytesCopied

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8901  Out Of Disk Space
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8994  No Write Privileges
         0x8995  File Detached
         0x8996  Server Out Of Memory
         0x89A2  IO Lock Error

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     74 --  Copy From One File To Another

CHANGES: 27 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP74FileCopy
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuSrcNWHandleB6,
   pnuint8  pbuDstNWHandleB6,
   nuint32  luSrcOffset,
   nuint32  luDstOffset,
   nuint32  luBytesToCopy,
   pnuint32 pluActualBytesCopied
)
{
   #define NCP_FUNCTION    ((nuint) 74)
   #define NCP_REQ_LEN     ((nuint) 25)
   #define NCP_REPLY_LEN   ((nuint) 4)

   nint   i;
   nint32 lCode;
   nuint8 abuReq[NCP_REQ_LEN];

   abuReq[0] = buReserved;

   for (i = 0; i < (nint) 6; i++)
      abuReq[i+1] = pbuSrcNWHandleB6[i];

   for (i = 0; i < (nint) 6; i++)
      abuReq[i+7] = pbuDstNWHandleB6[i];

   NCopyHiLo32(&abuReq[13], &luSrcOffset);
   NCopyHiLo32(&abuReq[17], &luDstOffset);
   NCopyHiLo32(&abuReq[21], &luBytesToCopy);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               pluActualBytesCopied, NCP_REPLY_LEN, NULL);
   if(lCode == 0)
   {
      *pluActualBytesCopied = NSwapHiLo32(*pluActualBytesCopied);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/74.c,v 1.7 1994/09/26 17:38:57 rebekah Exp $
*/

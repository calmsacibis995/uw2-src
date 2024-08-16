/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:71.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP71FileGetSize**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP71FileGetSize
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuNWHandleB6,
            pnuint32 pluFileSize,
         );

REMARKS: This call allows a client to determine the current length of a file that the
         client has open.  It can be used by clients cooperatively sharing a file that
         might need to be extended.

         When a shared file needs to be extended, the client extending the file needs
         to lock the area of the file that will be affected for its exclusive use.
         This can be done by locking the entire file or by locking the section of the
         file that is beyond the current known end-of-file.

         After locking the file, the client extending the file must make this call to
         determine the current file length. If the file has already been extended by
         some other client, this call will reveal the length of the extension to the
         current client so that the file can be extended further, if needed. The file
         can then be unlocked.

         Only by using this method of extending a shared file can a client properly
         lock the current end of a file for exclusive access; otherwise, another
         client could extend the file at any time.

ARGS: <> pAccess
      >  buReserved
      >  pbuNWHandleB6
      <  pluFileSize

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8988  Invalid File Handle

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     71 --  Get Current Size Of File

CHANGES: 4 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP71FileGetSize
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   pnuint32 pluFileSize
)
{
   #define NCP_FUNCTION    ((nuint) 71)
   #define REQ_LEN         ((nuint) 7)
   #define REPLY_LEN       ((nuint) 4)

   nint   i;
   nint32 lCode;
   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buReserved;
   for (i = 0; i < (nint) 6; i++)
      abuReq[i+1] = pbuNWHandleB6[i];

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               pluFileSize, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      *pluFileSize = NSwapHiLo32(*pluFileSize);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/71.c,v 1.7 1994/09/26 17:38:53 rebekah Exp $
*/

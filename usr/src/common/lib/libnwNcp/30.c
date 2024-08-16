/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:30.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP30SyncClrPhyRec**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP30SyncClrPhyRec
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuNWHandleB6,
            nuint32  luLockOffset,
            nuint32  luLockLen,
         );

REMARKS: This call removes the specified byte range from the client's data byte
         range table.  If the byte range is locked, then it is cleared.  The
         client can't use the specified byte range until that range is again
         logged and locked.

         The byte range specified must match a byte range previously logged.  Clearing
         only a portion of a previously logged byte range is not allowed.

ARGS: <> pAccess
      >  buReserved
      >  pbuNWHandleB6
      >  luLockOffset
      >  luLockLen

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FF  Unlock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     26  --  Log Physical Record
         27  --  Lock Physical Record Set
         28  --  Release Physical Record
         29  --  Release Physical Record Set
         31  --  Clear Physical Record Set

NCP:     30  --  Clear Physical Record

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP30SyncClrPhyRec
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockOffset,
   nuint32  luLockLen
)
{
   #define NCP_FUNCTION    ((nuint) 30)
   #define REQ_LEN         ((nuint) 15)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint  i;

   abuReq[0] = (nuint8) buReserved;
   for (i = 0; i < 6; i++)
      abuReq[1+i] = pbuNWHandleB6[i];
   NCopyHiLo32(&abuReq[7], &luLockOffset);
   NCopyHiLo32(&abuReq[11], &luLockLen);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/30.c,v 1.7 1994/09/26 17:37:47 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:26.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP26SyncLogPhyRec******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
          NWNCP26SyncLogPhyRec
          (
             pNWAccess pAccess,
             nuint8   buLockFlag,
             pnuint8  pbuNWHandleB6,
             nuint32  luLockOffset,
             nuint32  luLockLen,
             nuint16  suLockTimeout,
          );

REMARKS: This call allows a station to determine whether it has console
         privileges on the target server.  If it does, the Successful
         completion code will be returned.  If it does not, the No Console
         Rights completion code will be returned.

ARGS: <> pAccess

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     26  Log Physical Record

CHANGES: 20 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP26SyncLogPhyRec
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockOffset,
   nuint32  luLockLen,
   nuint16  suLockTimeout
)
{
   #define NCP_FUNCTION    ((nuint) 26)
   #define REQ_LEN         ((nuint) 17)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buLockFlag;
   NWCMemMove( &abuReq[1], pbuNWHandleB6, 6 );
   NCopyHiLo32( &abuReq[7], &luLockOffset );
   NCopyHiLo32( &abuReq[11], &luLockLen );
   NCopyHiLo16( &abuReq[15], &suLockTimeout );

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/26.c,v 1.7 1994/09/26 17:37:41 rebekah Exp $
*/

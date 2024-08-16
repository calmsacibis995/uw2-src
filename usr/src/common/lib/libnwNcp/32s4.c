/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:32s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP32s4SyncSemClose**************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP32s4SyncSemClose
         (
            pNWAccess pAccess,
            nuint32  luSemHandle,
         )

REMARKS: This call allows a client to close a semaphore that is no longer needed.  The
         corresponding Semaphore Open Count (number of clients accessing the
         semaphore) is decremented by one.  If the Semaphore Open Count is zero after
         this call, the server deletes the semaphore.


ARGS: <> pAccess
      >  luSemHandle

INCLUDE: ncpsync.h

RETURN:  0x0000 Successful
         0x89FF LockError

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     32 01  Examine Semaphore

NCP:     32 04  Close Semaphore

CHANGES: 30 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP32s4SyncSemClose
(
   pNWAccess pAccess,
   nuint32  luSemHandle
)
{
   #define NCP_FUNCTION    ((nuint) 32)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[5];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1],&luSemHandle);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/32s4.c,v 1.7 1994/09/26 17:37:55 rebekah Exp $
*/

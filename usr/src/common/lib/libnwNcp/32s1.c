/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:32s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWExamineSemaphore************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP32s1SyncSemExamine
         (
            pNWAccess pAccess,
            nuint32  luSemHandle,
            pnuint8  pbuSemVal,
            pnuint8  pbuSemOpenCnt,

         )

REMARKS: This call returns the current value of the target semaphore.  Semaphore
         Open Count is the number of clients that are using the semaphore.  Semaphore
         Value is the current value of the semaphore.  Semaphore Value is discussed
         in the Wait On Semaphore and Signal Semaphore calls.


ARGS: <> pAccess
       > luSemHandle
       < pbuSemVal      (Optional)
       < pbuSemOpenCnt  (Optional)

INCLUDE: ncpsync.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     32 01  Examine Semaphore

CHANGES: 30 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP32s1SyncSemExamine
(
	pNWAccess pAccess,
	nuint32  luSemHandle,
	pnuint8  pbuSemVal,
	pnuint8  pbuSemOpenCnt
)
{
	#define NCP_FUNCTION    ((nuint) 32)
	#define NCP_SUBFUNCTION ((nuint8) 1)
	#define REQ_LEN         ((nuint) 5)
	#define REPLY_LEN       ((nuint) 2)

   nint32   lCode;
	nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

	abuReq[0] = NCP_SUBFUNCTION;
	NCopyLoHi32(&abuReq[1], &luSemHandle);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
					abuReply, REPLY_LEN, NULL);
	if (lCode == 0)
	{
		if (pbuSemVal)
			*pbuSemVal = abuReply[0];

		if (pbuSemOpenCnt)
			*pbuSemOpenCnt = abuReply[1];
	}

	return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/32s1.c,v 1.7 1994/09/26 17:37:51 rebekah Exp $
*/

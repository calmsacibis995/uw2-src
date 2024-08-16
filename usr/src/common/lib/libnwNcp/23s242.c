/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s242.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s242GetSemInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s242GetSemInfo
         (
            pNWAccess pAccess,
            pnuint16 psuIterHnd,
            nuint8   buSemNameLen,
            pnstr8   pbstrSemName,
            pnuint16 psuOpenCnt,
            pnuint16 psuSemValue,
            pnuint16 psuNumRecs,
            pNWNCPSemInfo3x pSemInfo,
         );

REMARKS: This call returns information about a single semaphore.  This call should
         be used iteratively when a logical connection has more semaphores than
         can be placed in the reply message.

         Last Record Seen indicates the last record for which information is being
         returned.  On the first call, Last Record Seen must be set to zero.
         Subsequent calls should use the Next Request Record returned by the
         file server.  If the value in the Next Request Record field is zero, the file
         server has returned all information to the client.

         Semaphore Name Length is the length of Semaphore Name.

         Semaphore Name is the name that identifies the semaphore.

         Next Request Record contains the record number to be placed in Last
         Record Seen for the next iteration of this call.  If Next Request Record is
         zero, all records have been returned.

         Open Count indicates the number of logical connections that have this
         semaphore open.

         Semaphore Value indicates the current value of the semaphore (-
         127..128).  A negative value is usually interpreted as the number of
         workstations waiting for the service represented by the semaphore.  A
         positive value is usually interpreted as the number of free resources
         available in the resource pool governed by the semaphore.

         The following information is repeated the number of times indicated by
         the value in Number Of Records.

            Logical Connection Number indicates the logical connection using the
            semaphore.

            Task Number indicates which task within the logical connection has
            the semaphore open.

         The requesting workstation have console operator rights to make this
         call.  This routine replaces the NetWare 286 v2.1 NCP Get Semaphore
         Information (0x2222  23 226).


ARGS: <> pAccess
      <> psuIterHnd
      >  buSemNameLen
      >  pbstrSemName
      <  psuOpenCnt (optional)
      <  psuSemValue
      <  psuNumRecs
      <  pSemInfo (optional)

INCLUDE: ncpserve.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 226  Get Semaphore Information (old)
         23 241  Get Connection's Semaphore's

NCP:     23 242  Get Semaphore Information

CHANGES: 8 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s242GetSemInfo
(
   pNWAccess          pAccess,
   pnuint16          psuIterHnd,
   nuint8            buSemNameLen,
   pnstr8            pbstrSemName,
   pnuint16          psuOpenCnt,
   pnuint16          psuSemValue,
   pnuint16          psuNumRecs,
   pNWNCPSemInfo3x   pSemInfo
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        242)
   #define NCP_STRUCT_LEN     ((nuint16) (4 + buSemNameLen))
   #define NCP_REQ_LEN        ((nuint)           6)
   #define NCP_REP_LEN        ((nuint)           8)
   #define NCP_REQ_FRAGS      ((nuint)           2)
   #define NCP_REP_FRAGS      ((nuint)           2)
   #define NCP_MAX_SEM_STUFF  ((nuint)         512)

   nint32   lCode;
   nuint16 suNCPLen;
	nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN], abuSemStuff[NCP_MAX_SEM_STUFF];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], psuIterHnd);
   abuReq[5] = buSemNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrSemName;
   reqFrag[1].uLen  = buSemNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = abuSemStuff;
   replyFrag[1].uLen  = NCP_MAX_SEM_STUFF;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
				replyFrag, NULL);
   if (lCode == 0)
	{
		NCopyHiLo16(psuIterHnd, &abuReply[0]);
		if (psuOpenCnt)
			NCopyLoHi16(psuOpenCnt, &abuReply[2]);
		NCopyLoHi16(psuSemValue, &abuReply[4]);
		NCopyLoHi16(psuNumRecs, &abuReply[6]);

		if (pSemInfo)
		{
			nint i;

			for (i = 0; i < (nint)*psuNumRecs; i++)
			{
				NCopyLoHi16(&pSemInfo[i].suLogConnNum, &abuSemStuff[i*4]);
            NCopyLoHi16(&pSemInfo[i].suTaskNum, &abuSemStuff[i*4+2]);
         }
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s242.c,v 1.7 1994/09/26 17:36:58 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s225.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s225GetConnSems**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP23s225GetConnSems
         (
            pNWAccess pAccess,
            nuint16  suConnNum,
            pnuint16 psuIterHnd,
            pnuint16 psuNumSems,
            pnuint8  pConnSemsInfo,
         );

REMARKS: This function allows an operator to get a list of the semaphores that a
         connection is using.  Because a connection may be using more
         semaphores than the server can report in one reply message, this call
         can be made iteratively.  On the first call, Last Record Seen should be
         set to zero.  If the server returns a Next Request Record value that is
         non-zero, then the server has not reported all the information.  The
         calling program should repeat the request using the value returned in
         the Next Request Record as the new value for Last Record Seen.  This
         process should be repeated until a reply packet with Next Request
         Record set to zero is received or until the Completion Code is not zero.
         When Next Request Record is zero, there is no more information
         avaliable on the specified connection.

         The server returns a Number Of Semaphores indicating the status of
         semaphores that the target connection is using.  The Number Of
         Semaphores returned is recorded in Number Of Semaphores.  Each
         semaphore record contains the following information.

         Open Count indicates the number of connections that have this
         semaphore open for use.

         Semaphore Value is the current value of the semaphore.  A negative
         value is (normally interpreted as) the number of workstations waiting
         for the service represented by the semaphore.  A positive value indicates
         the number of free resources available in the resource pool governed by
         the semaphore.

         Task Number contains the number of the connection's task that is using
         the semaphore.

         Semaphore Name Length contains the length of the semaphore name.

         Semaphore Name contains the name of the semaphore.


ARGS: <> pAccess
      >  suConnNum
      <> psuIterHnd
      <  psuNumSems
      <  pConnSemsInfo

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights
         0x89FD   Bad Station Number

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 241  Get Connection's Semaphores

NCP:     23 225  Get Connection's Semaphores (old)

CHANGES: 9 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s225GetConnSems
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumSems,
   pnuint8  pConnSemsInfo
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        225)
   #define NCP_STRUCT_LEN     ((nuint16)         5)
   #define NCP_REQ_LEN        ((nuint)           7)
   #define NCP_REP_LEN        ((nuint)           4)
   #define NCP_REQ_FRAGS      ((nuint)           1)
   #define NCP_REP_FRAGS      ((nuint)           2)
   #define NCP_MAX_SEMS_INFO  ((nuint)         508)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suConnNum);
   NCopyLoHi16(&abuReq[5], psuIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = pConnSemsInfo;
   replyFrag[1].uLen  = NCP_MAX_SEMS_INFO;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
         replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(psuIterHnd, &abuReply[0]);
      NCopyLoHi16(psuNumSems, &abuReply[2]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s225.c,v 1.7 1994/09/26 17:36:31 rebekah Exp $
*/

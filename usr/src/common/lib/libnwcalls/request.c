/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:request.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwmisc.h"

/*manpage*NWRequest*********************************************************
SYNTAX:  NWCCODE N_API NWRequest
         (
            NWCONN_HANDLE  conn,
            nuint16        function,
            nuint16        numReqFrags,
            NW_FRAGMENT NWPTR reqFrags,
            nuint16        numReplyFrags,
            NW_FRAGMENT NWPTR replyFrags
         )

REMARKS: Passes an NCP request to the server

ARGS: >  conn
      >  function
         The NCP function number number

      >  numReqFrags
         Number of fragments pointed to by reqFrags - 5 maximum

      >  reqFrags
         Pointer to list of request fragments:

         typedef struct
         {
            void NWPTR fragAddress;
         #if defined(NWNLM) || defined(WIN32)
            nuint32 fragSize;
         #else
            nuint16 fragSize;
         #endif
         } NW_FRAGMENT;

      >  numReplyFrags
         Number of fragments pointed to by replyFrags - 5 maximum

      >  reqFrags
         Pointer to list of reply fragments.

INCLUDE: nwmisc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     n/a

CHANGES: 14 Oct 1993 - gutted for nwncp
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWRequest
(
   NWCONN_HANDLE     conn,
   nuint16           function,
   nuint16           numReqFrags,
   NW_FRAGMENT NWPTR reqFrags,
   nuint16           numReplyFrags,
   NW_FRAGMENT NWPTR replyFrags
)
{
   NWCFrag _reqFrags[5], _replyFrags[5];
   nuint16 i;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   for(i = 0; i < numReqFrags; i++)
   {
      _reqFrags[i].pAddr = reqFrags[i].fragAddress;
      _reqFrags[i].uLen  = (nuint) reqFrags[i].fragSize;
   }

   for(i = 0; i < numReplyFrags; i++)
   {
      _replyFrags[i].pAddr = replyFrags[i].fragAddress;
      _replyFrags[i].uLen  = (nuint) replyFrags[i].fragSize;
   }

   return (NWCCODE) NWCRequest(&access, (nuint) function,
                       (nuint) numReqFrags, _reqFrags,
                       (nuint) numReplyFrags, _replyFrags,
                       NULL);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/request.c,v 1.7 1994/09/26 17:49:11 rebekah Exp $
*/

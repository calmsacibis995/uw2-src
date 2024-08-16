/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:orderreq.c	1.6"
#if defined N_PLAT_MSW && defined N_ARCH_32
#include <windows.h>
#endif
#if defined N_PLAT_MSW 
#include <malloc.h>
#endif

#include "ntypes.h"
#if defined N_PLAT_MSW && defined N_ARCH_32
#include "nwcint.h"
#endif

#ifdef N_PLAT_UNIX
#include <stdlib.h>
#endif

#include "nwclient.h"
#include "nwcaldef.h"
#include "nwcint.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwconnec.h"
#include "nwerror.h"

/*manpage*NWOrderedRequestToAll*********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWOrderedRequestToAll
         (
            nuint16 suLockReqCode,
            nuint16 suReqCode,
            nuint16 suReqFragCount,
            NW_FRAGMENT NWPTR reqFragList,
            nuint16 suErrorReqCode,
            nuint16 suErrorFragCount,
            NW_FRAGMENT NWPTR errorFragList
         )

REMARKS:

ARGS:  > suLockReqCode
       > suReqCode
       > suReqFragCount
       > reqFragList
       > suErrorReqCode
       > suErrorFragCount
       > errorFragList

INCLUDE: nwundoc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 21 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWOrderedRequestToAll
(
   nuint16     suLockReqCode,
   nuint16     suReqCode,
   nuint16     suReqFragCount,
   NW_FRAGMENT NWPTR reqFragList,
   nuint16     suErrorReqCode,
   nuint16     suErrorFragCount,
   NW_FRAGMENT NWPTR errorFragList
)
{
#if defined N_PLAT_MSW  || \
    defined(N_PLAT_UNIX)
   typedef struct NET_ADDR
   {
      nuint8   addr[30];
   } NET_ADDR;

   NET_ADDR       N_FAR *inetAddr;
   NWCCODE        ccode;
   int            errIdx;  /* must remain an int! */
   nuint          numConns, suNextSend, x, y;
   nuint          authenticationState;
   nuint32        scanIndex = 0;
   nuint32        connRef;
   NWCONN_HANDLE  N_FAR *errorSend;
   NWCONN_HANDLE  N_FAR *connList;
   NWCTranAddr    tranAddr;
   NWCDeclareAccess(access);

   suLockReqCode = suLockReqCode;

   /* Get the number of connections that we will be sending this request to.*/
   /* (public + private connections)                                        */
   ccode = (NWCCODE)NWCGetNumConns(&access, NULL, &x, &y, NULL);
   numConns = x + y;

   errorSend = NWCMalloc(sizeof(NWCONN_HANDLE)*numConns);
   connList = NWCMalloc(sizeof(NWCONN_HANDLE)*numConns);
   inetAddr = NWCMalloc(sizeof(NET_ADDR)*numConns);
   if (!errorSend || !connList || !inetAddr)
   {
      ccode = INSUFFICIENT_RESOURCES;
      goto FreeMemory;
   }

   authenticationState = NWC_AUTH_STATE_NONE;

   /* Get connection list of all valid connections */
   for (ccode = x = 0; ccode == 0 && x < numConns; )
   {
      tranAddr.pbuBuffer = inetAddr[x].addr;
      tranAddr.uLen = 30;
      ccode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, 
			NWC_CONN_INFO_AUTH_STATE,
            sizeof(authenticationState), &authenticationState,
            NWC_MATCH_NOT_EQUALS, NWC_CONN_INFO_TRAN_ADDR, sizeof(nuint32),
            &connRef, &tranAddr);

      if (ccode == 0)
      {
         ccode = (NWCCODE)NWCOpenConnByReference(&access, connRef,
                     NWC_OPEN_LICENSED );
         if (ccode == 0)
            connList[x++] = (NWCONN_HANDLE)NWCGetConn(access);
         NWCMemSet(&tranAddr.pbuBuffer[tranAddr.uLen], 0, 30-tranAddr.uLen);
      }
   }

   if (ccode != SCAN_COMPLETE)
      goto FreeMemory;

   ccode = 0;
   numConns = x;

   /* Send requests from least valued internet address to most */
   for(x = errIdx = 0; x < numConns; x++)
   {
      for(y = suNextSend = 0; y < numConns; y++)
         if(NWCMemCmp(inetAddr[suNextSend].addr, inetAddr[y].addr, 30) > 0)
            suNextSend = y;   /* New least valued internet address */

      ccode = NWRequest(connList[suNextSend], suReqCode,
                  suReqFragCount, reqFragList, 0, NULL);
      if(ccode)
      {
         /* Send error packet to all servers in reverse order
            that the normal request was sent */

         for (--errIdx; errIdx >= 0; errIdx--)
             NWRequest(errorSend[errIdx], suErrorReqCode,
                        suErrorFragCount, errorFragList, 0, NULL);

         goto CleanUp;
      }
      else
      {
         /* Keep track of sending order in case of error */
         errorSend[errIdx++] = connList[suNextSend];
      }

      /* Mark the connection as sent */
      NWCMemSet(inetAddr[suNextSend].addr, 0xff, 30);
   }
CleanUp:
   for (x=0; x<numConns; x++)
   {
      NWCSetConn(access, connList[x]);
      NWCCloseConn(&access);
   }

FreeMemory:
   if (errorSend)
      NWCFree(errorSend);
   if (connList )
      NWCFree(connList );
   if (inetAddr )
      NWCFree(inetAddr );
   return(ccode);

#elif defined(NWOS2)

   suLockReqCode     = suLockReqCode;
   suReqCode         = suReqCode;
   suReqFragCount    = suReqFragCount;
   reqFragList       = reqFragList;
   suErrorReqCode    = suErrorReqCode;
   suErrorFragCount  = suErrorFragCount;

   return ((NWCCODE) NWCCallGate(_NWC_ORDEREDNETREQUEST, &errorFragList));

#else
#if   defined(N_PLAT_DOS)||defined(N_PLAT_MSW) || defined( N_PLAT_NLM )

   return((NWCCODE)NWCOrderedRequestAll(suLockReqCode,suReqCode,suReqFragCount,
                                        (pNWCFrag)reqFragList, suErrorReqCode,
                                         suErrorFragCount, (pNWCFrag) errorFragList));

#endif
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/orderreq.c,v 1.8 1994/09/26 17:48:33 rebekah Exp $
*/


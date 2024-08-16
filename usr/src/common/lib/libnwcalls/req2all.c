/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:req2all.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#include "ntypes.h"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include "nwcint.h"
#endif
#include "nwcaldef.h"
#include "nwcint.h"
#include "nwundoc.h"
#include "nwclient.h"
#include "nwconnec.h"
#include "nwmisc.h"
#include "nwerror.h"

/*manpage*NWRequestToAll****************************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWRequestToAll
         (
            nuint16 suReqCode,
            nuint16 suReqFragCount,
            NW_FRAGMENT NWPTR reqFragList
         )

REMARKS:

ARGS: >  suReqCode
      >  suReqFragCount
      >  reqFragList

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 21 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWRequestToAll
(
   nuint16 suReqCode,
   nuint16 suReqFragCount,
   NW_FRAGMENT NWPTR reqFragList
)
{
#if  defined N_PLAT_WNT || \
     defined(N_PLAT_UNIX)
   NWCONN_HANDLE  conn;
   NWCCODE        ccode;
   nuint          authenticationState;
   nuint32        scanIndex = 0;
   nuint32        connRef;
   NWCDeclareAccess(access);

   authenticationState = NWC_AUTH_STATE_NONE;

   for(ccode = 0; ccode == 0;)
   {
      ccode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, 
			NWC_CONN_INFO_AUTH_STATE,
            sizeof(authenticationState), &authenticationState,
            NWC_MATCH_NOT_EQUALS, NWC_CONN_INFO_CONN_REF, sizeof(nuint32),
            &connRef, &connRef);

      if (ccode == 0)
      {
         ccode = (NWCCODE)NWCOpenConnByReference(&access, connRef, NWC_OPEN_LICENSED);
         if (ccode == 0)
         {
            conn = (NWCONN_HANDLE)NWCGetConn(access);
            NWRequest(conn,
                  suReqCode,
                  suReqFragCount,
                  reqFragList,
                  0,
                  NULL);

            NWCCloseConn(&access);
         }
      }
   }
   if (ccode == SCAN_COMPLETE)
      ccode = 0;

   return(ccode);

#elif defined(NWOS2)

  suReqCode = suReqCode;
  suReqFragCount = suReqFragCount;
  return (NWCCallGate(_NWC_NETREQUESTALL, &reqFragList));

#else
#if   defined(N_PLAT_DOS)||defined(N_PLAT_MSW) || defined( N_PLAT_NLM )

   return((NWCCODE)NWCRequestAll(suReqCode,suReqFragCount,(pNWCFrag)reqFragList));
#endif
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/req2all.c,v 1.7 1994/09/26 17:49:09 rebekah Exp $
*/


/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtmaxcon.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#include "ntypes.h"
#include "nwcint.h"
#else
#include "ntypes.h"
#endif

#include "nwintern.h"
#include "nwcint.h"
#include "nwclient.h"
#include "nwconnec.h"
#include "nwmisc.h"
#include "nwundoc.h"


/*manpage*NWGetMaximumConnections*******************************************
SYNTAX:  void N_API NWGetMaximumConnections
         (
           pnuint16 maxConns
         )

REMARKS:

ARGS: <  maxConns

INCLUDE: nwconnec.h

RETURN:

CLIENT:  DOS WIN OS2 NT NLM

SEE:

CHANGES: 17 Sep 1993 - hungarian notation added - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
void N_API NWGetMaximumConnections
(
   pnuint16 maxConns
)
{
#if defined(N_PLAT_OS2)
   NWCCallGate(_NWC_GET_MAX_CONNS, maxConns);

#elif defined(N_PLAT_UNIX) || \
      defined(N_PLAT_NLM) || \
     (defined N_PLAT_MSW) || \
      defined(N_PLAT_DOS)

   nuint    maximumConns;
   nuint    bucket;
   nuint32  rcode;
   NWCDeclareAccess(access);

   rcode = NWCGetNumConns(&access, &maximumConns, &bucket, &bucket, &bucket);
   if (rcode == 0)
      *maxConns = (nuint16)maximumConns;
   else
      *maxConns = 0;

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtmaxcon.c,v 1.7 1994/09/26 17:47:07 rebekah Exp $
*/

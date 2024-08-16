/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:nwinit.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwncp.h"
#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwundoc.h"

/*manpage*NWCallsInit*******************************************************
SYNTAX:  NWCCODE N_API NWCallsInit
         (
            void NWPTR in,
            void NWPTR out
         )

REMARKS: Initializes the NWCALLS library. This includes setting up the
         double byte character tables (DBCS) and initializes the link
         with the appropriate requester.

ARGS:    Both arguments should be set to NULL

INCLUDE: nwmisc.h

RETURN:

SERVER:  n/a

CLIENT:  DOS WIN OS2 NT

NCP:     n/a

CHANGES: 17 Jun 1993 - added NT support - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWCallsInit
(
   void NWPTR in,
   void NWPTR out
)
{
   NWCDeclareAccess(access);

   out = out;
   in = in;

   return (NWCCODE) NWNCPInit(&access);
}

/*manpage*NWCallsTerm*****************************************************
SYNTAX:  NWCCODE N_API NWCallsTerm(void)

REMARKS: Terminates the NWCALLS library.

ARGS:    n/a

INCLUDE:

RETURN:

SERVER:  n/a

CLIENT:  NT

NCP:     n/a

CHANGES: 17 Jun 1993 - written - jwoodbur
****************************************************************************/
NWCCODE N_API NWCallsTerm(void)
{
   /* NWNCPTerm will call NWClientTerm so we don't need to */
   NWNCPTerm(NULL);

   return 0;
}

#ifdef N_PLAT_OS2

extern LOCK_SEM_NODE *head[3];
extern LOCK_SEM_NODE *tail[3];

N_GLOBAL_LIBRARY( nuint16 )
NWCallsEntryPoint
(
   void
)
{
   NWCMemSet(head, 0x00, sizeof(head));
   NWCMemSet(tail, 0x00, sizeof(tail));

   return (nuint16) (NWCallsInit(NULL, NULL) ? 1 : 0);
}
#endif

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/nwinit.c,v 1.7 1994/09/26 17:48:21 rebekah Exp $
*/

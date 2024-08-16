/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtactpst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetActiveProtocolStacks*****************************************
SYNTAX:  NWCCODE N_API NWGetActiveProtocolStacks
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_ACTIVE_STACKS NWPTR fseActiveStacks
         )

REMARKS:

ARGS: >  conn
      >  startNum
      <  fseActiveStacks

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 40  Active Protocol Stacks

CHANGES: 23 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetActiveProtocolStacks
(
   NWCONN_HANDLE       conn,
   nuint32             startNum,
   NWFSE_ACTIVE_STACKS NWPTR fseActiveStacks
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s40GetActiveProtoStacks(&access, startNum,
      (pNWNCPFSEVConsoleInfo) &fseActiveStacks->serverTimeAndVConsoleInfo,
      &fseActiveStacks->reserved, &fseActiveStacks->maxNumOfStacks,
      &fseActiveStacks->stackCount, &fseActiveStacks->nextStartNum,
      (pNWNCPFSEStackInfo) fseActiveStacks->stackInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtactpst.c,v 1.7 1994/09/26 17:46:53 rebekah Exp $
*/


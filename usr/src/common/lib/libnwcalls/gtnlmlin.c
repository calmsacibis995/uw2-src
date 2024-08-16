/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtnlmlin.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetNLMLoadedList************************************************
SYNTAX:  NWCCODE N_API NWGetNLMLoadedList
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_NLM_LOADED_LIST NWPTR fseNLMLoadedList
         )

REMARKS:

ARGS: >  conn
      >  startNum
      <  fseNLMLoadedList

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 10  Get NLM Loaded List

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNLMLoadedList
(
   NWCONN_HANDLE         conn,
   nuint32               startNum,
   NWFSE_NLM_LOADED_LIST NWPTR fseNLMLoadedList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s10GetNLMLoadedList(&access, startNum,
      (pNWNCPFSEVConsoleInfo) &fseNLMLoadedList->serverTimeAndVConsoleInfo,
      &fseNLMLoadedList->reserved,
      &fseNLMLoadedList->numberNLMsLoaded,
      &fseNLMLoadedList->NLMsInList,
      fseNLMLoadedList->NLMNums));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtnlmlin.c,v 1.7 1994/09/26 17:47:14 rebekah Exp $
*/


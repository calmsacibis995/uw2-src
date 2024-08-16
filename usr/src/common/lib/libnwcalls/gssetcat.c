/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gssetcat.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetServerSetCategories******************************************
SYNTAX:  NWCCODE N_API NWGetServerSetCategories
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_SERVER_SET_CATEGORIES NWPTR  fseServerSetCategories
         )

REMARKS:

ARGS: >  conn
      >  startNum
      <  fseServerSetCategories

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 61  Get Server Set Categories

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetServerSetCategories
(
   NWCONN_HANDLE        conn,
   nuint32              startNum,
   NWFSE_SERVER_SET_CATEGORIES NWPTR  fseServerSetCategories
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s61GetServerSetCategory(&access, startNum,
            (pNWNCPFSEVConsoleInfo)
            &fseServerSetCategories->serverTimeAndVConsoleInfo,
            &fseServerSetCategories->reserved,
            &fseServerSetCategories->nextSequenceNumber,
            &fseServerSetCategories->numberOfSetCategories,
            &fseServerSetCategories->categoryName[0],
            &fseServerSetCategories->categoryName[1]));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gssetcat.c,v 1.7 1994/09/26 17:46:50 rebekah Exp $
*/


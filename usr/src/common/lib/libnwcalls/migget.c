/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migget.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwmigrat.h"

/*manpage*NWGetDefaultSupportModule*****************************************
SYNTAX:  NWCCODE N_API NWGetDefaultSupportModule
         (
           NWCONN_HANDLE conn,
           pnuint32 moduleID
         )

REMARKS: This API is used to get the default Read/Write Support Module ID.

ARGS:

INCLUDE: nwmigrat.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 134 Set Default Read Write Support Module ID.

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDefaultSupportModule
(
   NWCONN_HANDLE  conn,
   pnuint32       moduleID
)
{
    NWCDeclareAccess(access);

    NWCSetConn(access, conn);

    return ((NWCCODE) NWNCP90s134GetSetVolDMStatus( &access, (nuint32) 0,
                        moduleID));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migget.c,v 1.7 1994/09/26 17:48:07 rebekah Exp $
*/

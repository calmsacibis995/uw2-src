/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:purgefil.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwdel.h"

/*manpage*NWPurgeErasedFiles************************************************
SYNTAX:  NWCCODE N_API NWPurgeErasedFiles
         (
            NWCONN_HANDLE conn
         );

REMARKS:

ARGS: >  conn

INCLUDE: nwdel.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 16  Purge Erased Files

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWPurgeErasedFiles
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP22s16DelPurge(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/purgefil.c,v 1.7 1994/09/26 17:48:42 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:delhndl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwdirect.h"

/*manpage*NWDeallocateDirectoryHandle***************************************
SYNTAX:  NWCCODE N_API NWDeallocateDirectoryHandle
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle
         )

REMARKS:

ARGS: >  conn,
      >  dirHandle

INCLUDE: nwdirect.h

RETURN:  0x0000  Successful
         0x899B  Bad Directory Handle

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 20  Deallocate Directory Handle

CHANGES: 13 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDeallocateDirectoryHandle
(
  NWCONN_HANDLE   conn,
  NWDIR_HANDLE    dirHandle
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP22s20FreeDirHandle(&access, (nuint8) dirHandle));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/delhndl.c,v 1.7 1994/09/26 17:45:08 rebekah Exp $
*/

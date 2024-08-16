/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setdrest.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwdirect.h"

/*manpage*NWSetDirSpaceLimit************************************************
SYNTAX:  NWCCODE N_API NWSetDirSpaceLimit
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint32        spaceLimit
         );

REMARKS: Set a space limit on a particular subdirectory.

         The limit is specified in 4K blocks.  If spaceLimit is 0, the
         space restrictions are lifted, if spaceLimit is -1L then the space
         limit on the directory is set to 0K.

ARGS: >  conn
      >  dirHandle
      >  suSpaceLimit

INCLUDE: nwdirect.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 36  Set Directory Disk Space Restriction

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetDirSpaceLimit
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint32        spaceLimit
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP22s36SetDirDiskSpcRest(&access, (nuint8) dirHandle,
               spaceLimit));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setdrest.c,v 1.7 1994/09/26 17:49:53 rebekah Exp $
*/

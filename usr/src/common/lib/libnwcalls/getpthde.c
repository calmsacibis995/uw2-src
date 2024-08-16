/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getpthde.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwdpath.h"

/*manpage*NWGetPathFromDirectoryEntry***************************************
SYNTAX:  NWCCODE N_API NWGetPathFromDirectoryEntry
         (
            NWCONN_HANDLE conn,
            nuint8  volNum,
            nuint16 dirEntry,
            pnuint8 len,
            pnstr8  pathName
         );

REMARKS:

ARGS: >  conn
      >  volNum
      >  dirEntry
      <> len
      <  pathName

INCLUDE: nwdpath.h

RETURN:

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 26  Get Path Name Of A Volume--Directory Number Pair

CHANGES: 14 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetPathFromDirectoryEntry
(
   NWCONN_HANDLE  conn,
   nuint8         volNum,
   nuint16        dirEntry,
   pnuint8        len,
   pnstr8         pathName
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP22s26GetPathName(&access, volNum, dirEntry,
      len, pathName);
   if (ccode != 0)
      len[0] = pathName[0] = 0;  /* do some zeroing on an error */

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getpthde.c,v 1.7 1994/09/26 17:46:19 rebekah Exp $
*/

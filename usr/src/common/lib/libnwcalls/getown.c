/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getown.c	1.4"
#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWGetOwningNameSpace**********************************************
SYNTAX:  NWCCODE N_API NWGetOwningNameSpace(
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 path,
            pnuint8 nameSpace)

REMARKS:

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetOwningNameSpace
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint8        nameSpace
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;
   nuint8 buNameSpace;

   buNameSpace = (nuint8) __NWGetCurNS(conn, dirHandle, path);

   ccode = NWGetNSEntryInfo(conn, dirHandle, path,
                           buNameSpace, buNameSpace,
                           0x8000, IM_OWNING_NAMESPACE, &entryInfo);

   if(!ccode && nameSpace)
      *nameSpace = (nuint8) entryInfo.NSCreator;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getown.c,v 1.6 1994/06/08 23:09:49 rebekah Exp $
*/

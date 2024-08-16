/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:tmphandl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwcaldef.h"
#include "nwnamspc.h"
#include "ncpfile.h"

/*manpage*NWAllocTempNSDirHandle********************************************
SYNTAX:  NWCCODE N_API NWAllocTempNSDirHandle
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 pbstrDirPath,
            nuint8 buNameSpace,
            NWDIR_HANDLE NWPTR newDirHandle
         )

REMARKS:

ARGS: >  conn
      >  dirHandle
      >  pbstrDirPath
      >  buNameSpace
      <  newDirHandle

INCLUDE: nwnamspc.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 12  Allocate Short Directory Handle

CHANGES: 22 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAllocTempNSDirHandle
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8 pbstrDirPath,
   nuint8 buNameSpace,
   NWDIR_HANDLE NWPTR newDirHandle
)
{
   NWCCODE ccode;
   NWNCPCompPath cPath;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   cPath.luDirBase    = (nuint32) dirHandle;
   cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
   NWNCPPackCompPath(-1, pbstrDirPath, -1, &cPath, (nflag32) 0);

   ccode = (NWCCODE) NWNCP87s12AllocDirHandle(&access, buNameSpace,
               (nuint8) 0, (nuint16) 0x0001, &cPath, newDirHandle, NULL,
               NULL);

   if (ccode != 0)
      *newDirHandle = (nuint8) 0;

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/tmphandl.c,v 1.7 1994/09/26 17:50:16 rebekah Exp $
*/

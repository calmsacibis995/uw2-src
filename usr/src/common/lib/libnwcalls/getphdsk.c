/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getphdsk.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetPhysicalDiskStats********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWGetPhysicalDiskStats
         (
            NWCONN_HANDLE  conn,
            nuint8         physicalDiskNum,
            PHYS_DSK_STATS N_FAR * statBuffer
         );

REMARKS:

ARGS:
      >  conn
      >  buPhysDiskNum
      <  pStatBuffer

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 216  Read Physical Disk Statistics

CHANGES: 10 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWGetPhysicalDiskStats
(
   NWCONN_HANDLE  conn,
   nuint8         physicalDiskNum,
   PHYS_DSK_STATS N_FAR * statBuffer
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(statBuffer, 0x00, (nuint) sizeof(*statBuffer));

   return ((NWCCODE) NWNCP23s216ReadPhyDiskStats(&access, physicalDiskNum,
               (pNWNCPDiskStats) statBuffer));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getphdsk.c,v 1.7 1994/09/26 17:46:17 rebekah Exp $
*/

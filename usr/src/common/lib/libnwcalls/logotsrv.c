/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:logotsrv.c	1.6"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#ifdef N_PLAT_OS2
# include <os2def.h>
# include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwcaldef.h"
#include "nwcint.h"
#include "nwundoc.h"
#include "nwerror.h"
#include "nwserver.h"
#include "nwclient.h"


/*manpage*NWLogoutFromFileServer*********************************************
SYNTAX:  NWCCODE N_API NWLogoutFromFileServer
         (
            NWCONN_HANDLE conn
         );

REMARKS: Logs out of specfied server.

         Under OS/2, calls are made to the NetWare IFS to delete any
         drive mappings that may be dependent on the connection, then
         the requester is called to perform the logout function.

ARGS: >  conn

INCLUDE: nwserver.h

RETURN:  INVALID_CONNECTION

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT NLM

SEE:

NCP:     n/a

CHANGES: 17 Jun 1993 - modified for NT - jwoodbur
         13 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWLogoutFromFileServer
(
   NWCONN_HANDLE conn
)
{
#if defined(N_PLAT_UNIX) || \
    defined(N_PLAT_NLM) || \
    defined N_PLAT_MSW || \
    defined(N_PLAT_DOS)

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWCUnauthenticateConnection(&access)); 

#elif defined(N_PLAT_OS2)
   nint8   DriveConTab[78], devName[3];
   nuint16 returnLength = 78, i;

   DosFSCtl((PSZ) DriveConTab, sizeof(DriveConTab), (pnuint16) &returnLength,
      NULL, 0, NULL, 0xC000 | 0x0004, (PSZ) "NETWARE", 0xffff, 3, 0L);

   for (i = 0; i < returnLength / 3; i++)
   {
      if (*(pnuint16)&DriveConTab[i*3] == conn && DriveConTab[i*3+2] != 1)
      {
         devName[0] = (char)i + (char)'A';
         devName[1] = ':';
         devName[2] = '\0';
         DosFSAttach((PSZ)devName, (PSZ)"NETWARE", NULL, 0, 1, 0L);
      }
   }
   return (NWCCallGate(_NWC_LOGOUT, &conn));

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/logotsrv.c,v 1.8 1994/09/26 17:47:56 rebekah Exp $
*/



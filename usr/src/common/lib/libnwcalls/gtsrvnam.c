/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtsrvnam.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwconnec.h"
#include "nwerror.h"
#include "nwserver.h"

/*manpage*NWGetFileServerName***********************************************
SYNTAX:  NWCCODE N_API NWGetFileServerName
         (
            NWCONN_HANDLE  conn,
            pnstr8         serverName
         );

REMARKS:

ARGS: >  conn
      <  serverName

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     NWGetConnInfo

NCP:     n/a

CHANGES: 13 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetFileServerName
(
   NWCONN_HANDLE  conn,
   pnstr8         serverName
)
{
   *serverName = '\0';
#if defined(N_PLAT_UNIX) || \
    defined N_PLAT_MSW || \
    defined(N_PLAT_DOS)
   {
      NWCDeclareAccess(access);

      NWCSetConn(access, conn);

    /*  NWCStrLen will ALWAYS return ZERO because we just stuffed
	 *  '\0' in '*serverName' above - DUH!!!
	 *return((NWCCODE)NWCGetConnInfo(&access, NWC_CONN_INFO_SERVER_NAME, 
     *						NWCStrLen(serverName), serverName));
	 */

        return((NWCCODE)NWCGetConnInfo(&access, NWC_CONN_INFO_SERVER_NAME, 
						NWC_MAX_SERVER_NAME_LEN, serverName));
   }
#else
   {
      return(NWGetConnInfo(conn, NW_SERVER_NAME, serverName));
   }
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtsrvnam.c,v 1.8 1994/09/26 17:47:30 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtconnum.c	1.5"
#include "ntypes.h"

#include "nwundoc.h"
#include "nwserver.h"
#include "nwclient.h"
#include "nwconnec.h"

/*manpage*NWGetConnectionNumber*********************************************
SYNTAX:  NWCCODE N_API NWGetConnectionNumber
         (
            NWCONN_HANDLE conn,
            NWCONN_NUM NWPTR connNumber
         )

REMARKS: This function obtains the workstation's connection number to the
         file server.

ARGS: >  conn
      <  connNumber

INCLUDE: nwconnec.h

RETURN:  INVALID_CONNECTION
         Under DOS, if the connection is less than 1, or greater than 8.

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 17 Sep 1993 - hungarian notation added - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetConnectionNumber
(
   NWCONN_HANDLE     conn,
   NWCONN_NUM NWPTR  connNumber
)
{
#if defined(N_PLAT_UNIX) || \
    defined N_PLAT_MSW || \
    defined(N_PLAT_DOS)
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWCGetConnInfo(&access, NWC_CONN_INFO_CONN_NUMBER, sizeof(nuint),
            connNumber));

#else
   return NWGetConnInfo(conn, NW_CONN_NUM, connNumber);
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtconnum.c,v 1.7 1994/09/26 17:46:56 rebekah Exp $
*/

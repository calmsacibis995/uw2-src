/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:qryaccin.c	1.4"
#include "nwcaldef.h"
#include "nwserver.h"
#include "nwbindry.h"
#include "nwacct.h"

/*manpage*NWQueryAccountingInstalled****************************************
SYNTAX:  NWCCODE N_API NWQueryAccountingInstalled
         (
            NWCONN_HANDLE  conn,
            pnuint8        installed
         )

REMARKS:

ARGS: >  conn
      <  pbuInstalled
         Returns 1 if accounting is installed on the fileserver otherwise 0

INCLUDE: nwacct.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 21 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWQueryAccountingInstalled
(
   NWCONN_HANDLE  conn,
   pnuint8        installed
)
{
   nuint8  acctVer;                   /* 1 means accounting installed */
   NWCCODE ccode, ccode2;
   nstr8   srvName[48];
   nuint32 sequence = (nuint32) -1;

   if((ccode = NWGetFileServerName(conn, srvName)) != 0)
      return (ccode);

   ccode = NWGetFileServerExtendedInfo(conn, &acctVer, NULL, NULL,
               NULL, NULL, NULL, NULL);

   ccode2 = NWScanProperty(conn, srvName, OT_FILE_SERVER,
               (pnstr8) "ACCOUNT_SERVERS", &sequence, NULL, NULL, NULL, NULL, NULL);

   *installed = (nuint8)(ccode == 0 && ccode2 == 0 && acctVer == 1);

   return ((NWCCODE) 0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/qryaccin.c,v 1.6 1994/06/08 23:12:13 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtaclbyt.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetActiveConnListByType*****************************************
SYNTAX:  NWCCODE N_API NWGetActiveConnListByType
         (
            NWCONN_HANDLE  conn,
            nuint32        startConnNum,
            nuint32        connType,
            NWFSE_ACTIVE_CONN_LIST NWPTR fseActiveConnListByType
         )

REMARKS:

ARGS:  > conn
       > startConnNum
       > connType
       < fseActiveConnListByType

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 14  Get Active Connection List by Type

CHANGES: 24 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
NWCCODE N_API NWGetActiveConnListByType
(
   NWCONN_HANDLE  conn,
   nuint32        startConnNum,
   nuint32        connType,
   NWFSE_ACTIVE_CONN_LIST NWPTR fseActiveConnListByType
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s14GetActiveConnsByType(&access, startConnNum,
            connType, (pNWNCPFSEVConsoleInfo)
            &fseActiveConnListByType->serverTimeAndVConsoleInfo,
            &fseActiveConnListByType->reserved,
            fseActiveConnListByType->activeConnBitList));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtaclbyt.c,v 1.7 1994/09/26 17:46:52 rebekah Exp $
*/


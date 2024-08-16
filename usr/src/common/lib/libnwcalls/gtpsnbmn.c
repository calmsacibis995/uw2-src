/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtpsnbmn.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetProtocolStkNumsByMediaNum************************************
SYNTAX:  NWCCODE N_API NWGetProtocolStkNumsByMediaNum
         (
            NWCONN_HANDLE  conn,
            nuint32        mediaNum,
            NWFSE_PROTOCOL_ID_NUMS NWPTR fseProtocolStkIDNums
         )

REMARKS:

ARGS:  > conn
       > mediaNum
       < fseProtocolStkIDNums


INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 44  Get Protocol Stack Number by Media Number

CHANGES: 23 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetProtocolStkNumsByMediaNum
(
   NWCONN_HANDLE          conn,
   nuint32                mediaNum,
   NWFSE_PROTOCOL_ID_NUMS NWPTR fseProtocolStkIDNums
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWNCP123s44GetProtNumByMedia(&access, mediaNum,
      (pNWNCPFSEVConsoleInfo)
      &fseProtocolStkIDNums->serverTimeAndVConsoleInfo,
      &fseProtocolStkIDNums->reserved,
      &fseProtocolStkIDNums->stackIDCount,
      fseProtocolStkIDNums->stackIDs));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtpsnbmn.c,v 1.7 1994/09/26 17:47:24 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtmnbymn.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"
#include "nwfse.h"

/*manpage*NWGetMediaNamebyMediaNum******************************************
SYNTAX:  NWCCODE N_API NWGetMediaNameByMediaNum
         (
            NWCONN_HANDLE  conn,
            nuint32        mediaNum,
            pnstr8         mediaName,
            NWFSE_MEDIA_NAME_LIST NWPTR fseMediaNameList
         )

REMARKS:

ARGS:

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 46  Get Media Name by Media Number

CHANGES: 23 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetMediaNameByMediaNum
(
  NWCONN_HANDLE         conn,
  nuint32               mediaNum,
  pnstr8                mediaName,
  NWFSE_MEDIA_NAME_LIST NWPTR fseMediaNameList
)
{
   NWCCODE ccode;
   nuint8 nameLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = (NWCCODE)NWNCP123s46GetMediaNameByNum(&access, mediaNum,
      (pNWNCPFSEVConsoleInfo) &fseMediaNameList->serverTimeAndVConsoleInfo,
      &fseMediaNameList->reserved, &nameLen, mediaName))==0)
   {
      mediaName[nameLen] = 0;
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtmnbymn.c,v 1.7 1994/09/26 17:47:10 rebekah Exp $
*/


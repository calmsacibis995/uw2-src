/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:chsvrri2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwserver.h"

/*manpage*NWChangeToClientRights2*******************************************
SYNTAX:  NWCCODE N_API NWChangeToClientRights2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber
         )

REMARKS:

ARGS:  > conn
       > queueID
       > jobNumber

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 133  Change To Client Rights
         23 116  Change To Client Rights

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWChangeToClientRights2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber
)
{
   nuint16 serverVer;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   queueID = NSwap32(queueID);

   if(serverVer >= 3110)
   {
      return ((NWCCODE) NWNCP23s133ChangeToClientRights(&access, queueID,
         jobNumber));
   }
   else
   {
      return ((NWCCODE) NWNCP23s116ChangeToClientRights(&access, queueID,
         (nuint16) jobNumber));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/chsvrri2.c,v 1.7 1994/09/26 17:44:21 rebekah Exp $
*/

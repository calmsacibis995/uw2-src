/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rename.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"
#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPRename*******************************************************
SYNTAX:  NWCCODE N_API NWAFPRename
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            nuint32        AFPSourceEntryID,
            nuint32        AFPDestEntryID,
            pnstr8         AFPSourcePath,
            pnstr8         AFPDstPath
         );

REMARKS:

ARGS: >  conn
      >  volNum
      >  AFPSourceEntryID
      >  AFPDestEntryID
      >  AFPSourcePath
      >  AFPDstPath

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 07  AFP Rename

CHANGES: 19 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPRename
(
  NWCONN_HANDLE   conn,
  nuint16         volNumber,
  nuint32         AFPSourceEntryID,
  nuint32         AFPDestEntryID,
  pnstr8          AFPSourcePath,
  pnstr8          AFPDestPath
)
{
    NWCDeclareAccess(access);

    NWCSetConn(access, conn);

    return ((NWCCODE) NWNCP35s7AFPRename(&access, (nuint8)volNumber,
               NSwap32(AFPSourceEntryID), NSwap32(AFPDestEntryID),
               *AFPSourcePath, &AFPSourcePath[1],
               *AFPDestPath, &AFPDestPath[1]));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rename.c,v 1.7 1994/09/26 17:49:05 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:deletefi.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPDelete*******************************************************
SYNTAX:  NWCCODE N_API NWAFPDelete
         (
            NWCONN_HANDLE conn,
            nuint16 volNum,
            nuint32 AFPEntryID,
            pnstr8  AFPPathString
         )

REMARKS: This call deletes a file or a directory.
         The Path Mod String can be null.
         Base Directory ID alone can specify the file or the directory.
         Directories to be deleted must be empty.
         Files to be deleted must be closed by all users.

ARGS: >  conn
      >  volNum
      >  AFPEntryID
      >  AFPPathString

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 03  AFP Delete

CHANGES: 19 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPDelete
(
  NWCONN_HANDLE conn,
  nuint16   suVolNum,
  nuint32   AFPEntryID,
  pnstr8    AFPPathString
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP35s3AFPDelete(&access, (nuint8) suVolNum,
               NSwap32(AFPEntryID), (nuint8) AFPPathString[0],
               (AFPPathString+1)));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/deletefi.c,v 1.7 1994/09/26 17:45:05 rebekah Exp $
*/

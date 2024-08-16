/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getbasei.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"
#include "nwdpath.h"

/*manpage*NWAFPGetEntryIDFromPathName***************************************
SYNTAX:  NWCCODE N_API NWAFPGetEntryIDFromPathName
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            pnuint32       AFPEntryID
         );

REMARKS: Converts a NetWare style path into a 32-bit Macintosh
         file or directory entry ID.

         Converts a length specified string containing a NetWare-style path
         specification into a unique 32-bit AFP File or Directory ID. The
         Directory Base and Path specifications are given in Netware "short
         name" format. The AFP Base ID is the unique 32-bit identifier of
         the file or directory.

ARGS: >  conn
      >  dirHandle
      >  path
      <  AFPEntryID (optional)


INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 12  AFP Get Entry ID From Path Name

CHANGES: 20 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPGetEntryIDFromPathName
(
  NWCONN_HANDLE   conn,
  NWDIR_HANDLE    dirHandle,
  pnstr8          path,
  pnuint32        AFPEntryID
)
{
   pnstr8  NWPath;
   nuint8  pathLen;
   nuint32 tempEntryID;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWPath = NWStripServerOffPath(path, NULL);
   pathLen   = (nuint8) NWCStrLen(NWPath);

   if ((ccode = (NWCCODE) NWNCP35s12AFPGEntryIDFrmPathNm(&access, dirHandle,
        pathLen, NWPath, &tempEntryID)) == 0)
   {
      if (AFPEntryID)
         *AFPEntryID = NSwap32(tempEntryID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getbasei.c,v 1.7 1994/09/26 17:45:44 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:conv2ent.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwclocal.h"

/*manpage*NWConvertPathToDirEntry*******************************************
SYNTAX:  NWCCODE N_API NWConvertPathToDirEntry
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            DIR_ENTRY NWPTR dirEntry
         );

REMARKS:

ARGS:
      >  conn
      >  dirHandle
      >  path
      <  dirEntry
         Pointer to structure to receive volume number/directory entry

         typedef struct
         {
            nuint8  volNumber;
            nuint32 dirEntry;
         } DIR_ENTRY;

INCLUDE: nwmisc.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 244  Convert Path To Dir Entry

CHANGES: 14 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWConvertPathToDirEntry
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   DIR_ENTRY      NWPTR dirEntry
)
{
   nstr8 tempPath[260];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(tempPath, path);
   NWConvertToSpecChar(tempPath);

   return ((NWCCODE) NWNCP23s244ConvertPathToEntry(&access,
               (nuint8) dirHandle, (nuint8) NWCStrLen(path), path,
               &dirEntry->volNumber, &dirEntry->dirEntry));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/conv2ent.c,v 1.7 1994/09/26 17:44:44 rebekah Exp $
*/

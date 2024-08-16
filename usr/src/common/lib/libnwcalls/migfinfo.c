/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migfinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwnamspc.h"
#include "nwmisc.h"
#include "nwmigrat.h"
#include "ncpfile.h"

/*manpage*NWGetDMFileInfo***************************************************
SYNTAX:  NWCCODE N_API NWGetDMFileInfo
         (
           NWCONN_HANDLE conn,
           NWDIR_HANDLE dirHandle,
           pnstr8   path,
           nuint8   namSpc,
           pnuint32 moduleID,
           pnuint32 restoreTime,
           pnuint32 dataStreams
         )

REMARKS: Gets the information about the data migrated files.

ARGS:  > conn
       < moduleID
         ID of the Support Module containing the migrated data.

       < restoreTime
         An estimate of how long it will take to retrieve the data.

       < dataStreams
         An array of data streams that are supported.

INCLUDE: nwmigrat.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 129  Data Migrator File Information

CHANGES: 14 Dec 1993 - NWNCP Enabled - alim
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetDMFileInfo
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8   path,
   nuint8   namSpc,
   pnuint32 moduleID,
   pnuint32 restoreTime,
   pnuint32 dataStreams
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetNSEntryInfo(conn, dirHandle, path, namSpc, namSpc,
                               SA_ALL, IM_DIRECTORY, &entryInfo)) != 0)
      return ccode;

   return ((NWCCODE) NWNCP90s129DMFileInfo(&access,
            (nuint32) entryInfo.volNumber, entryInfo.dirEntNum,
            (nuint32) namSpc, moduleID, restoreTime, dataStreams));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migfinfo.c,v 1.7 1994/09/26 17:48:06 rebekah Exp $
*/

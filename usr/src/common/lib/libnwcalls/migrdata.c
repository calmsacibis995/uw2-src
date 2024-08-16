/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migrdata.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"
#include "nwmigrat.h"

/*manpage*NWMoveFileFromDM***************************************************
SYNTAX:  NWCCODE N_API NWMoveFileFromDM
         (
           NWCONN_HANDLE conn,
           NWDIR_HANDLE dirHandle,
           pnstr8 path,
           nuint8 namSpc
         )

REMARKS: Move a file's data from some online long term storage media
         to a NetWare volume.

ARGS:

INCLUDE:

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 133  Move File Data From Data Migrator

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWMoveFileFromDM
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         namSpc
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetNSEntryInfo(conn, dirHandle, path, namSpc, namSpc,
                           SA_ALL, IM_DIRECTORY, &entryInfo)) != 0)
       return (ccode);

    return ((NWCCODE) NWNCP90s133MovDataFromDM( &access,
                        (nuint32) entryInfo.volNumber, entryInfo.dirEntNum,
                        (nuint32) namSpc));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migrdata.c,v 1.7 1994/09/26 17:48:08 rebekah Exp $
*/

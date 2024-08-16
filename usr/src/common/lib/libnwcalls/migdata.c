/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migdata.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"
#include "nwmigrat.h"

/*manpage*NWMoveFileToDM****************************************************
SYNTAX:  NWCCODE N_API NWMoveFileToDM
         (
           NWCONN_HANDLE conn,
           NWDIR_HANDLE dirHandle,
           pnstr8 path,
           nuint8  namSpc,
           nuint32 moduleID,
           nuint32 saveKeyFlag
         )

REMARKS: Moves a file's data to online long term storage media while leaving
         the file visible on a NetWare volume

ARGS:  > moduleID
         The assigned ID number of the support module that will migrate the
         data.

       > saveKeyFlag
         If this flag is set to SAVE_KEY_WHEN_FILE_IS_DEMIGRATED when the
         file is migrated then the key will be saved when the file is later
         de-migrated. A new data migrator key will not have to then be
         generated if the file is later re-migrated.

         This is very useful is the file is expected to be migrated and
         de-migrated a lot. Time will be saved because the file will not
         need to be deleted on the migrated media and the data will be
         checked to see if it has changed before it is migrated.

INCLUDE: nwmigrat.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 128 Move File Data To Data Migrator

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWMoveFileToDM
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         namSpc,
   nuint32        moduleID,
   nuint32        saveKeyFlag
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;
   NWCDeclareAccess(access);


   NWCSetConn(access, conn);

   if((ccode = NWGetNSEntryInfo(conn, dirHandle, path, namSpc, namSpc,
                               SA_ALL, IM_DIRECTORY, &entryInfo)) != 0)
       return (ccode);

    return ((NWCCODE) NWNCP90s128MovDataToDM( &access,
                        (nuint32) entryInfo.volNumber, entryInfo.dirEntNum,
                        (nuint32) namSpc, moduleID, saveKeyFlag));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migdata.c,v 1.7 1994/09/26 17:48:05 rebekah Exp $
*/

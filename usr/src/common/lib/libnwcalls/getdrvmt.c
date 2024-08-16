/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdrvmt.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFSDriveMapTable**********************************************
SYNTAX:  NWCCODE N_API NWGetFSDriveMapTable
         (
            NWCONN_HANDLE conn,
            DRV_MAP_TABLE N_FAR * tableBuffer
         );

REMARKS:

ARGS: >  conn
      <  tableBuffer

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23  215  Get Drive Mapping Table

CHANGES: 10 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFSDriveMapTable
(
   NWCONN_HANDLE  conn,
   DRV_MAP_TABLE  N_FAR * tableBuffer
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(tableBuffer, 0, (nuint) sizeof(*tableBuffer));

   return ((NWCCODE) NWNCP23s215GetDriveMapTable(&access,
         &tableBuffer->systemElapsedTime, &tableBuffer->SFTSupportLevel,
         &tableBuffer->logicalDriveCount, &tableBuffer->physicalDriveCount,
         tableBuffer->diskChannelTable, &tableBuffer->pendingIOCommands,
         tableBuffer->driveMappingTable, tableBuffer->driveMirrorTable,
         tableBuffer->deadMirrorTable, &tableBuffer->reMirrorDriveNumber,
         &tableBuffer->reserved, &tableBuffer->reMirrorCurrentOffset,
         tableBuffer->SFTErrorTable));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdrvmt.c,v 1.7 1994/09/26 17:45:52 rebekah Exp $
*/

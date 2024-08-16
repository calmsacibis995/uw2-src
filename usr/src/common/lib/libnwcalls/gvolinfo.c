/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gvolinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwvol.h"

/*manpage*NWGetVolumeInfoWithNumber*****************************************
SYNTAX:  NWCCODE N_API NWGetVolumeInfoWithNumber
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            pnstr8         volName,
            pnuint16       totalBlocks,
            pnuint16       sectorsPerBlock,
            pnuint16       availableBlocks,
            pnuint16       totalDirEntries,
            pnuint16       availableDirEntries,
            pnuint16       volIsRemovableFlag
         );

REMARKS:

ARGS: >  conn
      >  volNum
      <  volName
      <  totalBlocks
      <  sectorsPerBlock
      <  availableBlocks
      <  totalDirEntries
      <  availableDirEntries
      <  volIsRemovableFlag

INCLUDE: nwvol.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     18 --   Get Volume Info With Number

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetVolumeInfoWithNumber
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   pnstr8         volName,
   pnuint16       totalBlocks,
   pnuint16       sectorsPerBlock,
   pnuint16       availableBlocks,
   pnuint16       totalDirEntries,
   pnuint16       availableDirEntries,
   pnuint16       volIsRemovableFlag
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP18VolGetInfo(&access, (nuint8) volNum,
               sectorsPerBlock, totalBlocks, availableBlocks,
               totalDirEntries, availableDirEntries, volName,
               volIsRemovableFlag));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gvolinfo.c,v 1.7 1994/09/26 17:47:36 rebekah Exp $
*/

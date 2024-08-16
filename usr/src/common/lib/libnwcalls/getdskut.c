/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdskut.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwvol.h"
#include "nwbindry.h"

/*manpage*NWGetDiskUtilization**********************************************
SYNTAX:  NWCCODE N_API NWGetDiskUtilization
         (
            NWCONN_HANDLE  conn,
            nuint32        objID,
            nuint8         volNum,
            pnuint16       usedDirectories,
            pnuint16       usedFiles,
            pnuint16       usedBlocks
         )

REMARKS: Gets disk usage for a specified bindery object on a specified volume

ARGS: >  objID,
      >  volNum,
      <  usedDirectories,
      <  usedFiles,
      <  usedBlocks

INCLUDE: nwvol.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x8998   Disk Map Error
         0x89A1   Directory I/O Error
         0x89F2   No Object Read

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 14  Get Disk Utilization

CHANGES: 13 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDiskUtilization
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   nuint8         volNum,
   pnuint16       usedDirectories,
   pnuint16       usedFiles,
   pnuint16       usedBlocks
)
{
   NWCCODE ccode;
   nstr8 abstrObjName[48];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   /* NWGetObjectName will do the objID swap, so don't do it here */
   if((ccode = NWGetObjectName(conn, objID, abstrObjName, NULL)) != 0)
      return (ccode);

   if ((ccode = (NWCCODE)NWNCP23s14GetDiskUtilization(&access, volNum,
         NSwap32(objID), NULL, NULL, usedDirectories, usedFiles,
         usedBlocks)) != 0)
   {
      if(usedDirectories)
         *usedDirectories = (nuint16) 0;
      if(usedFiles)
         *usedFiles = (nuint16) 0;
      if(usedBlocks)
         *usedBlocks = (nuint16) 0;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdskut.c,v 1.7 1994/09/26 17:45:57 rebekah Exp $
*/

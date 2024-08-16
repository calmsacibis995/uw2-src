/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migvolst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwmigrat.h"

/*manpage*NWGetDMVolumeInfo*************************************************
SYNTAX:  NWCCODE N_API  NWGetDMVolumeInfo
         (
           NWCONN_HANDLE conn,
           nuint16 volNum,
           nuint32 moduleID,
           pnuint32 numFilesMigrated,
           pnuint32 totalMigratedSize,
           pnuint32 spaceUsedOnDM,
           pnuint32 limboSpaceUsedOnDM,
           pnuint32 spaceMigrated,
           pnuint32 filesInLimbo
         )

REMARKS:

ARGS: >  volNum
         Volume number that has the migrated files.

      >  moduleID
         ID of the support module to get the data for.

      <  numFilesMigrated
         Number of files from the selected volume that have been migrated.

      <  totalMigratedSize
         Total size needed to recover all the data on this selected volume.

      <  spaceUsedOnDM
         Size of the data on the migrator media.

      <  limboSpaceUsedOnDM
         Size of the data on the migrator area that has been demigrated.
         Since the data is generally read only the file will be kept on the
         migrator until the file is either deleted or remigrated with
         changes if the SAVE_KEY_WHEN_FILE_IS_DEMIGRATED was set when the
         file was migrated.

      <  spaceMigrated
         Total size of the migrated data for the volume including the limbo
         space used.

      <  fileInLimbo
         Number of files that are in limbo of that have been de-migrated
         with the SAVE_KEY_WHEN_FILE_IS_DEMIGRATED option and have not been
         migrated back to the data migrater.

INCLUDE: nwmigrat.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 130 Get Volume Data Migrator Status

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API  NWGetDMVolumeInfo
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   nuint32        moduleID,
   pnuint32       numFilesMigrated,
   pnuint32       totalMigratedSize,
   pnuint32       spaceUsedOnDM,
   pnuint32       limboSpaceUsedOnDM,
   pnuint32       spaceMigrated,
   pnuint32       filesInLimbo
)
{
   NWCCODE ccode;
   nuint32 luNumFilesMigrated;
   nuint32 luTotalMigratedSize;
   nuint32 luSpaceUsedOnDM;
   nuint32 luLimboSpaceUsedOnDM;
   nuint32 luSpaceMigrated;
   nuint32 luFilesInLimbo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = (NWCCODE) NWNCP90s130GetVolDMStatus( &access,
                           (nuint32) volNum, moduleID,
                           &luNumFilesMigrated, &luTotalMigratedSize,
                           &luSpaceUsedOnDM, &luLimboSpaceUsedOnDM,
                           &luSpaceMigrated, &luFilesInLimbo)) != 0)
       return (ccode);

   if(numFilesMigrated)
       *numFilesMigrated = luNumFilesMigrated;

   if(totalMigratedSize)
       *totalMigratedSize = luTotalMigratedSize;

   if(spaceUsedOnDM)
       *spaceUsedOnDM = luSpaceUsedOnDM;

   if(limboSpaceUsedOnDM)
      *limboSpaceUsedOnDM = luLimboSpaceUsedOnDM;

   if(spaceMigrated)
       *spaceMigrated = luSpaceMigrated;

   if(filesInLimbo)
       *filesInLimbo = luFilesInLimbo;

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migvolst.c,v 1.7 1994/09/26 17:48:13 rebekah Exp $
*/

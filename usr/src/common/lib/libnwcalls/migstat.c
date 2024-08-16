/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migstat.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwmigrat.h"
#include "ncpfile.h"

/*manpage*NWGetDataMigratorInfo*********************************************
SYNTAX:  NWCCODE N_API NWGetDataMigratorInfo
         (
           NWCONN_HANDLE conn,
           pnuint32 DMPresentFlag,
           pnuint32 majorVersion,
           pnuint32 minorVersion,
           pnuint32 DMSMRegistered
         )

REMARKS: Gets information about the data migrator.

ARGS:  > conn
       < DMPresentFlag
         If the flag is equal to -1, then the DM nlm has been loaded and is
         running.  If the flag is equal to 0, then the DM nlm in not loaded.

       < majorVersion
         Data migrator major version number.

       < minorVersion
         Data migrator minor version number.

       < DMSMRegistered
         Specifies whether or not the support module has been registered
         with the data migrater.

INCLUDE: nwmigrat.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 131  Migrator Status Information

CHANGES: 13 Dec 1993 - NWNCP Enabled - alim
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetDataMigratorInfo
(
  NWCONN_HANDLE conn,
  pnuint32 DMPresentFlag,
  pnuint32 majorVersion,
  pnuint32 minorVersion,
  pnuint32 DMSMRegistered
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP90s131MigratStatusInfo(&access, DMPresentFlag,
            majorVersion, minorVersion, DMSMRegistered));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migstat.c,v 1.7 1994/09/26 17:48:11 rebekah Exp $
*/

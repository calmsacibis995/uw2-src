/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gvolblvl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetVolumeInfoByLevel********************************************
SYNTAX:  NWCCODE N_API NWGetVolumeInfoByLevel
         (
            NWCONN_HANDLE  conn,
            nuint32        volNum,
            nuint32        infoLevel,
            NWFSE_VOLUME_INFO_BY_LEVEL NWPTR fseVolumeInfo
         )

REMARKS:

ARGS: >  conn
      >  volNum
      >  infoLevel
      <  fseVolumeInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 34  Get Volume Information By Level

CHANGES: 23 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetVolumeInfoByLevel
(
   NWCONN_HANDLE              conn,
   nuint32                    volNum,
   nuint32                    infoLevel,
   NWFSE_VOLUME_INFO_BY_LEVEL NWPTR fseVolumeInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s34GetVolInfoByLevel(&access,
         volNum, infoLevel,
         (pNWNCPFSEVConsoleInfo) &fseVolumeInfo->serverAndVConsoleInfo,
         &fseVolumeInfo->reserved, &fseVolumeInfo->infoLevel,
         (pNWNCPFSEVolInfo) &fseVolumeInfo->volumeInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gvolblvl.c,v 1.7 1994/09/26 17:47:35 rebekah Exp $
*/


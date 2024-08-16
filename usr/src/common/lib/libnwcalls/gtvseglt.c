/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtvseglt.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetVolumeSegmentList********************************************
SYNTAX:  NWCCODE N_API NWGetVolumeSegmentList
         (
            NWCONN_HANDLE  conn,
            nuint32        volNum,
            NWFSE_VOLUME_SEGMENT_LIST NWPTR fseVolumeSegmentList
         )

REMARKS:

ARGS: >  conn
      >  volNum
      <  fseVolumeSegmentList

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 33  Get Volume Segment List

CHANGES: 23 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetVolumeSegmentList
(
   NWCONN_HANDLE             conn,
   nuint32                   volNum,
   NWFSE_VOLUME_SEGMENT_LIST NWPTR fseVolumeSegmentList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s33GetVolSegmentList(&access, volNum,
      (pNWNCPFSEVConsoleInfo) &fseVolumeSegmentList->serverTimeAndVConsoleInfo,
      &fseVolumeSegmentList->reserved,
      &fseVolumeSegmentList->numOfVolumeSegments,
      (pNWNCPFSEVolSegment) fseVolumeSegmentList->volumeSegment));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtvseglt.c,v 1.7 1994/09/26 17:47:34 rebekah Exp $
*/


/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:chkaroot.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwerror.h"
#include "nwdirect.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"

/*manpage*CheckPathAtRoot***************************************************
SYNTAX:  NWCCODE N_API CheckPathAtRoot
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 pbstrPath
         )

REMARKS: Returns:
           0 if not at root
           non-Zero if at the root

ARGS: >  conn
      >  dirHandle
      >  pbstrPath

INCLUDE: nwundoc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 20 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API CheckPathAtRoot
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         pbstrPath
)
{
   NWCCODE ccode;
   pnstr8  pbstrTempPathPtr;
   nstr8   abstrDirHandlePath[300];

   if(dirHandle == 0)
   {
      if(pbstrPath == NULL)
         return (INVALID_PATH);

      /* Path is fully specified in pbstrPath */
      /* If last character is a ':' then path is at the root */
      pbstrTempPathPtr = (pnstr8) &pbstrPath[NWCStrLen(pbstrPath)];
      pbstrTempPathPtr = NWPrevChar(pbstrPath, pbstrTempPathPtr);
      ccode = (*pbstrTempPathPtr == ':') ? N_TRUE : N_FALSE;
   }
   else
   {
      /* Path is relative to dirHandle */
      /* If pbstrPath is valid, then the path is not at the root */
      if(pbstrPath != NULL && *pbstrPath != 0)
         ccode = N_FALSE;
      else
      {
         /* No path specified, check the directory handle path */
         if((ccode = NWGetDirectoryHandlePath(conn, dirHandle,
                        abstrDirHandlePath)) == 0)
         {
            pbstrTempPathPtr = &abstrDirHandlePath[NWCStrLen(abstrDirHandlePath)];
            pbstrTempPathPtr = NWPrevChar(abstrDirHandlePath, pbstrTempPathPtr);
            ccode = (*pbstrTempPathPtr == ':') ? N_TRUE : N_FALSE;
         }
         else
            ccode = N_FALSE;
      }
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/chkaroot.c,v 1.7 1994/06/08 23:07:53 rebekah Exp $
*/

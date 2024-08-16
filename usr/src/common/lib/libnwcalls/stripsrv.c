/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:stripsrv.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwclocal.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"

/*manpage*NWStripServerOffPath**********************************************
SYNTAX:  pnstr8  N_API NWStripServerOffPath
         (
            pnstr8 path,
            pnstr8 server
         )

REMARKS: Removes a server name from the front of a path.

         The server string will be copied to the buffer pointes to by
         server, if specified. The start of the path will be returned.

ARGS: >  path
      <  server (optional)
         Buffer where the server name should be placed (48 characters)

INCLUDE: nwdpath.h


RETURN:  Returns a pointer to the path after the server name.

SERVER:  n/a

CLIENT:  DOS WIN OS2

SEE:     NWParsePath, NWIndexPath, NWParseNetWarePath, NWCleanPath

NCP:     n/a

CHANGES: 04 Sep 1992 - Changed name and added to nwdpath.h - jwoodbur
         22 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
pnstr8 N_API NWStripServerOffPath
(
   pnstr8 path,
   pnstr8 server
)
{
   pnstr8 pbstrDir;
   pnstr8 pbstrVol;
   nint8 ch;

   if (path == NULL)
      return NULL;

   if(server != NULL)
      *server = 0x00;

   /** Check for a volume name **/
   for(pbstrDir = path; *pbstrDir && (*pbstrDir != ':');
            pbstrDir = NWNextChar(pbstrDir))
      ;

   if(*pbstrDir == 0x00)
      return(path);

   /** Check if server name exists **/
   for(pbstrVol = path; *pbstrVol && (*pbstrVol != ':' &&
            *pbstrVol != '/' && *pbstrVol != '\\'); pbstrVol = NWNextChar(pbstrVol))
      ;

   if(*pbstrVol == ':')
      return(path);

   /* Copy server name */
   if(server != NULL)
   {
      ch = *pbstrVol;
      *pbstrVol = 0x00;
      NWCStrCpy(server, path);
      *pbstrVol = ch;
   }
   pbstrVol = NWNextChar(pbstrVol);

   return(pbstrVol);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/stripsrv.c,v 1.7 1994/09/26 17:50:09 rebekah Exp $
*/

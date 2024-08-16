/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getlname.c	1.3"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWGetLongName*****************************************************
SYNTAX:  NWCCODE N_API NWGetLongName(
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 path,
            nuint8 srcNamSpc,
            nuint8 dstNamSpc,
            pnstr8 longName)

REMARKS:

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLongName
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         srcNamSpc,
   nuint8         dstNamSpc,
   pnstr8         longName
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;

   ccode = NWGetNSEntryInfo(conn, dirHandle, path, srcNamSpc, dstNamSpc,
                  0x8000, IM_ENTRY_NAME, &entryInfo);
   if(!ccode && longName)
   {
      NWCMemMove( longName, entryInfo.entryName, entryInfo.nameLength);
      longName[entryInfo.nameLength] = '\0';
   }

   return (ccode);
}

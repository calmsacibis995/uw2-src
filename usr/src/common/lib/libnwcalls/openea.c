/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:openea.c	1.4"
#include "nwcaldef.h"
#include "nwea.h"

/*manpage*NWOpenEA**********************************************************
SYNTAX:  NWCCODE N_API NWOpenEA
         (
           NWCONN_HANDLE conn,
           NWDIR_HANDLE dirHandle,
           pnstr8 path,
           pnstr8 EAName,
           nuint8 nameSpace,
           NW_EA_HANDLE N_FAR * EAHandle
         )

REMARKS: Fills the NW_EA_HANDLE structure so that it can be used by
         NWReadEA and NWWriteEA.

         The findfirst/next functions will also return an NW_EA_HANDLE
         structure, properly filled in.

         This function combines the functionality of NWGetDirectoryBase and
         NWGetEAHandleStruct into one call.

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWOpenEA
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnstr8         EAName,
   nuint8         nameSpace,
   NW_EA_HANDLE N_FAR * EAHandle
)
{
   NWCCODE ccode;
   NW_IDX idxStruct;

   if(EAName == NULL || EAHandle == NULL)
      return (0x89ff);

   if((ccode = NWGetDirectoryBase(conn, dirHandle, path, nameSpace,
                                 &idxStruct)) != 0)
   {
      return (ccode);
   }

   return (NWGetEAHandleStruct(conn, EAName, &idxStruct, EAHandle));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/openea.c,v 1.6 1994/06/08 23:11:58 rebekah Exp $
*/

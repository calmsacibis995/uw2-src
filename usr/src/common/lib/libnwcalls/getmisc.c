/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getmisc.c	1.4"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwnamspc.h"
#include "nwmisc.h"
#include "nwundoc.h"

/*manpage*NWNSGetMiscInfo***************************************************
SYNTAX:  NWCCODE N_API NWNSGetMiscInfo(
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 path,
            nuint8 dstNameSpace,
            NW_IDX NWPTR idxStruct)

REMARKS: Retrieves information to be used in further calls to the Name Space.

         The idxStruct is used by NetWare as an index to quickly locate a
         directory entry (file or directory).  This structure is required
         as a parameter for other functions and should not be modified by
         the application.

ARGS:
      <  idxStruct
         Pointer to an index structure.

         typedef struct
         {
            nuint8  volNumber;
            nuint8  srcNameSpace;
            nuint32 srcDirBase;
            nuint8  dstNameSpace;
            nuint32 dstDirBase;
         } NW_IDX;

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 27 Apr 1992 - modified - jwoodbur
            all fields in idx set to 0xff
         Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWNSGetMiscInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         dstNameSpace,
   NW_IDX NWPTR   idxStruct
)
{
   NWCCODE ccode;
   NW_ENTRY_INFO entryInfo;

   if(idxStruct == NULL)
      return (0x89ff);

   idxStruct->srcNameSpace = (nuint8) __NWGetCurNS(conn, dirHandle, path);
   idxStruct->dstNameSpace = dstNameSpace;
   if((ccode = NWGetNSEntryInfo(conn, dirHandle, path,
                  idxStruct->srcNameSpace, idxStruct->dstNameSpace,
                  0x8000, IM_DIRECTORY, &entryInfo)) == 0)
   {
      idxStruct->volNumber  = (nuint8) entryInfo.volNumber;
      idxStruct->srcDirBase = entryInfo.DosDirNum;
      idxStruct->dstDirBase = entryInfo.dirEntNum;
   }
   else
      NWCMemSet(idxStruct, 0xff, sizeof(NW_IDX));

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getmisc.c,v 1.6 1994/06/08 23:09:44 rebekah Exp $
*/

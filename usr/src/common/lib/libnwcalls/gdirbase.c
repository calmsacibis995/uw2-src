/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gdirbase.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwundoc.h"
#include "nwnamspc.h"

/*manpage*NWGetDirectoryBase************************************************
SYNTAX:  NWCCODE N_API NWGetDirectoryBase
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8 pbstrPath,
            nuint8 buDestNameSpace,
            NW_IDX NWPTR idxStruct
         )

REMARKS:

ARGS: >  conn,
      >  dirHandle,
      >  pbstrPath,
      >  buDestNameSpace,
      >  idxStruct

INCLUDE: nwnamspc.h

RETURN:  0x0000 SUCCESSFUL

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 22  Generate Directory Base and Volume Number

CHANGES: 14 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDirectoryBase
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         pbstrPath,
   nuint8         buDestNameSpace,
   NW_IDX         NWPTR idxStruct
)
{
   NWCCODE ccode;
   NWNCPCompPath compPath;
   nuint8 buNamSpc;
   nuint8 abuReserved[3];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   idxStruct->srcNameSpace = (nuint8) __NWGetCurNS(conn, dirHandle, pbstrPath);
   idxStruct->dstNameSpace = buDestNameSpace;
   buNamSpc = idxStruct->dstNameSpace;

   abuReserved[0] = (nuint8) 0;
   abuReserved[1] = (nuint8) 0;
   abuReserved[2] = (nuint8) 0;

   compPath.luDirBase = (nuint32) dirHandle;
   compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
   NWNCPPackCompPath((nint) -1, pbstrPath, (nint) -1, &compPath,
                     (nflag32) 0);

   ccode = (NWCCODE) NWNCP87s22GenDirBaseVolNum(&access, buNamSpc,
               abuReserved, &compPath, &idxStruct->dstDirBase,
               &idxStruct->srcDirBase, &idxStruct->volNumber);

   if(ccode)
      NWCMemSet(idxStruct, 0xff, sizeof(NW_IDX));

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gdirbase.c,v 1.7 1994/09/26 17:45:35 rebekah Exp $
*/

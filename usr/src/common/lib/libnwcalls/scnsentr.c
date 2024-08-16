/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnsentr.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwnamspc.h"

/*manpage*NWScanNSEntryInfo*************************************************
SYNTAX:  NWCCODE N_API NWScanNSEntryInfo
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            nuint8 buNameSpace,
            nuint16 suAttr,
            SEARCH_SEQUENCE NWPTR pIterHnd,
            pnstr8 pbstrSrchPattern,
            nuint32 luRetMask,
            NW_ENTRY_INFO NWPTR pEntryInfo
         );

REMARKS: Get directory entry information using a specific name space.

         This call can be used iteratively with wildcards.  On the first
         call, the srchDirNumber of the SEARCH_SEQUENCE structure should be
         set to -1.  After that, the server will manage the information.

ARGS:  > conn
       > dirHandle
       > buNameSpace
       > suAttr
      <> pIterHnd
       > pbstrSrchPattern
       > luRetMask
      <  pEntryInfo

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 02  Initialize Search
         87 03  Search For File or Subdirectory

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanNSEntryInfo
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   nuint8 buNameSpace,
   nuint16 suAttr,
   SEARCH_SEQUENCE NWPTR pIterHnd,
   pnstr8 pbstrSrchPattern,
   nuint32 luRetMask,
   NW_ENTRY_INFO NWPTR pEntryInfo
)
{
   NWCCODE ccode;
   nuint8 abstrTempSrchPattern[260];
   nuint8 buNull = '\0';

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(pIterHnd->searchDirNumber == (nuint32) -1L) /* Initialize search */
   {
      NWNCPCompPath cPath;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      /* The buNull tells ncp 87s2 to begin searching at the root
      directory  [lbendixs] */

      NWNCPPackCompPath(-1, (pnstr)&buNull, -1, &cPath, (nflag32)0);

      if((ccode = (NWCCODE) NWNCP87s2ScanFirst(&access, buNameSpace,
               (nuint8) 0, &cPath, (pNWNCPSearchSeq) pIterHnd)) != 0)
         return (ccode);
   }

   /* parse the string for wild cards and mark them */
   _NWFillWildPath(abstrTempSrchPattern, pbstrSrchPattern);

   return ((NWCCODE) NWNCP87s3ScanNext(&access, buNameSpace, (nuint8) 0,
      suAttr, luRetMask | 1, (pNWNCPSearchSeq) pIterHnd,
      abstrTempSrchPattern[0], &abstrTempSrchPattern[1], NULL,
      (pNWNCPEntryStruct) pEntryInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnsentr.c,v 1.7 1994/09/26 17:49:39 rebekah Exp $
*/

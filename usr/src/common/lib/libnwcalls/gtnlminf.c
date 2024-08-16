/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtnlminf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetNLMInfo******************************************************
SYNTAX:  NWCCODE N_API NWGetNLMInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        NLMNum,
            pnstr8         fileName,
            pnstr8         NLMName,
            pnstr8         copyright,
            NWFSE_NLM_INFO NWPTR fseNLMInfo
         )

REMARKS:

ARGS: >  conn
      >  NLMNum
      <  fileName
      <  NLMName
      <  copyright
      <  fseNLMInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 11  NLM Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNLMInfo
(
   NWCONN_HANDLE  conn,
   nuint32        NLMNum,
   pnstr8         fileName,
   pnstr8         NLMName,
   pnstr8         copyright,
   NWFSE_NLM_INFO NWPTR fseNLMInfo
)
{
   NWCCODE ccode;
   nuint8 fileNameLen, nameLen, copyRightLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE)NWNCP123s11GetNLMInfo(&access, NLMNum,
            (pNWNCPFSEVConsoleInfo) &fseNLMInfo->serverTimeAndVConsoleInfo,
            &fseNLMInfo->reserved, (pNWNCPFSENLMInfo) &fseNLMInfo->NLMInfo,
            &fileNameLen, fileName, &nameLen, NLMName,
            &copyRightLen, copyright);
   if (ccode == 0)
   {
      if (fileName)
         fileName[fileNameLen] = 0x00;
      if (NLMName)
         NLMName[nameLen] = 0x00;
      if (copyright)
         copyright[copyRightLen] = 0x00;
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtnlminf.c,v 1.7 1994/09/26 17:47:13 rebekah Exp $
*/


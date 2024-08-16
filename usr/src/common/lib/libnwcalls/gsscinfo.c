/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gsscinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetServerSetCommandsInfo****************************************
SYNTAX:  NWCCODE N_API NWGetServerSetCommandsInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_SERVER_SET_CMDS_INFO NWPTR  fseServerSetCmdsInfo
         )

REMARKS:

ARGS: >  conn
      >  startNum
      <  fseServerSetCmdsInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 60  Get Server Set Commands Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetServerSetCommandsInfo
(
   NWCONN_HANDLE     conn,
   nuint32           startNum,
   NWFSE_SERVER_SET_CMDS_INFO NWPTR  fseServerSetCmdsInfo
)
{
   NWCCODE ccode;
   nuint8 nameLen, valueLen;
   nstr8  tmpName[484], tmpValue[484];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE)NWNCP123s60GetServerSetInfo(&access, startNum,
         (pNWNCPFSEVConsoleInfo)
         &fseServerSetCmdsInfo->serverTimeAndVConsoleInfo,
         &fseServerSetCmdsInfo->reserved,
         &fseServerSetCmdsInfo->numberOfSetCommands,
         &fseServerSetCmdsInfo->nextSequenceNumber,
         &fseServerSetCmdsInfo->setCmdType,
         &fseServerSetCmdsInfo->setCmdCategory,
         &fseServerSetCmdsInfo->setCmdFlags,
         &nameLen, tmpName, &valueLen, tmpValue)) == 0)
   {
      pnuint8 pbuIndex = fseServerSetCmdsInfo->setNameAndValueInfo;

      *pbuIndex++ = nameLen;
       NWCMemMove(pbuIndex, tmpName, nameLen);
      pbuIndex   += nameLen;
      *pbuIndex++ = valueLen;
       NWCMemMove(pbuIndex, tmpValue, valueLen);
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gsscinfo.c,v 1.7 1994/09/26 17:46:49 rebekah Exp $
*/


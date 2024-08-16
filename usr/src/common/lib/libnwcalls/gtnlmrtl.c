/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtnlmrtl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwfse.h"

/*manpage*NWGetNLMsResourceTagList******************************************
SYNTAX:  NWCCODE N_API NWGetNLMsResourceTagList
         (
            NWCONN_HANDLE  conn,
            nuint32        NLMNum,
            nuint32        startNum,
            NWFSE_NLMS_RESOURCE_TAG_LIST NWPTR fseNLMsResourceTagList
         )

REMARKS:

ARGS: >  conn
      >  NLMNUM
      >  startNum
      <  fseNLMsResourceTagList

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 15  Get NLM's Resource Tag List

CHANGES: 24 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNLMsResourceTagList
(
   NWCONN_HANDLE     conn,
   nuint32           NLMNum,
   nuint32           startNum,
   NWFSE_NLMS_RESOURCE_TAG_LIST NWPTR fseNLMsResourceTagList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) (NWNCP123s15GetNLMResTagList(&access, NLMNum,
         startNum, (pNWNCPFSEVConsoleInfo)
         &fseNLMsResourceTagList->serverTimeAndVConsoleInfo,
         &fseNLMsResourceTagList->reserved,
         &fseNLMsResourceTagList->totalNumOfResourceTags,
         &fseNLMsResourceTagList->packetResourceTags,
         &fseNLMsResourceTagList->resourceTagBuf[0])));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtnlmrtl.c,v 1.7 1994/09/26 17:47:15 rebekah Exp $
*/


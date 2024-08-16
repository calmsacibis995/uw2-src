/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rdnsinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwnamspc.h"
#include "nwmisc.h"

/*manpage*NWReadNSInfo******************************************************
SYNTAX:  NWCCODE N_API NWReadNSInfo
         (
            NWCONN_HANDLE conn,
            NW_IDX NWPTR idxStruct,
            NW_NS_INFO NWPTR NSInfo,
            pnuint8 pbuData
         );


REMARKS: This is a NetWare 386 v3.11 call.  It replaces the earlier call Get Name
         Space Information (02222  22  47).

         This call gets specific name space information.  Note also that 1) this call
         is passed to the name space NLM and 2) this call is an expensive time user
         on the server.

         The NSInfoBitMask is explained in more detail in the Introduction to
         Directory Services.

ARGS: >  conn
      >  idxStruct
      >  NSInfo
         pbuData

INCLUDE: nwnamspc.h

RETURN:  0x0000 Successful

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 19  Get NS Information

CHANGES: 16 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReadNSInfo
(
   NWCONN_HANDLE  conn,
   NW_IDX NWPTR   idxStruct,
   NW_NS_INFO NWPTR NSInfo,
   pnuint8        pbuData
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP87s19NSGetInfo(&access, idxStruct->srcNameSpace,
                     idxStruct->dstNameSpace, (nuint8) 0,
                     idxStruct->volNumber, idxStruct->srcDirBase,
                     NSInfo->NSInfoBitMask, pbuData));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rdnsinfo.c,v 1.7 1994/09/26 17:48:46 rebekah Exp $
*/

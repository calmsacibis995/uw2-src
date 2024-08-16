/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:writeens.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWWriteExtendedNSInfo*********************************************
SYNTAX:  NWCCODE N_API NWWriteExtendedNSInfo(
           NWCONN_HANDLE conn,
           NW_IDX NWPTR idxStruct,
           NW_NS_INFO NWPTR NSInfo,
           pnuint8 pbuHugeDataB512)

REMARKS: Writes the extended (huge) name space
         information for the specified name space.

         The extendedInfoMask is a "read-only" information field that
         should be preserved from the NWReadExtendedNSInfo call.

         The dstNameSpace and dstDirBase from idxStruct are used to
         determine what entry to use for the write.

         The 16 byte hugeStateInfo array in the NSInfo structure should be
         set to all zeros before the first call.

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 27  Set Huge NS Information

CHANGES: 14 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWWriteExtendedNSInfo
(
   NWCONN_HANDLE     conn,
   NW_IDX NWPTR      idxStruct,
   NW_NS_INFO NWPTR  NSInfo,
   pnuint8           pbuHugeDataB512
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP87s27NSSetHugeInfo(&access, idxStruct->dstNameSpace,
                        idxStruct->volNumber, idxStruct->dstDirBase,
                        NSInfo->extendedBitMask, NSInfo->hugeStateInfo,
                        NSInfo->hugeDataLength, pbuHugeDataB512,
                        NSInfo->hugeStateInfo, &NSInfo->hugeDataLength));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/writeens.c,v 1.7 1994/09/26 17:50:34 rebekah Exp $
*/

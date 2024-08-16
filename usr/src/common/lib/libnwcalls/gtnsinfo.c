/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtnsinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWGetNSInfo*******************************************************
SYNTAX:  NWCCODE N_API NWGetNSInfo
         (
            NWCONN_HANDLE conn,
            NW_IDX NWPTR idxStruct,
            NW_NS_INFO NWPTR nsInfo
         );

REMARKS: Returns the NW_NS_INFO structure to be used in reading and writing
         information to the name space.

         The NSInfo will be returned for the destNameSpace in the idxStruct.

ARGS: >  conn
      >  idxStruct
         the structure returned from the NWNSGetMiscInfo function.
         The dstNameSpace will be used to obtain the NS info.

      <  nsInfo
         Pointer to a structure:

         typedef struct NWNSINFO
         {
            nuint32 NSInfoBitMask;
            nuint32 fixedBitMask;
            nuint32 reservedBitMask;
            nuint32 extendedBitMask;
            nuint16  fixedBitsDefined;
            nuint16  reservedBitsDefined;
            nuint16  extendedBitsDefined;
            nuint32 fieldsLenTable[32];
            nuint8  hugeStateInfo[16];
         } NW_NS_INFO;

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 23  Query NS Information Format

CHANGES: 14 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNSInfo
(
   NWCONN_HANDLE     conn,
   NW_IDX NWPTR      idxStruct,
   NW_NS_INFO NWPTR  nsInfo
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP87s23NSQueryInfoFormat(&access,
         (nuint8) idxStruct->dstNameSpace, (nuint8) idxStruct->volNumber,
         &nsInfo->fixedBitMask, &nsInfo->reservedBitMask,
         &nsInfo->extendedBitMask, &nsInfo->fixedBitsDefined,
         &nsInfo->reservedBitsDefined, &nsInfo->extendedBitsDefined,
         nsInfo->fieldsLenTable);

   if(ccode == 0)
   {
      nsInfo->NSInfoBitMask = (nsInfo->fixedBitMask |
                               nsInfo->reservedBitMask) & 0xFFFFFFFEL;
      NWCMemSet(nsInfo->hugeStateInfo, 0, (nuint) 16);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtnsinfo.c,v 1.7 1994/09/26 17:47:16 rebekah Exp $
*/

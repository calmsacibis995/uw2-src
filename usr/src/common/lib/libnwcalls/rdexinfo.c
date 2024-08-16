/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rdexinfo.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwnamspc.h"
#include "nwmisc.h"
#include "ncpfile.h"

/*manpage*NWReadExtendedNSInfo**********************************************
SYNTAX:  NWCCODE N_API NWReadExtendedNSInfo(
            NWCONN_HANDLE conn,
            NW_IDX NWPTR idxStruct,
            NW_NS_INFO NWPTR NSInfo,
            pnuint8 data)

REMARKS:

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 26  Get Huge NS Information

CHANGES: 6 Dec 1993 - NWNCP Enabled - alim/lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReadExtendedNSInfo
(
  NWCONN_HANDLE conn,
  NW_IDX NWPTR idxStruct,
  NW_NS_INFO NWPTR NSInfo,
  pnuint8 data
)
{
nint  j;
nuint8 abuNxtHugeStateInfo[16];
nuint8 temp;
NWCCODE ccode;
NWCDeclareAccess(access);

NWCSetConn(access, conn);


   if((ccode = (NWCCODE)NWNCP87s26NSGetHugeInfo(&access, idxStruct->dstNameSpace,
                  idxStruct->volNumber, idxStruct->dstDirBase,
                  NSInfo->extendedBitMask, NSInfo->hugeStateInfo,
                  abuNxtHugeStateInfo,
                  &NSInfo->hugeDataLength, data)) != 0)

   {
      /* if the hugeBitsDefined is set, but no huge information is found
         then an error code 8902 is returned.  This causes problems,
         therefore, this code was added to trap this condition.  If the OS
         is changed in the future to give some other kind of error, it can
         be eliminated.
      */
      if(ccode == 0x8902)
      {
         NSInfo->hugeDataLength = 0;
         return(NO_EXTENDED_NS_INFO);
      }
      return ccode;
   }

   NWCMemMove(NSInfo->hugeStateInfo, &abuNxtHugeStateInfo[0], 16);
   temp = (nuint8) 0;
   for(j=0; j < (nint) 16; j++)
      temp |= NSInfo->hugeStateInfo[j]; /* all zeros when done */

   if (!temp || (NSInfo->hugeDataLength == 0))
      return(NS_EOF);
   else
      return(MORE_NS_TO_READ);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rdexinfo.c,v 1.8 1994/09/26 17:48:44 rebekah Exp $
*/

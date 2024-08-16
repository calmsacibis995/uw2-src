/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s7.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s7EntrySetDOSInfo******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s7EntrySetDOSInfo
         (
            pNWAccess             pAccess,
            nuint8               buNamSpc,
            nuint8               buReserved,
            nuint16              suSrchAttrs,
            nuint32              luModifyDOSMask,
            pNWNCPModifyDosInfo  pInfo,
            pNWNCPCompPath       pCompPath
         );

REMARKS: Modifies DOS information while in another name space.

ARGS: <> pAccess
       > buNamSpc
       > buReserved
       > suSrchAttrs
       > luModifyDOSMask
       > pInfo
       > pCompPath

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 07  Modify File or SubDirectory DOS Information

CHANGES: 27 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s7EntrySetDOSInfo
(
   pNWAccess            pAccess,
   nuint8              buNamSpc,
   nuint8              buReserved,
   nuint16             suSrchAttrs,
   nuint32             luModifyDOSMask,
   pNWNCPModifyDosInfo pInfo,
   pNWNCPCompPath      pCompPath
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 7)
   #define INFO_LEN        ((nuint) 38)
   #define REQ_LEN         ((nuint) (9 + INFO_LEN))
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8  abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buReserved;
   NCopyLoHi16((pnuint16)&abuReq[3], &suSrchAttrs);
   NCopyLoHi32((pnuint32)&abuReq[5], &luModifyDOSMask);

/*
   Extended Attributes need to be part of the initial "word" of attrs
	in order for a set to work.
   NCopyLoHi16((pnuint16)&abuReq[9], &pInfo->suFileAttrs);
*/
   abuReq[9] = (nuint8)pInfo->suFileAttrs;
   abuReq[10] = pInfo->buFileXAttrs;
   abuReq[11] = pInfo->buFileMode;
   abuReq[12] = pInfo->buFileXAttrs;
   NCopyLoHi16((pnuint16)&abuReq[13], &pInfo->suCreationDate);
   NCopyLoHi16((pnuint16)&abuReq[15], &pInfo->suCreationTime);
   NCopyHiLo32((pnuint32)&abuReq[17], &pInfo->luCreatorID);
   NCopyLoHi16((pnuint16)&abuReq[21], &pInfo->suModifiedDate);
   NCopyLoHi16((pnuint16)&abuReq[23], &pInfo->suModifiedTime);
   NCopyHiLo32((pnuint32)&abuReq[25], &pInfo->luModifierID);
   NCopyLoHi16((pnuint16)&abuReq[29], &pInfo->suArchivedDate);
   NCopyLoHi16((pnuint16)&abuReq[31], &pInfo->suArchivedTime);
   NCopyHiLo32((pnuint32)&abuReq[33], &pInfo->luArchiverID);
   NCopyLoHi16((pnuint16)&abuReq[37], &pInfo->suLastAccessDate);
   NCopyLoHi16((pnuint16)&abuReq[39], &pInfo->suInheritanceGrantMask);
   NCopyLoHi16((pnuint16)&abuReq[41], &pInfo->suInheritanceRevokeMask);
   NCopyLoHi32((pnuint32)&abuReq[43], &pInfo->luMaxSpace);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   return NWCRequest(pAccess,
                     NCP_FUNCTION,
                     REQ_FRAGS,
                     reqFrag,
                     REPLY_FRAGS,
                     NULL,
                     NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s7.c,v 1.7 1994/09/26 17:39:42 rebekah Exp $
*/

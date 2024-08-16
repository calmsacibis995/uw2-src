/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:afpunpak.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpafp.h"
#include "unicode.h"
#include "nwclient.h"

/*manpage*NWCUnpackAFPPacket***************************************************
SYNTAX:  N_GLOBAL_LIBRARY( void )
         NWCUnpackAFPPacket
         (
            pNWNCPAFPFileInfo  pMacFileInfo,
            pnuint8         pbuRecPackedB120
         )

REMARKS: Unpacks the Apple File Protocol Packets and loads them into
         the Mac File Info structure pointed to by pMacFileInfo.

ARGS: >  pbuRecPackedB120
      <> pMacFileInfo

INCLUDE:

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2

CHANGES: 19 Aug 1993 - Written - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( void )
NWCUnpackAFPPacket
(
   pNWNCPAFPFileInfo  pMacFileInfo,
   pnuint8         pbuRecPackedB120
)
{
   NCopyHiLo32(&pMacFileInfo->luEntryID,         pbuRecPackedB120 +  0 );
   NCopyHiLo32(&pMacFileInfo->luParentID,        pbuRecPackedB120 +  4 );
   NCopyHiLo16(&pMacFileInfo->suAttr,            pbuRecPackedB120 +  8 );
   NCopyHiLo32(&pMacFileInfo->luDataForkLen,     pbuRecPackedB120 + 10 );
   NCopyHiLo32(&pMacFileInfo->luResourceForkLen, pbuRecPackedB120 + 14 );
   NCopyHiLo16(&pMacFileInfo->suTotalOffspring,  pbuRecPackedB120 + 18 );
   NCopyHiLo16(&pMacFileInfo->suCreationDate,    pbuRecPackedB120 + 20 );
   NCopyHiLo16(&pMacFileInfo->suAccessDate,      pbuRecPackedB120 + 22 );
   NCopyHiLo16(&pMacFileInfo->suModifyDate,      pbuRecPackedB120 + 24 );
   NCopyHiLo16(&pMacFileInfo->suModifyTime,      pbuRecPackedB120 + 26 );
   NCopyHiLo16(&pMacFileInfo->suBackupDate,      pbuRecPackedB120 + 28 );
   NCopyHiLo16(&pMacFileInfo->suBackupTime,      pbuRecPackedB120 + 30 );
   NWCMemMove(pMacFileInfo->abuFinderInfo,       pbuRecPackedB120 + 32, (nuint) 32);
   NWCMemMove(pMacFileInfo->abuLongName,         pbuRecPackedB120 + 64, (nuint) 32);
   NCopyHiLo32(&pMacFileInfo->luOwnerID,         pbuRecPackedB120 + 96 );
   NWCMemMove(pMacFileInfo->abuShortName,        pbuRecPackedB120 + 100, (nuint) 12);
   NCopyHiLo16(&pMacFileInfo->suAccessPrivileges,pbuRecPackedB120 + 112 );
   NWCMemMove(pMacFileInfo->abuProDOSInfo,       pbuRecPackedB120 + 114, (nuint) 6);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/afpunpak.c,v 1.7 1994/09/26 17:40:24 rebekah Exp $
*/

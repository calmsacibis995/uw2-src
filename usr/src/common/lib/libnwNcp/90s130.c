/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s130.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s130GetVolDMStatus**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s130GetVolDMStatus
         (
            pNWAccess    pAccess,
            nuint32     luVol,
            nuint32     luSupportModuleID,
            pnuint32    pluFilesMigrated,
            pnuint32    pluTotalMigratedSize,
            pnuint32    pluSpaceUsedOnDM,
            pnuint32    pluLimboSpaceUsedOnDM,
            pnuint32    pluSpaceMigrated,
            pnuint32    pluFilesInLimbo
         );

REMARKS:

ARGS: <> pAccess
       > luVol
       > luSupportModuleID
      <  pluFilesMigrated (optional)
      <  pluTotalMigratedSize (optional)
      <  pluSpaceUsedOnDM (optional)
      <  pluLimboSpaceUsedOnDM (optional)
      <  pluSpaceMigrated (optional)
      <  pluFilesInLimbo (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x8978   Service Unavailable on this Volume
         0x897E   Invalid Length
         0x8998   Invalid Volume

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     90 132  DM Support Module Information

NCP:     90 130  Volume DM status

CHANGES: 21 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s130GetVolDMStatus
(
   pNWAccess    pAccess,
   nuint32     luVol,
   nuint32     luSupportModuleID,
   pnuint32    pluFilesMigrated,
   pnuint32    pluTotalMigratedSize,
   pnuint32    pluSpaceUsedOnDM,
   pnuint32    pluLimboSpaceUsedOnDM,
   pnuint32    pluSpaceMigrated,
   pnuint32    pluFilesInLimbo
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 130)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 24)

   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   nuint32 ccode;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16( &abuReq[0], &suNCPLen );

   abuReq[2] = NCP_SUBFUNCTION; /* SubFunc */
   NCopyLoHi32( &abuReq[3], &luVol );
   NCopyLoHi32( &abuReq[7], &luSupportModuleID );


   ccode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuRep, NCP_REPLY_LEN, NULL);

   if ( ccode != 0 )
      return (ccode);

   if ( pluFilesMigrated )
      NCopyLoHi32( pluFilesMigrated, &abuRep[0] );
   if ( pluTotalMigratedSize )
      NCopyLoHi32( pluTotalMigratedSize, &abuRep[4] );
   if ( pluSpaceUsedOnDM )
      NCopyLoHi32( pluSpaceUsedOnDM, &abuRep[8] );
   if ( pluLimboSpaceUsedOnDM )
      NCopyLoHi32( pluLimboSpaceUsedOnDM, &abuRep[12] );
   if ( pluSpaceMigrated )
      NCopyLoHi32( pluSpaceMigrated, &abuRep[16] );
   if ( pluFilesInLimbo )
      NCopyLoHi32( pluFilesInLimbo, &abuRep[20] );

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s130.c,v 1.7 1994/09/26 17:40:18 rebekah Exp $
*/

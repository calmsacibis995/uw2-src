/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s134.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s134GetSetVolDMStatus**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s134GetSetVolDMStatus
         (
            pNWAccess pAccess,
            nuint32    luGetSetFlag,
            pnuint32   pluSupportModuleID,
         );

REMARKS:

ARGS: <> pAccess
      >  luGetSetFlag
      <> pluSupportModuleID

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x897E   Invalid Length

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     90 132 DM Support Module Information

NCP:     90 134 Get/Set Read-Write Support Module ID

CHANGES: 21 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s134GetSetVolDMStatus
(
   pNWAccess pAccess,
   nuint32  luGetSetFlag,
   pnuint32 pluSupportModuleID
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 134)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 4)

   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   nuint32 ccode;

   suNCPLen = NCP_STRUCT_LEN;
   abuReq[2] = NCP_SUBFUNCTION; /* SubFunc */

   NCopyHiLo16( &abuReq[0], &suNCPLen );
   NCopyLoHi32( &abuReq[3], &luGetSetFlag );

   if ( pluSupportModuleID )
      NCopyLoHi32( &abuReq[7], pluSupportModuleID );
   else
      NWCMemSet( &abuReq[7], 0, 4 );


   ccode = (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuRep, NCP_REPLY_LEN, NULL));

   if ( ccode != 0 )
      return (ccode);

   if ( pluSupportModuleID )
      NCopyLoHi32(pluSupportModuleID, &abuRep[0] );

   return((NWRCODE) 0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s134.c,v 1.7 1994/09/26 17:40:22 rebekah Exp $
*/

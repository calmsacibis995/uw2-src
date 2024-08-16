/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s133.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s133MovDataFromDM**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s133MovDataFromDM
         (
            pNWAccess pAccess,
            nuint32    luVol,
            nuint32    luDirEntry,
            nuint32    luNameSpace,
         );

REMARKS:

***********************WARNING********************************
Abends the server with an invalid name space.

ARGS: <> pAccess
      >  luVol
      >  luDirEntry
      >  luNameSpace

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x8978   Service Unavailable on this Volume
         0x897E   Invalid Length
         0x8998   Invalid Volume
         0x899C   Invalid Directory Entry
         0x89A8   Access Denied

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     90 128 Move File Data To DM

NCP:     90 133 Move File Data From DM

CHANGES: 21 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s133MovDataFromDM
(
   pNWAccess pAccess,
   nuint32  luVol,
   nuint32  luDirEntry,
   nuint32  luNameSpace
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 133)
   #define NCP_STRUCT_LEN  ((nuint16) 13)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 0)


   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   abuReq[2] = NCP_SUBFUNCTION;  /* SubFunc */

   NCopyHiLo16( &abuReq[0], &suNCPLen );
   NCopyLoHi32( &abuReq[3], &luVol );
   NCopyLoHi32( &abuReq[7], &luDirEntry );
   NCopyLoHi32( &abuReq[11], &luNameSpace );


   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s133.c,v 1.7 1994/09/26 17:40:21 rebekah Exp $
*/

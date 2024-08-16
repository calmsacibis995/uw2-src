/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s128.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s128MovDataToDM**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s128MovDataToDM
         (
            pNWAccess pAccess,
            nuint32    luVol,
            nuint32    luDirEntry,
            nuint32    luNameSpace,
            nuint32    luSupportModuleID,
            nuint32    luDMFlags,
         );

REMARKS:

ARGS: <> pAccess
      >  luVol
      >  luDirEntry
      >  luNameSpace
      >  luSupportModuleID
      >  luDMFlags,

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x897E   Invalid Length
         0x8998   Invalid Volume
         0x899C   Invalid Directory Entry
         0x89A8   Invalid Support Module ID

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     90 133 Move File Data From DM

NCP:     90 128 Move File Data To DM

CHANGES: 21 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s128MovDataToDM
(
   pNWAccess    pAccess,
   nuint32     luVol,
   nuint32     luDirEntry,
   nuint32     luNameSpace,
   nuint32     luSupportModuleID,
   nuint32     luDMFlags
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 128)
   #define NCP_STRUCT_LEN  ((nuint16) 21)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16( &abuReq[0], &suNCPLen );
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32( &abuReq[3], &luVol );
   NCopyLoHi32( &abuReq[7], &luDirEntry );
   NCopyLoHi32( &abuReq[11], &luNameSpace );
   NCopyLoHi32( &abuReq[15], &luSupportModuleID );
   NCopyLoHi32( &abuReq[19], &luDMFlags );


   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s128.c,v 1.7 1994/09/26 17:40:15 rebekah Exp $
*/

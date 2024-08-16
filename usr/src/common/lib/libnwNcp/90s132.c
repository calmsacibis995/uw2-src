/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:90s132.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP90s132GetDMSupportModInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP90s132GetDMSupportModInfo
         (
            pNWAccess   pAccess,
            nuint32    luInfoLevel,
            nuint32    luSupportModuleID,
            pnuint8    pbuInfoBuf,
            pnuint32   pluInfoBufSize
         );

REMARKS:

ARGS: <> pAccess
       > luInfoLevel
       > luSupportModuleID
      <  pbuInfoBuf
      <> pluInfoBufSize

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful
         0x897E   Invalid Length
         0x89A8   Invalid Support Module ID
         0x89FF   Failure or Invalid Info Level

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     90 130  Volume DM status

NCP:     90 132  DM Support Module Information

CHANGES: 21 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP90s132GetDMSupportModInfo
(
    pNWAccess   pAccess,
    nuint32    luInfoLevel,
    nuint32    luSupportModuleID,
    pnuint8    pbuInfoBuf,
    pnuint32   pluInfoBufSize
)
{
   #define NCP_FUNCTION    ((nuint) 90)
   #define NCP_SUBFUNCTION ((nuint8) 132)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define NCP_REQ_LEN     ((nuint) 2 + NCP_STRUCT_LEN)
   #define NCP_REPLY_LEN   ((nuint) 512)

   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   nuint32 ccode;
   nuint   repLen = 0;
   nuint32 luNumberOfSMs;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16( &abuReq[0], &suNCPLen );

   abuReq[2] = NCP_SUBFUNCTION; /* SubFunc */

   NCopyLoHi32( &abuReq[3], &luInfoLevel );
   NCopyLoHi32( &abuReq[7], &luSupportModuleID );

   ccode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuRep, NCP_REPLY_LEN, &repLen);

   if (ccode != 0)
      return (ccode);

   if (*pluInfoBufSize < (nuint) repLen)
      return (NWRCODE) 0x89FF;

   switch( luInfoLevel )
   {
      case 0:
         NCopyLoHi32( pbuInfoBuf, &abuRep[0] ); /* IOStatus */
         NCopyLoHi32( (pbuInfoBuf+4), &abuRep[4] ); /* InfoBlockSize */
         NCopyLoHi32( (pbuInfoBuf+8), &abuRep[8] ); /* AvailSpace */
         NCopyLoHi32( (pbuInfoBuf+12), &abuRep[12] ); /* UsedSpace */
         NWCMemMove( (pbuInfoBuf+16), &abuRep[16], repLen-16 );
         break;

      case 1:
         {
         int i = 0;

            NCopyLoHi32( &luNumberOfSMs, &abuRep[0] );
            NCopyLoHi32( pbuInfoBuf, &abuRep[0] );
            while( luNumberOfSMs-- )
            {
               NCopyLoHi32( (pbuInfoBuf+4+i), &abuRep[i+4] );
               i += 4;
            }
         }
         break;

      default:
         NWCMemMove( pbuInfoBuf, abuRep, repLen );
         break;
   }

   *pluInfoBufSize = repLen;

   return ((NWRCODE) 0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/90s132.c,v 1.7 1994/09/26 17:40:20 rebekah Exp $
*/

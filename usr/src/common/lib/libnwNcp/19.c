/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:19.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP19GetStationNum**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP19GetStationNum
         (
            pNWAccess pAccess,
            pnuint8  pbuStationNumB3,
         );

REMARKS: This request returns the calling station's 3-byte station number.

ARGS: <> pAccess
      <  pbuStationNumB3

INCLUDE: ncpconn.h

RETURN:  0x0000    Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 22  Get Station's Logged Info

NCP:     19 --  Get Station Number

CHANGES: 29 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP19GetStationNum
(
   pNWAccess pAccess,
   pnuint8  pbuStationNumB3
)
{
   #define NCP_FUNCTION    ((nuint) 19)
   #define REQ_LEN         ((nuint) 0)
   #define REPLY_LEN       ((nuint) 3)

   nint32   lCode;
   nuint8  abuReply[REPLY_LEN];

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, NULL, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      nint i;

      for (i = 0; i < 3; i++)
         pbuStationNumB3[i] = abuReply[i];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/19.c,v 1.7 1994/09/26 17:33:23 rebekah Exp $
*/

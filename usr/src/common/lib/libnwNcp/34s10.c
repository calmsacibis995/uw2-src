/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s10.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s10TTSSetTransBits******************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s10TTSSetTransBits
         (
            pNWAccess pAccess,
            nuint8   buFlag,
         );

REMARKS: Allows a client to set the transaction bits in the
         Control Flags byte. Only bit 0 is used currently, if bit 0 is set
         forced record locking is "on".  The default setting is record
         locking "on".

ARGS: <> pAccess
      >  buFlag

INCLUDE: ncptts.h

RETURN:  0x0000  Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 09  TTS Get Transaction Bits

NCP:     34 10  TTS Set Transaction Bits

CHANGES: 13 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s10TTSSetTransBits
(
   pNWAccess pAccess,
   nuint8   buFlag
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 10)
   #define REQ_LEN         ((nuint) 2)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buFlag;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s10.c,v 1.7 1994/09/26 17:37:59 rebekah Exp $
*/

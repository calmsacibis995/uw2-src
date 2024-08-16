/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s9.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s9TTSGetTransBits**********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s9TTSGetTransBits
         (
            pNWAccess pAccess,
            pnuint8  pbuFlags,
         )

REMARKS: Returns the transaction bits (Control Flags) for the task.
         The bits are defined as follows:

         Bit    Description
         ---    -----------
         0      0 - forced record locking is off
                1 - forced record locking is on
         1-7    reserved

ARGS: <> pAccess,
      <  pbuFlags,


INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 09  TTS Get Transaction Bits

CHANGES: 13 Aug 1993 -written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s9TTSGetTransBits
(
   pNWAccess pAccess,
   pnuint8  pbuFlags
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 9)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 1)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;

   return( NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
                            pbuFlags, REPLY_LEN, NULL) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s9.c,v 1.7 1994/09/26 17:38:09 rebekah Exp $
*/

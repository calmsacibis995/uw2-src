/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s0TTSIsAvailable*******************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s0TTSIsAvailable
         (
            pNWAccess pAccess,
         )

REMARKS:

ARGS: <  pAccess

INCLUDE: ncptts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 00  TTS Is Available

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s0TTSIsAvailable
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 0)
   #define NCP_STRUCT_LEN  ((nuint16) 0)

   return( NWCRequestSimple(pAccess, NCP_FUNCTION, NCP_STRUCT_LEN,
               NCP_SUBFUNCTION) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s0.c,v 1.7 1994/09/26 17:37:57 rebekah Exp $
*/

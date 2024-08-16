/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s207.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP23s207DisableTracking**************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP23s207DisableTracking
         (
            pNWAccess pAccess,
         );

REMARKS: Allows an operator to disable the Transaction Tracking functions
         of an SFT file server.  Typically, an operator will disable
         Transaction Tracking when a volume containing the transaction file
         is filled to capacity.  Then, to make space on the volume, the
         operator (or other users) will move or remove unnecessary files
         from the volume.

         The operator can re-enable Transaction Tracking by using the
         Enable Transaction Tracking call (0x2222 23 208).

         If the calling station does not have operator privileges, the No
         Console Rights completion code is returned and the server's
         Transaction Tracking status remains unchanged.

ARGS: <> pAccess

INCLUDE: ncptts.h

RETURN:

SERVER:  2.2

CLIENT:  DOS OS2 WIN

SEE:     23 208  Enable Transaction Tracking
         23 213  Get Transaction Tracking Statistics

NCP:     23 207  Disable Transaction Tracking

CHANGES: 18 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s207DisableTracking
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION         ((nuint)     23)
   #define NCP_SUBFUNCTION      ((nuint8)   207)
   #define NCP_STRUCT_LEN       ((nuint16)    1)
   #define NCP_REQ_LEN          ((nuint)      3)
   #define NCP_REP_LEN          ((nuint)      0)

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return( NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, NULL,
                  NCP_REP_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s207.c,v 1.7 1994/09/26 17:36:01 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:lenstr.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPASCIIZToLenStr***********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWAFPASCIIZToLenStr
         (
            pnstr8 dstStr,
            pnstr8 srcStr
         )

REMARKS: Changes a null terminated string to a length preceeded string.

ARGS:

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: Art 9/22/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWAFPASCIIZToLenStr
(
   pnstr8 dstStr,
   pnstr8 srcStr
)
{
   nuint16 len;

   len = (nuint16) NWCStrLen((pnstr)srcStr);
   if(len > 255)
      return (NWCCODE)(len - 255);

   *dstStr = (nuint8)len;
   NWCMemMove(++dstStr, srcStr, len );

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/lenstr.c,v 1.7 1994/09/26 17:47:43 rebekah Exp $
*/

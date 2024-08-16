/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gcmpthl2.c	1.4"
#include "ntypes.h"
#include "nwundoc.h"

/*manpage*_NWGetComPathLen2*************************************************
SYNTAX:  nuint16 N_API _NWGetComPathLen2
         (
            PHSTRUCT NWPTR comPath
         )

REMARKS:

ARGS: >  comPath

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 20 Sep 1993 - hungarian notation added - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
nuint16 N_API _NWGetComPathLen2
(
   PHSTRUCT NWPTR cPath
)
{
   nuint16 suCount=0;
   nuint16 suLen;
   pnuint8 pbuidx;

   suLen  = cPath->srcCompCount; /* get number of components */
   suLen += cPath->destCompCount;
   pbuidx  = cPath->componentPath;       /* point to first component  */
   while(suLen)
   {
      suCount += (nuint16)*pbuidx + 1;  /* add length + one byte   */
      pbuidx += (*pbuidx + 1);        /* point to next component   */
      suLen--;
   }
   return ((nuint16) (suCount + (nuint16) 14));          /* add the first 14 bytes of struc */
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gcmpthl2.c,v 1.6 1994/06/08 23:09:05 rebekah Exp $
*/

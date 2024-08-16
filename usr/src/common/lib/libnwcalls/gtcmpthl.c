/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtcmpthl.c	1.4"
#include "ntypes.h"
#include "nwcaldef.h"
#include "nwundoc.h"

/*manpage*_NWGetCompathStructLength*****************************************
SYNTAX:  nuint16 N_API _NWGetCompathStructLength
         (
            pnuint8 pbuCPath
         )

REMARKS:

ARGS:

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 21 Sep 1993 - Hungarian Notation Added - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
nuint16 N_API _NWGetCompathStructLength
(
  pnuint8 pbuCPath
)
{
   nuint16 suCount=0;
   nuint16 suLen;
   pnuint8 pbuIdx;

  pbuIdx = pbuCPath + 6;          /* pointer to number of components*/
  suLen = (nuint8)*pbuIdx;           /* get the number    */
  pbuIdx++;                      /* point to first component  */
  while(suLen)
  {
    suCount += (nuint16)*pbuIdx + 1;  /* add length + one byte   */
    pbuIdx += (*pbuIdx + 1);        /* point to next component   */
    suLen--;
  }
  return(suCount + 7);          /* add the first 7 bytes of struc */
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtcmpthl.c,v 1.6 1994/06/08 23:10:27 rebekah Exp $
*/

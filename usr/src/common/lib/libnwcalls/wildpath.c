/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:wildpath.c	1.4"
#include "ntypes.h"
#include "nwcaldef.h"
#include "nwundoc.h"

/*manpage*__NWFillWildPath**************************************************
SYNTAX:  void N_API __NWFillWildPath
         (
            pnuint8  pbuPath,
            pnstr8   pbstrDirPath,
            nuint16  suAugmentFlag
         )

REMARKS:

ARGS: <  pbuPath
      >  pbstrDirPath
      >  suAugmentFlag

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 22 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API __NWFillWildPath
(
   pnuint8  pbuPath,
   pnstr8   pbstrDirPath,
   nuint16  suAugmentFlag
)
{
   nuint8 buCompCount;

   _NWMakeComponentPath(&buCompCount, pbuPath, pbstrDirPath, suAugmentFlag);
}

void N_API _NWFillWildPath
(
   pnuint8 pbuBuf,
   pnstr8  pbstrPath
)
{
   __NWFillWildPath(pbuBuf, pbstrPath, (nuint16) 0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/wildpath.c,v 1.6 1994/06/08 23:13:59 rebekah Exp $
*/

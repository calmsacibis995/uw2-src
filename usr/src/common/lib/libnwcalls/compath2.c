/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:compath2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwundoc.h"

/*manpage*__NWFillComponentPath2********************************************
SYNTAX:  void N_API __NWFillComponentPath2
         (
            PHSTRUCT NWPTR cPath,
            pnuint8 pbuSrcPath,
            pnuint8 pbuDestPath,
            nuint16 suAugmentFlag
         )

REMARKS:

ARGS: >  cPath
      >  pbuSrcPath
      >  pbuDestPath
      >  suAugmentFlag

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 14 Sep 1993 - modified to comply with standards (hungarian) - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API __NWFillComponentPath2
(
   PHSTRUCT NWPTR cPath,
   pnuint8  pbuSrcPath,
   pnuint8  pbuDestPath,
   nuint16  suAugmentFlag
)
{
   nuint16 suCompCnt, suLen;

  _NWMakeComponentPath(&cPath->srcCompCount, cPath->componentPath, pbuSrcPath,
            suAugmentFlag);
  pbuSrcPath = (pnuint8) cPath->componentPath;
  for(suCompCnt = (nuint16)cPath->srcCompCount; suCompCnt; suCompCnt--)
  {
    suLen = (nuint16) *pbuSrcPath++;
    pbuSrcPath += suLen;
  }
  _NWMakeComponentPath(&cPath->destCompCount, pbuSrcPath, pbuDestPath, suAugmentFlag);
}

void N_API _NWFillComponentPath2
(
  PHSTRUCT NWPTR cPath,
  pnuint8 pbuSrcPath,
  pnuint8 pbuDestPath
)
{
  __NWFillComponentPath2(cPath, pbuSrcPath, pbuDestPath, (nuint16) 0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/compath2.c,v 1.7 1994/09/26 17:44:40 rebekah Exp $
*/

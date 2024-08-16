/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:compath.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwncp.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwclocal.h"

/*manpage*_NWMakeComponentPath**********************************************
SYNTAX:  void N_API _NWMakeComponentPath
         (
            pnuint8 compCnt,
            pnuint8 compPath,
            pnstr8  path,
            nuint16 augmentFlag
         );

REMARKS:

ARGS: >  compCnt,
      >  compPath,
      >  path,
      >  augmentFlag

INCLUDE: nwundoc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: ?? Apr 1993 - written - jwoodbur
           constructs component paths doing augmentation correctly
         ?? May 1993 - modified - gcampbel, jwoodbur
           fixed augmentation
         14 Sep 1993 - modified to new standards (hungarian) - lbendixs
         14 Oct 1993 - now calls nwncp
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API _NWFillComponentPath
(
  pnuint8 pbuPacketPtr,
  pnstr8  pbstrDirPath
)
{
  _NWMakeComponentPath(pbuPacketPtr, &pbuPacketPtr[1], pbstrDirPath,
                       (nuint16) 0);
}

void N_API _NWMakeComponentPath
(
   pnuint8 pbuCompCnt,
   pnuint8 pbuCompPath,
   pnstr8  pbstrPath,
   nuint16 suAugmentFlag
)
{
nuint16 len;

   NWNCPMakeACompPath((nint)NWCStrLen(pbstrPath), pbstrPath,
                      0, pbuCompCnt, pbuCompPath, &len,
                      suAugmentFlag ? (nflag32)NCP_AUGMENT : (nflag32)0);

/*
pnuint8 pbuLenByte;
nuint8  buComponentLen = 0, buNumComponents = 0;
nuint8  buTempChar, buNewChar;

  suAugmentFlag = suAugmentFlag;

  pbuLenByte   = pbuCompPath++;
  if(pbstrPath != NULL)
  {
    while((buTempChar = (nuint8)*pbstrPath++) != '\0')
    {
      if(buTempChar == ':' || buTempChar == '\\' || buTempChar == '/')
      {
        if(!((buTempChar == '\\' || buTempChar == '/') && (*pbstrPath == '\\' || *pbstrPath == '/')))
        {
          if(buComponentLen)
          {
            buNumComponents++;
            *pbuLenByte = buComponentLen;
            pbuLenByte = pbuCompPath++;
            buComponentLen = 0;
          }
        }
        else
        {
          buComponentLen++;
          *pbuCompPath++= (nuint8) *pbstrPath++;
        }
      }
      else
      {
        buNewChar = buTempChar;
        switch(buTempChar)
        {
          case ('*'):
          case ('.'):
          case ('?'):
          case (0xff):
            if(suAugmentFlag)
            {
              buNewChar |= 0x80;
              *pbuCompPath++ = 0xff;
              buComponentLen++;
            }
            else if(buTempChar != '.')
            {
              *pbuCompPath++ = 0xff;
              buComponentLen++;
            }
            buTempChar = '\0';
            break;
        }

        if(buTempChar && NWCharType((int) buTempChar) == 2)
        {
          buComponentLen++;
          *pbuCompPath++ = buNewChar;
          buNewChar = *pbstrPath++;
        }

        *pbuCompPath++ = buNewChar;
        buComponentLen++;
      }
    }
  }
  if(buComponentLen)
  {
    *pbuLenByte = buComponentLen;
    buNumComponents++;
  }
  *pbuCompCnt = buNumComponents;
*/
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/compath.c,v 1.7 1994/09/26 17:44:39 rebekah Exp $
*/

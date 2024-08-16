/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:convaug.c	1.5"
#include "nwclient.h"
#include "ntypes.h"
#include "nwcaldef.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwclocal.h"
#include "nwlocale.h"

/*manpage*NWConvertAndAugment***********************************************
SYNTAX:  void N_API NWConvertAndAugment
         (
            nptr pString,
            nuint16 suFlag
         );

REMARKS: Searches a path for characters that could potentially conflict with
         augmented wild cards, then converts them to some predetermined
         values (predetermined by the server). The augmentFlag is checked and
         if equal to USE_DOS_WILD_MATCH, a search is made for wild card
         characters and any that are found are augmented.

ARGS: >  pString
      >  suFlag
         flag that will cause this function to augment wild cards if set to
         USE_NW_WILD_MATCH. Possible values of this flag are:

         USE_DOS_WILD_MATCH   =  0
         USE_NW_WILD_MATCH    =  1

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 15 Sep 1993 - NWNCP Enabled (hungarianized) - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API NWConvertAndAugment
(
   nptr     pString,
   nuint16  suFlag
)
{
   NWConvertToSpecChar(pString);

   if(suFlag == USE_DOS_WILD_MATCH)
   {
      while(*(pnuint8)pString != '\0')
      {
         switch(*(pnuint8)pString)
         {
         case('*'):
            *(pnuint8)pString |= 0x80;
            break;

         case('.'):
            *(pnuint8)pString |= 0x80;
            break;

         case('?'):
            *(pnuint8)pString |= 0x80;
            break;
         }
         pString = NWNextChar(pString);
      }
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/convaug.c,v 1.7 1994/09/26 17:44:45 rebekah Exp $
*/


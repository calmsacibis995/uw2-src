/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:prevchar.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/prevchar.c,v 1.4 1994/09/26 17:21:15 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==	file		: PrevChar.c
  ==
  ==	routine 	: NWPrevChar
  ==
  ==	author		: Allan Neill
  ==
  ==	date		: 2 July 1990
  ==
  ==	comments	: This function is Double-Byte sensitive
  ==
  ==    modifications   :
  ==
  ==	dependencies	:
  ==
  ========================================================================*/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

char N_FAR * N_API NWPrevChar(
   const char N_FAR *string,
   char N_FAR *position)
{
   int  charSize;
   char N_FAR *tmp;

   if (string == NULL || position < string)  /* something went wrong */
      return (char N_FAR *) NULL;

   if (position == string)
      return position;

   tmp = position;
   position--;

   if (_DBCSVector[0].lowValue == 0)  /* only single byte chars legitimate */
      return position;

   while (position > string)  /* go back to the front of the string or  */
   {                          /* until a single byte character is found */
      position--;
      if (NWCharType(*position) == NWSINGLE_BYTE)
      {
         position++;
         break;
      }
   }

   /*
    * Now go forward while we remain on a previous character until we
    * reach the immediately previous character position
    */
   while ((position + (charSize = NWCharType(*position))) < tmp)
      position += charSize;

   return position;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/prevchar.c,v 1.4 1994/09/26 17:21:15 rebekah Exp $
*/

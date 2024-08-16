/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:inc.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/inc.c,v 1.1 1994/09/26 17:20:36 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : INC.C
  ==
  ==	routine 	: NWIncrement
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 19 July 1989
  ==
  ==	comments	: Increment a string pointer by n characters,
  ==			  whether characters be single-byte of double-byte.
  ==
  ==    modifications   :
  ==
  ==	dependencies	: NWCharType
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

/*
    This function is only called if working with a double byte
    character set.
*/

char N_FAR * N_API NWIncrement(
   char N_FAR *string,
   size_t numChars)
{
   int  charSize;
   int  count;

   if (_DBCSVector[0].lowValue)
   {
      if (string)
      {
         for (count = 0; (numChars > 0) && (*string); numChars--)
         {
            string += (charSize = NWCharType(*string));
            count += charSize;
         }

         return (string);
      }
      else
      {
         return (char N_FAR *) NULL;
      }
   }
   else
   {
      for (count = 0; (numChars > 0) && (*string); numChars--)
         string++;

      return (string);
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/inc.c,v 1.1 1994/09/26 17:20:36 rebekah Exp $
*/

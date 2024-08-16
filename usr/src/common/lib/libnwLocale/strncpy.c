/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:strncpy.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strncpy.c,v 1.1 1994/09/26 17:21:26 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRNCPY.C
  ==
  ==	routine 	: NWLstrncpy
  ==
  ==	author		: Allan Neill
  ==
  ==	date		: 10 September 1990
  ==
  ==	comments	: Locale-sensitive version of ANSII standard
  ==			  lstrncpy
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

char N_FAR * N_API NWstrncpy(
   char N_FAR *target_string,
   char N_FAR *source_string,
   int numChars)
{
   char N_FAR *target = target_string;

   if (source_string && target_string)
   {
      if (_DBCSVector[0].lowValue)
      {
         for (; numChars && *source_string; numChars--)
         {
            if (NWCharType((unsigned char) *source_string) == NWSINGLE_BYTE)
            {
               *target++ = *source_string++;
            }
            else  /* NWDOUBLE_BYTE */
            {
               *target++ = *source_string++;
               *target++ = *source_string++;
            }
         }
      }
      else
      {
         for (; numChars && *source_string; numChars--)
            *target++ = *source_string++;
      }

      if (numChars)
         *target = '\0';
   }
   else
   {
      target_string = (char N_FAR *) NULL;
   }

   return target_string;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strncpy.c,v 1.1 1994/09/26 17:21:26 rebekah Exp $
*/

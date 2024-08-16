/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:strbcpy.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strbcpy.c,v 1.1 1994/09/26 17:21:20 rebekah Exp $"
/*========================================================================
  ==	library 	: NWLOCALE
  ==
  ==    file   : STRBCPY.C
  ==
  ==	routine 	: NWLstrbcpy
  ==
  ==	author	: Vance Campbell
  ==
  ==	date		: 18 feb. 1994
  ==
  ==	comments	: Locale-sensitive version of ANSI standard lstrncpy. This
  ==             function differs from NWstrncpy in that it copies a
  ==             specified number of BYTES instead of a specified number of
  ==             characters. This function also guarantees that the string
  ==             will be NULL terminated, so at most n-1 bytes will be copied.
  ==
  ==  modifications  :
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

char N_FAR * N_API NWLstrbcpy(
   char N_FAR *target_string,
   char N_FAR *source_string,
   int numBytes)
{
   char N_FAR *target = target_string;

   if (source_string && target_string)
   {
		numBytes--;		/* leave room for terminating NULL */
      if (_DBCSVector[0].lowValue)
      {
         for (; numBytes && *source_string; numBytes--)
         {
            if (NWCharType((unsigned char) *source_string) == NWSINGLE_BYTE)
            {
               *target++ = *source_string++;
            }
            else  /* NWDOUBLE_BYTE */
            {
               if (numBytes < 2)
                  break;

               *target++ = *source_string++;
               *target++ = *source_string++;
               numBytes--;
            }
         }
      }
      else
      {
         for (; numBytes && *source_string; numBytes--)
            *target++ = *source_string++;
      }

/*      if (numBytes) */
         *target = '\0';
   }
   else
   {
      target_string = (char N_FAR *) NULL;
   }

   return target_string;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strbcpy.c,v 1.1 1994/09/26 17:21:20 rebekah Exp $
*/

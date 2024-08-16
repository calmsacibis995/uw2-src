/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrcspn.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrcspn.c,v 1.1 1994/09/26 17:20:59 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRCSPN.C
  ==
  ==    routine         : NWLstrcspn
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 13 July 1989
  ==
  ==	comments	: Scans a string for the first segment that is not
  ==			  in a given set of characters
  ==
  ==			  This function is Double-Byte sensitive
  ==
  ==	modifications	: 17 September 1990 Create a non-enabled routine
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

#if (defined N_PLAT_NLM || defined WIN32 || N_PLAT_UNIX)
#define _fstrlen strlen
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

size_t N_API NWLstrcspn(
   const char N_FAR *string1,
   const char N_FAR *string2)
{
   char N_FAR *t1;
   char N_FAR *t2;
   char N_FAR *found;
   int  segment;

   /* If a valid string was passed */
   if (string1 != NULL)
   {
      /* If the search string is not null */
      if (string2 != NULL)
      {
         found = NULL;
         t1 = (char N_FAR *)string1;

         /* If single-byte system */
         if (L_MB_CUR_MAX == 1)
         {
            /* Loop thru string1 until match for char in string2 is found */
            while ((*t1 != '\0') && (found == NULL))
            {
               for (t2 = (char N_FAR *)string2;
               (*t2 != '\0') && (*t2 != *t1);
               t2++)
                  ;

               if (*t2 != (char) NULL)
                  found = t2;
               else
                  t1++;
            }

            segment = (int)(t1 - (char N_FAR *)string1);
         }
         else  /* System is multi-byte */
         {
            /* Loop thru string1 until match for char in string2 is found */
            while ((*t1 != '\0') && (found == NULL))
            {
               for (t2 = (char N_FAR *)string2;
               (*t2 != (char) NULL) && (_NWMBCompare(t1,t2) != 0);
               t2 = NWNextChar(t2))
                  ;

               if (*t2 != (char) NULL)
                  found = t2;
               else
                  t1 = NWNextChar(t1);
            }

            segment = (int)(t1 - (char N_FAR *)string1);
         }
      }
      else
      {
         segment = _fstrlen(string1);
      }
   }
   else
   {
	   segment = 0;
   }

   return segment;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrcspn.c,v 1.1 1994/09/26 17:20:59 rebekah Exp $
*/

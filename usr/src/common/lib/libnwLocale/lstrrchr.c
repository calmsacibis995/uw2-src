/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrrchr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrrchr.c,v 1.1 1994/09/26 17:21:04 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRRCHR.C
  ==
  ==    routine         : NWLstrrchr
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 27 July 1989
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

#if (defined N_PLAT_NLM || (defined N_PLAT_MSW && defined N_ARCH_32) || N_PLAT_UNIX)
#define _fstrlen strlen
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include	"enable.h"

char N_FAR * N_API NWLstrrchr(
   char N_FAR *string,
   int find)
{
   char N_FAR *last = (char N_FAR *) NULL;

   /* If there is a string to search in */
   if (string != NULL)
   {
      /* If this is a single-byte system, search from the end of the
      string and decrement by 1 */
      if (L_MB_CUR_MAX == 1)
      {
         /*
           Watch out for a pointer to the terminating '\0'
           JBI 3/8/93 added:
             if (*string)
         */
         if (*string)
         {
           last = string + _fstrlen(string) - 1;
           while (last && last >= string)
           {
              if (*last == (char)find)
                 break;
              else if (last == string)
                 last = NULL;
              else
                 last--;
           }
         }
      }
      /* If it's a double-byte system, search from the beginning of the
         string because it might be more efficient */
      else
      {
         for ( ; *string != (char) NULL; string = NWNextChar(string) )
            if (*string == (char) find)
               last = string;
      }

      return (last);
   }
   else
   {
      return ((char N_FAR *) NULL);
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrrchr.c,v 1.1 1994/09/26 17:21:04 rebekah Exp $
*/

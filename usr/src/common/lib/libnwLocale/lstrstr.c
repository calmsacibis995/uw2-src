/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrstr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrstr.c,v 1.1 1994/09/26 17:21:07 rebekah Exp $"
/*========================================================================
  ==	library			: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRSTR.C
  ==
  ==    routine         : NWLstrstr
  ==
  ==	author			: Phil Karren
  ==
  ==    date            : 27 July 1989
  ==
  ==	comments		:	This function is Double-Byte sensitive
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

char N_FAR * N_API NWLstrstr(
   char N_FAR *string1,
   char N_FAR *string2)
{
   char N_FAR *temp1;
   char N_FAR *temp2;
   char N_FAR *index;
   char N_FAR *offset;

   offset = NULL;  /* Return NULL unless match is found */

   if (string2 != NULL)
   {
      if (string1 != NULL)
      {
         temp1 = string1;
         temp2 = string2;

         /* If this is a SBCS system */
         if (L_MB_CUR_MAX == 1)
         {
            /* Parse through string1 until a char matches first char
               in string2 */
            while (*temp1 != (char) NULL)
            {
               for (index = temp1 ;
               *index == *temp2 && *index != (char) NULL ;
               index++, temp2++)
                  ;
               
               /* If there was a match, return the position */
               if (*temp2 == (char) NULL)
               {
                  offset = temp1;
                  break;
               }
               else        /* Otherwise, keep looking */
               {
                  if (*index != (char) NULL)
                     temp2 = string2;
               }

               temp1++;
            }
         }
         else    /* Multibyte, not single byte */
         {
            while (*temp1 != (char) NULL)
            {	/* Move ahead in string1 until the character in string1
                  matches the first character in string2 */

               while (_NWMBCompare(temp1,temp2) == 0
               && *temp1 != (char) NULL)
               {
                  /* If there is still something to search for */
                  if (*temp1 != (char) NULL)
                  {
                     for (index = temp1;
                     (_NWMBCompare(index,temp2) == 0) &&
                     (*index != (char) NULL);
                     index = NWNextChar(index), temp2 = NWNextChar(temp2))
                        ;

                     /* If there was a match, return the position */
                     if (*temp2 == (char) NULL)
                     {
                        offset = temp1;
                     }
                     else    /* Otherwise, keep looking */
                     {
                        if (*index != (char) NULL)
                        {
                           temp2 = string2;
                           temp1 = index;
                        }
                     }
                  }
                  else
                  {
                     if (NWNextChar(temp2) == (char) NULL)
                        offset = temp1;
                  }
               }

               temp1 = NWNextChar(temp1);
            }
         }       /* end Multibyte case */
      }
   }

   return (offset);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrstr.c,v 1.1 1994/09/26 17:21:07 rebekah Exp $
*/

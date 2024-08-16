/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrpbrk.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrpbrk.c,v 1.1 1994/09/26 17:21:02 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRPBRK.C
  ==
  ==    routine         : NWLstrpbrk
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 13 July 1989
  ==
  ==	comments	: Returns a pointer to the first ocurrence of
  ==			  any character in a given set.
  ==
  ==			  This function is Double-Byte sensitive.
  ==
  ==    modifications   :
  ==
  ==	dependencies	: NWMBCompare
  ==			  NWNextChar
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

char N_FAR * N_API NWLstrpbrk(
   char N_FAR *string1,
   const char N_FAR *string2)
{
   char N_FAR *returnString;
   char N_FAR *tempPtr = NULL;

   /* Return NULL if either string is NULL */
   if ((string1 == NULL) || (string2 == NULL))
   {
      returnString = NULL;
   }
   else
   {
      returnString = NULL;

      /* If single-byte environment, assume all characters take one byte */
      if (L_MB_CUR_MAX == 1)
      {
         for ( ; *string1 != (char) NULL; string1++)
         {
            for (tempPtr = (char N_FAR *)string2;
            *tempPtr != (char) NULL; tempPtr++)
            {
               if (*tempPtr == *string1)
               {
                  returnString = (char N_FAR *)string1;
                  break;
               }
            }

            if (returnString != NULL)
               break;
         }
      }				       /* Multi-byte environment */
      else
      {
         for ( ; *string1 != (char) NULL ; string1 = NWNextChar(string1))
         {
            for (tempPtr = (char N_FAR *)string2; *tempPtr != (char) NULL;
            tempPtr = NWNextChar(tempPtr))
            {
               if (_NWMBCompare(tempPtr,string1) == 0)
               {
                  returnString = (char N_FAR *)string1;
                  break;
               }
            }

            if (returnString != NULL)
               break;
         }
      }
   }

   return(returnString);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrpbrk.c,v 1.1 1994/09/26 17:21:02 rebekah Exp $
*/

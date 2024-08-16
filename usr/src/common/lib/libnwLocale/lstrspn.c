/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrspn.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrspn.c,v 1.1 1994/09/26 17:21:06 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRSPN.C
  ==
  ==    routine         : NWLstrspn
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 13 July 1989
  ==
  ==	comments	:	Returns a pointer to the first ocurrence of
  ==						any character in a given set.
  ==
  ==						This function is Double-Byte sensitive.
  ==
  ==    modifications   :
  ==
  ==	dependencies	:	NWMBCompare
  ==						NWNextChar
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

size_t N_API NWLstrspn(
   const char N_FAR *string1,
   const char N_FAR *string2)
{
   char N_FAR *tempPtr1;
   char N_FAR *tempPtr2;

   /* If either parameter is NULL, return */
   if ((string1 == NULL) || (string2 == NULL))
   {
      return 0;
   }
   else
   {
      if (L_MB_CUR_MAX == 1)      /* If single-byte system */
      {
         for (tempPtr1 = (char N_FAR *)string1; *tempPtr1 != (char) NULL;
         tempPtr1++)
         {
            for (tempPtr2 = (char N_FAR *)string2; *tempPtr2 != *tempPtr1;
            tempPtr2++)
               if (*tempPtr2 == '\0')
                  return((size_t)(tempPtr1 - (char N_FAR *)string1));
         }
      }
      else if (L_MB_CUR_MAX == 2) /* System is multi-byte */
      {
         for (tempPtr1 = (char N_FAR *)string1;
         *tempPtr1 != (char) NULL;
         tempPtr1 = NWNextChar(tempPtr1))
            for (tempPtr2 = (char N_FAR *)string2;
            _NWMBCompare(tempPtr1,tempPtr2) != 0;
            tempPtr2 = NWNextChar(tempPtr2))
               if (*tempPtr2 == (char) NULL)
                  return((size_t)(tempPtr1 - (char N_FAR *)string1));
      }
   }

   return((size_t)(tempPtr1 - (char N_FAR *)string1));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrspn.c,v 1.1 1994/09/26 17:21:06 rebekah Exp $
*/

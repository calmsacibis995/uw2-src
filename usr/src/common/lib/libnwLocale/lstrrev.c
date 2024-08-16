/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrrev.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrrev.c,v 1.1 1994/09/26 17:21:05 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==	file		: LSTRREV.C
  ==
  ==	routine 	: NWLstrrev
  ==
  ==	author		: Allan Neill
  ==
  ==	date		: 29 June 1990
  ==
  ==	comments	: Reverse the string passed as string2
  ==
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

#if (defined N_PLAT_NLM || (defined N_PLAT_MSW && defined N_ARCH_32) || defined N_PLAT_UNIX)
#define _fstrlen strlen
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

char N_FAR * N_API NWLstrrev(
   char N_FAR *string1,
   char N_FAR *string2)
{
   char N_FAR *temp;
   char N_FAR *temp1;
   char N_FAR *temp2;

   if (string2 != NULL)
   {
      if (string1 != NULL)
      {
         temp1 = string1 + _fstrlen(string2);
         temp2 = string2;
         *temp1 = (char) NULL;

         /* If this is a SBCS system */
         if (L_MB_CUR_MAX == 1)
         {
            while (*temp2 != (char) NULL)
               *(--temp1) = *temp2++;
         }
         else    /* Multibyte, not single byte */
         {
            while (*temp2 != (char) NULL)
            {
               temp = temp2;
               temp2 = NWNextChar(temp2);

               if(temp + 1 == temp2) /* single byte */
               {
                  *(--temp1) = *temp;
               }
               else		  /* double byte */
               {
                  *(--temp1) = *(temp + 1);
                  *(--temp1) = *temp;
               }
            }
         }	    /* end Multibyte case */
      }
      else
      {
         return((char N_FAR *) NULL);
      }
   }

   return(temp1);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrrev.c,v 1.1 1994/09/26 17:21:05 rebekah Exp $
*/

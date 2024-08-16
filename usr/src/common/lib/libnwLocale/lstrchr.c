/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrchr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrchr.c,v 1.1 1994/09/26 17:20:57 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRCHR.C
  ==
  ==    routine         : NWLstrchr
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 18 July 1989
  ==
  ==	comments	: Locale-sensitive version of ANSII standard
  ==			  strchr
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

char N_FAR * N_API NWLstrchr(
   char N_FAR *string,
   int find)
{
   if (string != NULL)
   {
	   /* If system is single-byte */
      if (L_MB_CUR_MAX == 1)
      {
         while ((*string != '\0') && (*string != (char)find))
            string++;
      }
      else
      {
         /* Search until character is found or end of string is reached. */
         while ((*string != (char)NULL) && (*string != (char)find))
         string = NWNextChar(string);
      }
		if ((*string == '\0') && (*string == (char)find))
			return(string);
		else
	      return (*string != '\0' ? string : (char N_FAR *) NULL); 
   }
   else
	   return ((char N_FAR *) NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrchr.c,v 1.1 1994/09/26 17:20:57 rebekah Exp $
*/

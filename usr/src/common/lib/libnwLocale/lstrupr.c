/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrupr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrupr.c,v 1.1 1994/09/26 17:21:08 rebekah Exp $"
/*========================================================================
  ==	  library 	      : NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRUPR.c
  ==
  ==    routine         : NWLstrupr
  ==
  == 	  author		      : Phil Karren
  ==
  ==    date            : 21 June 1989
  ==
  ==	  comments	      : Convert a string to upper case.
  ==
  ==    modifications   :
  ==
  ==	  dependencies	   :
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

#if defined(N_PLAT_OS2)
#   define INCL_BASE
#   define INCL_DOSNLS
#   include <os2.h>
extern   COUNTRYCODE country;
#endif

#if defined N_PLAT_MSW
# include <windows.h>
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

#if defined(N_PLAT_DOS)
extern unsigned char upperCaseTable[256];
#endif

char N_FAR * N_API NWLstrupr(
   char N_FAR *string )
{
   char N_FAR *stringBaseAddress;

   stringBaseAddress = string;

#if defined(N_PLAT_DOS)
   if (!upperCaseTable[1])
      _NWGetUpperCaseTable(upperCaseTable);

   while( *string != '\0' )
   {
      *string = (unsigned char) upperCaseTable[ (unsigned char) *string ];
      string = NWNextChar(string);
   }

#elif defined(N_PLAT_OS2)
   {
      USHORT rc;

      rc = DosCaseMap(_fstrlen(string), &country, string);
   }

#elif defined N_PLAT_MSW && defined N_ARCH_16
   AnsiUpper(string);

#elif defined N_PLAT_UNIX

	while( *string != '\0' )
   {
      *string = toupper((int) *string);
      string = NWNextChar(string);
   }
#endif

   return (stringBaseAddress);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrupr.c,v 1.1 1994/09/26 17:21:08 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:charupr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/charupr.c,v 1.1 1994/09/26 17:20:26 rebekah Exp $"
/*========================================================================
  ==	  library 	      : NetWare Directory Services Platform Library
  ==
  ==    file            : CHARUPR.C
  ==
  ==    routine         : NWCharUpr
  ==
  == 	  author		      : Phil Karren
  ==
  ==    date            : 21 June 1989
  ==
  ==	  comments	      : Convert a character to upper case.
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

#if defined (NWOS2)
#define INCL_BASE
#define INCL_DOSNLS
# include <os2.h>
extern COUNTRYCODE country;
#endif

#if defined N_PLAT_MSW
#include <windows.h>
#endif

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

extern unsigned char upperCaseTable[256];

int N_API NWCharUpr(
   int chr)
{
#if defined(NWDOS)
   if (NWCharType((unsigned char) chr) == NWDOUBLE_BYTE)
      return chr;

   if (!upperCaseTable[1])
      _NWGetUpperCaseTable(upperCaseTable);

   chr = upperCaseTable[(unsigned char) chr];

#elif defined(NWOS2)
   char    string[4];

   if(_DBCSVector[0].lowValue)
   {
      if (NWCharType((unsigned char) chr) == NWDOUBLE_BYTE)
      {
         int N_FAR *ptr = (int N_FAR *) string;
         *ptr = (int) *string;
         chr = DosCaseMap(2, &country, (char N_FAR *) string);
         chr = *ptr;
      }
      else
      {
         string[0] = (char) chr;
         string[1] = 0;
         chr = DosCaseMap(1, &country, (char N_FAR *) string);
         chr = (char) string[0];
      }
   }
   else
   {
      string[0] = (char) chr;
      string[1] = 0;
      chr = DosCaseMap(1, &country, (char N_FAR *) string);
      chr = (char) string[0];
   }

#elif	defined N_PLAT_MSW && defined N_ARCH_16
   char string[4] = {0, 0, 0, 0};
   int N_FAR *ptr;

   ptr = (int N_FAR *) string;
 
   if(_DBCSVector[0].lowValue)
   {
      if(NWCharType((unsigned char) chr) == NWDOUBLE_BYTE)
         *ptr = chr;
      else
         string[0] = (char) chr;
   }
   else
      string[0] = (char) chr;

	AnsiUpper(string);

   chr = *ptr;

#elif defined N_PLAT_UNIX || (defined N_PLAT_MSW && defined N_ARCH_32)

	if(chr <=256)
		chr=toupper(chr);

#endif

   return (chr);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/charupr.c,v 1.1 1994/09/26 17:20:26 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrxfrm.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrxfrm.c,v 1.1 1994/09/26 17:21:09 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRXFRM.C
  ==
  ==    routine         : NWLstrxfrm
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 21 June 1989
  ==
  ==	comments	: Convert string to collation value.  Converted
  ==			  String can be used by strcmp.
  ==
  ==			  This should be used when repeated comparisons
  ==			  of the same strings will be made.  It can
  ==			  result in considerable savings.
  ==
  ==    modifications   :
  ==
  ==	dependencies	: NWCompareWeight
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

#if ((defined N_PLAT_MSW && defined N_ARCH_32) || defined N_PLAT_UNIX || defined N_PLAT_NLM)
#define _fstrlen strlen
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

size_t N_API NWLstrxfrm(
   char N_FAR *string1,
   char N_FAR *string2,
   size_t numBytes)
{
   char N_FAR *temp;
   int xfrmlen = 0;

   if ((string2 != NULL) && (numBytes > 0))
   {
      if (localeInfo._collateTable[0] != '\0')
      {
         /* If system is single-byte */
         if (L_MB_CUR_MAX == 1)
         {
            for (temp = string1; *string2 && (numBytes > 1);
            temp++, string2++, numBytes--)
               *temp = localeInfo._collateTable[(unsigned char)*string2];

            *temp = '\0';
            xfrmlen = _fstrlen(string1);
         }
         else
         {	 /* double-byte chars are converted to a single-byte weight */
            for (temp = string1; *string2 && numBytes > 1;
            temp++, string2 = NWNextChar(string2), numBytes--)
               *temp = localeInfo._collateTable[*string2];

            *temp = '\0';
            xfrmlen = _fstrlen(string1);
         }
      }
   }

   return (xfrmlen);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrxfrm.c,v 1.1 1994/09/26 17:21:09 rebekah Exp $
*/

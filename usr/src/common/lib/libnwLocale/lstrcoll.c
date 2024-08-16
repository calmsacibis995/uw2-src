/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrcoll.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrcoll.c,v 1.1 1994/09/26 17:20:58 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LSTRCOLL.C
  ==
  ==    routine         : NWLstrcoll
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 21 June 1989
  ==
  ==	comments	: Compare strings based on collation weight instead
  ==			  of ascii value.  This sorts all local characters
  ==			  correctly, including double-byte characters.
  ==
  ==    modifications   :
  ==
  ==	dependencies	: NWCompareWeight
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

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

#if (defined WIN32 || defined N_PLAT_UNIX)
#define _fstrcmp strcmp
#endif

int N_API NWLstrcoll(
   char N_FAR *string1,
   char N_FAR *string2)
{
   /* Note: Only the first byte of double-byte characters is compared.
      This conforms to JIS requirements */
   int rcode;

   if (localeInfo._collateTable[0] != '\0')
   {
      if (L_MB_CUR_MAX == 1)		/* If single-byte system */
      {
         while (((rcode = _NWCompareWeight((unsigned char) *string1,
         (unsigned char) *string2)) == 0) && (*string1))
         {
            string1++;
            string2++;
         }
      }
      else
      {
         while (((rcode = _NWCompareWeight(*string1, *string2)) == 0)
         && (*string1))
         {
            string1 = NWNextChar(string1);
            string2 = NWNextChar(string2);
         }
      }
   }
   else   /* If there is no collate table, the locale is the C locale. */
   {
      rcode = _fstrcmp(string1, string2);
   }

   return (rcode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrcoll.c,v 1.1 1994/09/26 17:20:58 rebekah Exp $
*/

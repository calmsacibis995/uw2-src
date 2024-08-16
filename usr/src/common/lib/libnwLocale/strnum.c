/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:strnum.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strnum.c,v 1.1 1994/09/26 17:21:27 rebekah Exp $"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#if (defined N_PLAT_NLM || WIN32 || N_PLAT_UNIX)
#define _fstrcpy strcpy
#define _fstrlen strlen
#endif

#if (defined WIN32 || defined N_PLAT_UNIX)
#define _fmemcpy memcpy
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

extern LCONV __lconvInfo;

char N_FAR * N_API NWstrnum(
   char N_FAR *buffer,
   NUMBER_TYPE Value)
{
   int j;
   int len;
   char string[MAX_NUMBER];
   char number[MAX_NUMBER];
   char N_FAR *ptr;
   VALUE tmp;

   if (__lconvInfo.decimal_point[0] == 0)
      _Llocaleconv();

#if (defined N_PLAT_UNIX || defined WIN32)

	/* These platforms do not support the IEEE 80 bit floating point
		format, but rather the IEEE double precision 64 bit format.
		We need to convert to 80 bit before submitting to our 80 bit routine */

	_NWDoubleToExtended(&tmp, &Value);

#else
	_fmemcpy(&tmp, &Value, 10);
	
/*   tmp.exponent += -16383 + 1; */
   tmp.exponent += 0xc002;
#endif


   len = _mantissa_to_decimal(string, tmp);

   j = 0;
   ptr = number;

   while (j < len)
   {
      if (j && !(j % ((*__lconvInfo.grouping)-'0')))
      {
         _fstrcpy(ptr, __lconvInfo.thousands_sep);
         ptr += _fstrlen(__lconvInfo.thousands_sep);
      }

      *ptr++ = string[j] + (char) '0';
      j++;
   }

   if (tmp.sign)   /* negative number */
		{
			/* This appears to be acceptible everywhere except */
			/* South Africa, which brackets the number with */
			/* parentheses. */

         _fstrcpy(ptr, __lconvInfo.negative_sign);
         ptr += _fstrlen(__lconvInfo.negative_sign);

		}
   *ptr = '\0';
   NWLstrrev(buffer, number);

   return buffer;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strnum.c,v 1.1 1994/09/26 17:21:27 rebekah Exp $
*/

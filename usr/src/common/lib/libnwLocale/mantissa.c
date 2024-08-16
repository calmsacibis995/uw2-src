/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:mantissa.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/mantissa.c,v 1.1 1994/09/26 17:21:12 rebekah Exp $"
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
#define _fmemset memset
#endif


N_GLOBAL_LIBRARY(int) _mantissa_to_decimal(
   char N_FAR *buffer,
   VALUE tmp)
{
   int i;
   int j;
   int bit_length = 0;
   int len = 1;
/*   unsigned int save; */
	nuint16 save;
   char N_FAR *ptr;

   ptr = buffer;
   _fmemset(buffer, 0x00, MAX_NUMBER);

   i = 0;
   j = 7;
   save = tmp.mantissa[j];
   if (!save)
      return len;

   while (bit_length < (int) tmp.exponent)
   {
      while (i++ < 8 && bit_length < (int) tmp.exponent)
      {
         bit_length++;
         ptr = buffer;

         while (ptr - buffer < len)
#ifdef N_PLAT_UNIX
			{
				*ptr = ((char) *ptr << 1);
				ptr++;
			}
#else
            *ptr++ = (char) ((nint16) *ptr << 1);
#endif
         ptr = buffer;

         if (save & 0x0080)
            (*ptr)++;
         save = save << 1;

         while (ptr - buffer < len)
         {
            if (*ptr >= 10)
            {
               *(ptr+1) += *ptr / 10;
               *ptr %= 10;

               if ((ptr + 1) - buffer == len)
                  len++;
            }

            ptr++;
         }
      }

      i = 0;
      j--;
      save = tmp.mantissa[j];
   }

   return len;
}

/***************************************************************************/

/* Internal function to convert from 64 bit Double Precision to 80 bit
Extended Precision format. This allows existing 80 bit display code to be
used and provides a single code path for number formatting routines */

N_GLOBAL_LIBRARY(void) _NWDoubleToExtended(
   void *buffer,
   void *value)
{
	nuint8	*ptr;
	nuint8	*src;
	nuint8	temp;
	int		i;

/* First stuff the mantissa */

	_fmemset(buffer, 0x00, 10);	/* start out clean because we're going to	*/
											/*	be oring, shifting, and anding */

	ptr = (nuint8*) buffer;			/* point to low order byte of mantissa */
	ptr ++;					/* skip first byte since it's always zero */
	src = (nuint8*) value;
	for(i=0; i<6; i++)
		{
			temp = (nuint8) ((*src) << 3);	/* shift the low order 5 bits to */
				                              /* be the high order bits of the */
														/*	next byte */

			*ptr++ |= temp;		/* or the bottom 5 bits in */
			temp = (nuint8)((*src++) >> 5);	/* shift upper 3 bits down to
				                              /* the bottom */
			*ptr |= temp;		/* or lower 3 bits in */
		}

	/* last byte is special case because we only grab 4 bits and or in the */
	/*	implicit "1" */

	temp = (nuint8)((*src) << 3);
	temp |= 0x80;
	*ptr++ |= temp;

/* Now do the exponent */

	/* source pointer should already point at the byte where the exponent */
	/* begins. We need to normalize it before proceeding. */


	/*	Grab the upper 4 bits of this byte, (which is really the low	*/
	/*	order 4 bits of the exponent) and shove it into the lower 4 bits of */
	/*	the exponent. */

	temp = (nuint8)((*src++) >> 4);
	*ptr |= temp;

	/* The next 4 bits of the exponent are in the low order 4 bits of the */
	/*	next byte, but go in the upper 4 bits of the low order byte of the */
	/*	target exponent */

	temp = (nuint8)((*src) << 4);
	*ptr++ |= temp;

	/* The last 3 bits of the exponent are in bits 4, 5, and 6 of the source
		byte. They go in the low order 3 bits of the next target byte. */

	temp = (nuint8)((*src) >> 4);
	temp &= 0x7;		/* mask off sign bit */
	*ptr |= temp;

	/* Last, but not least, do the sign bit */

	temp = (nuint8)((*src) & 0x80);
	*ptr |= temp;

	/* Now compensate for the shift in binary point and the "implicit 1" bit */
	/* by adding 2 to the exponent */

	ptr--;
	(*((nuint16*) ptr)) +=2;

	if (*((nuint16 *) ptr) == 2)
	{
		/* special case for input of 0 */
		*((nuint16*) ptr) = 0;

	} else {
		++ptr;
		*ptr ^= 0x4;
	}
}

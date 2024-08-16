/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:ltoa.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/ltoa.c,v 1.1 1994/09/26 17:21:10 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Library name:  NWLOCALE.LIB                                              *
 *	                                                                         *
 * Filename:      LTOA.C                                                    *
 *                                                                          *
 * Date Created:  November 1992                                             *
 *                                                                          *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.          *
 *                                                                          *
 * No part of this file may be duplicated, revised, translated, localized   *
 * or modified in any manner or compiled, linked or uploaded or downloaded	 *
 * to or from any computer system without the prior written consent of      *
 * Novell, Inc.                                                             *
 *                                                                          *
 ****************************************************************************/

#define NWL_EXCLUDE_FILE 1
#define NWL_EXCLUDE_TIME 1

#include "ntypes.h"
#include "nwlocale.h"
#include "locifunc.h"


char N_FAR * N_API NWultoa(
   unsigned long value,
   char N_FAR *buffer,
   unsigned int radix)
{
   char N_FAR *p = buffer;
   int i;
   char buf[34];

   if (buffer == 0)
      return buffer;
   if (radix < 2 || radix > 36)
   {
      buffer[0] = '\0';
      return buffer;
   }
   buf[0] = '\0';
   i = 1;

   do
   {
      buf[i] = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)(value % radix)];
      i++;
      value /= radix;
   } while (value != 0);

   while (*p++ = buf[--i])
      ;

   return buffer;
}


char N_FAR * N_API NWltoa(
   long value,
   char N_FAR *buffer,
   unsigned int radix)
{
   char N_FAR *p = buffer;

   if (radix == 10)
      if (value < 0)
      {
         *p++ = '-';
         value = - value;
      }

   NWultoa(value, p, radix);

   return buffer;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/ltoa.c,v 1.1 1994/09/26 17:21:10 rebekah Exp $
*/

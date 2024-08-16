/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:atoi.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/atoi.c,v 1.1 1994/09/26 17:20:24 rebekah Exp $"
/*****************************************************************************
* BEGIN_MANUAL_ENTRY ( NWatoi )
*
* Supported in (marked with X):
* 2.11  2.2   3.0   3.10  3.11  3.2
*  X		 X		 X		 X		 X		 X
*
* SOURCE MODULE  : ATOI.C
*
* API NAME       : NWatoi
*
* INCLUDE        : NWLOCALE.H
*
* SYNTAX         : int N_API NWatoi( char N_FAR *string );
*
* PARAMETERS     : string -> the string to be converted
*
* RETURN         : the converted string; 0 if the string cannot be converted
*
* DESCRIPTION    : converts a character string to an interger value
*
* MODIFICATIONS  :
*
* END_MANUAL_ENTRY
*****************************************************************************/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include <stdlib.h>
#include "ntypes.h"
/* #include "nwlocale.h"
#include "enable.h"
*/
int N_API NWatoi(
   char N_FAR *string )
{
   int ival;
   int negative = 0;

   while (*string == ' ' || *string == '\t' || *string == '\n'
   || *string == '\f' || *string == '\r' || *string == '\v')
      string++;

   if (*string == '-')
   {
      negative = 1;
      string++;
   }
   else if (*string == '+')
   {
      string++;
   }

   for (ival = 0; *string >= '0' && *string <= '9'; string++)
      ival = (ival * 10) + (*string - '0');

   if (negative)
      ival *= -1;

   return ival;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/atoi.c,v 1.1 1994/09/26 17:20:24 rebekah Exp $
*/

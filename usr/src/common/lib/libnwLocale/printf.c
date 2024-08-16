/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:printf.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/printf.c,v 1.1 1994/09/26 17:21:16 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Filename:      printf.c                                                  *
 *                                                                          *
 * Date Created:  Feb. 12, 1992                                             *
 *                                                                          *
 * Version:       1.00                                                      *
 *                                                                          *
 * Programmers:   Logan Wright                                              *
 *                                                                          *
 ****************************************************************************/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

# include <stdarg.h>
# include <stdio.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

/*****************************************************************************
* BEGIN_MANUAL_ENTRY ( NWprintf )
*
* Supported in (marked with X):
* 2.11  2.2   3.0   3.10  3.11  3.2
*  X     X     X     X     X     X
*
* SOURCE MODULE: PRINTF.C
*
* API NAME     : NWprintf
*
* DESCRIPTION  : Functions exactly like the standard C library function
*                with the added functionality of being enabled for
*                double-byte characters and also allowing for reordering
*                of the parameters as required for translation of the
*                message
*
* INCLUDE      : NWLOCALE.H
*
* SYNTAX       : int far cdecl NWprintf(char N_FAR *format, ...);
*
* PARAMETERS   :   -> input          <-output
*
*    -> format
*       - a standard printf control string with the possible
*         addition of reordering information
*
*    -> ...
*       - a variable number of parameters typical of printf usage
*
* RETURN       : the number of bytes output
*
* MODIFICATIONS: Replaced call to standard vprintf with our own enabled
*                version. 94-4-15  -VC
*
* END_MANUAL_ENTRY
*****************************************************************************/

#if (defined N_PLAT_UNIX || defined WIN32)
int NWprintf(
#else
int far cdecl NWprintf(
#endif

   char N_FAR *format,     /* Format control string               */
   ...)                    /* Other parameters follow             */

{

   va_list   parameters;   /* Pointer to parameters on the stack  */

	char newFormat[600];
	/*
		Get a pointer to the parameters
	*/
	va_start(parameters, format);

	/*
		If there is reordering to be done, do it now
	*/
   ReorderPrintfParameters(&format, newFormat, (unsigned int N_FAR *) parameters);

	/*
		Now call NWvprintf, with whatever parameters were appropriate
	*/
	return (NWvprintf(format, parameters));
}

/****************************************************************************/
/****************************************************************************/

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/printf.c,v 1.1 1994/09/26 17:21:16 rebekah Exp $
*/

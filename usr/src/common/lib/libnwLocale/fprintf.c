/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:fprintf.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/fprintf.c,v 1.1 1994/09/26 17:20:28 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Filename:      fprintf.c                                                  *
 *                                                                          *
 * Date Created:  Feb. 18, 1992                                             *
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
#define NWL_EXCLUDE_TIME 1

#include <stdio.h>
#include <stdarg.h>
#include "ntypes.h"
#include "nwlocale.h"

/*****************************************************************************
* BEGIN_MANUAL_ENTRY ( NWfprintf )
*
* Supported in (marked with X):
* 2.11  2.2   3.0   3.10  3.11  3.2
*  X     X     X     X     X     X
*
* SOURCE MODULE: FPRINTF.C
*
* API NAME     : NWfprintf
*
* DESCRIPTION  : Functions exactly like the standard C library function
*                with the added functionality of being enabled for
*                double-byte characters and also allowing for reordering
*                of the parameters as required for translation of the
*                message
*
* INCLUDE      : NWLOCALE.H
*
* SYNTAX       : int far cdecl NWfprintf(FILE N_FAR *stream,
*                                        char N_FAR *format, ...);
*
* PARAMETERS   :   -> input          <-output
*
*    -> stream
*       - the output stream to receive the message
*
*    -> format
*       - a standard fprintf control string with the possible
*         addition of reordering information
*
*    -> ...
*       - a variable number of parameters typical of fprintf usage
*
* RETURN       : the number of bytes output
*
* MODIFICATIONS:
*
* END_MANUAL_ENTRY
*****************************************************************************/

/* Kludge around this warning until we see if it matters... */
#ifdef _MSC_VER
# if (_MSC_VER > 600)
#  pragma warning(disable:4758)
# endif
#endif

void ReorderPrintfParameters(
        char N_FAR * N_FAR *format,
		char *newFormat,
        unsigned int N_FAR *parms);

#if (defined N_PLAT_UNIX || defined WIN32)
int	NWfprintf(
#else
int far cdecl NWfprintf(
#endif

   FILE N_FAR *stream,     /* Output stream                       */
   char N_FAR *format,     /* Format control string               */
   ...)                    /* Other parameters follow             */

{
   va_list   parameters;   /* Pointer to parameters on the stack  */
	int	count;

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
		Now call NWvfprintf, with whatever parameters were appropriate
	*/
	count = NWvfprintf(stream, format, parameters);
	va_end (parameters);
	return (count);
}

/****************************************************************************/
/****************************************************************************/

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/fprintf.c,v 1.1 1994/09/26 17:20:28 rebekah Exp $
*/

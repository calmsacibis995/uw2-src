/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:vfprintf.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/vfprintf.c,v 1.1 1994/09/26 17:21:54 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Filename:      vfprintf.c                                                  *
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
#define NWL_EXCLUDE_FILE 1

#ifdef N_PLAT_OS2
 #ifndef _DLL
  #define _DLL
 #endif
 #ifndef _MT
  #define _MT
 #endif
#include <os2.h>
#endif


# include <stdarg.h>
# include <stdio.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

/*****************************************************************************
* BEGIN_MANUAL_ENTRY ( NWvprintf )
*
* Supported in (marked with X):
* 2.11  2.2   3.0   3.10  3.11  3.2
*  X     X     X     X     X     X
*
* SOURCE MODULE: VFPRINTF.C
*
* API NAME     : NWvfprintf
*
* DESCRIPTION  : Functions exactly like the standard C library function
*                with the added functionality of being enabled for
*                double-byte characters and also allowing for reordering
*                of the parameters as required for translation of the
*                message
*
* INCLUDE      : NWLOCALE.H
*
* SYNTAX       : int N_API NWvfprintf(FILE N_FAR *stream,
*                                            char N_FAR *format,
*                                            char N_FAR *arglist);
*
* PARAMETERS   :   -> input          <-output
*
*    -> stream
*       - the output stream that receives the message
*
*    -> format
*       - a standard vfprintf control string with the possible
*         addition of reordering information
*
*    -> arglist
*       - a pointer to an array of a variable number of parameters typical
*         of vfprintf usage
*
* RETURN       : the number of bytes output
*
* MODIFICATIONS:
*
* END_MANUAL_ENTRY
*****************************************************************************/

/* internal declarations */

#define	PERCENT				37
#define	NEWLINE_CHAR		10
#define	RETURN_CHAR			13

static void N_FAR fileOutput(char outputChar,
	nuint8 N_FAR * N_FAR *stream,
	int N_FAR *count);

/****************************************************************************/

int N_API NWvfprintf(
	   FILE N_FAR *stream,   /* Output stream                       */
		char N_FAR *controlString,
		va_list args)
{
	nuint8 c;
	int count;

	char newFormat[600];
	/* Since this is an exported API entry point as well as an internal */
	/* entry point for NWprintf, we need to check parameter order both */
	/* places. That's OK because ReorderPrintfPararmeters can handle the */
	/* case of already reordered parameters */

   ReorderPrintfParameters(&controlString, newFormat, (unsigned int N_FAR *) args);

	/* Perform the actual output */
	count = 0;
	while((c = *controlString++) != 0)
	{
		/* Check for format (%) string. */

		if (c == PERCENT)
		{
			controlString += PercentFormat((nuint8 N_FAR *)controlString, &args, 
					fileOutput, (nuint8 N_FAR * N_FAR *) &stream, &count);
			continue;
		}

		fileOutput(c, (nuint8 N_FAR * N_FAR *) &stream, &count);
	}

	return (count);
}

/****************************************************************************/

static void N_FAR fileOutput(char outputChar,
	nuint8 N_FAR * N_FAR * stream, int N_FAR *count)
{
/* Kludge aound link problem with MSC 6 */

#if (defined N_PLAT_OS2 && defined _MSC_VER)
	HFILE h;
	USHORT u;
	FILE N_FAR *s;

	s = (FILE N_FAR *) *stream;

	h = s->_file;
	DosWrite(h, &outputChar, 1, &u);

	/* If we get a new line do a return also, cause  OS/2 acts like DOS */
	/* in this instance */

	if (outputChar == NEWLINE_CHAR)
	{
		outputChar = RETURN_CHAR;
		DosWrite(h, &outputChar, 1, &u);
		(*count)++;
	}
#else

	putc((int) outputChar, (FILE N_FAR *) *stream);

#endif
	(*count)++;
}

/****************************************************************************/


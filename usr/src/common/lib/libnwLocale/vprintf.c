/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:vprintf.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/vprintf.c,v 1.1 1994/09/26 17:21:55 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Filename:      vprintf.c                                                 *
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
* SOURCE MODULE: VPRINTF.C
*
* API NAME     : NWvprintf
*
* DESCRIPTION  : Functions exactly like the standard C library function
*                with the added functionality of being enabled for
*                double-byte characters and also allowing for reordering
*                of the parameters as required for translation of the
*                message
*
* INCLUDE      : NWLOCALE.H
*
* SYNTAX       : int N_API NWvprintf(char N_FAR *format,
*                                    char N_FAR *arglist);
*
* PARAMETERS   :   -> input          <-output
*
*    -> format
*       - a standard vprintf control string with the possible
*         addition of reordering information
*
*    -> arglist
*       - a pointer to an array of a variable number of parameters typical
*         of vprintf usage
*
* RETURN       : the number of bytes output
*
* MODIFICATIONS: Replaced call to standard vsprintf with our own enabled
*                version 94-4-15 -VC.
*
* END_MANUAL_ENTRY
*****************************************************************************/

/* internal declarations */

#define	PERCENT				37
static void N_FAR ScreenOutput(char outputChar,
	nuint8 N_FAR * N_FAR * string,
	int N_FAR *count);

/****************************************************************************/

int N_API NWvprintf(
		char N_FAR *controlString,
#ifdef N_PLAT_UNIX
		void *args)
#else
		va_list args)
#endif
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
					ScreenOutput, (nuint8 N_FAR * N_FAR *) &args,
					&count);
			continue;
		}

		ScreenOutput(c, (nuint8 N_FAR * N_FAR *) NULL, (int N_FAR *) &count); 
	}

/* The following chunk of code was in the original NLM source I derived */
/* this from. I will leave it in for now, in case we need it when we port */
/* the whole library to NLM.  94 April 15 -VC */

/* 	screenID->outputCursorPosition = screenInfo.outputLine * 2 *
			screenID->screenWidth + screenInfo.outputColumn * 2;

	EndScreenUpdateGroup(screenID);
*/
	return (count);
}

/****************************************************************************/

static void N_FAR ScreenOutput(char outputChar,
	nuint8 N_FAR * N_FAR * stream, int N_FAR *count)
{
	putchar(outputChar);
	N_UNUSED_VAR (stream);
	(*count)++;
}

/****************************************************************************/
/****************************************************************************/

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/vprintf.c,v 1.1 1994/09/26 17:21:55 rebekah Exp $
*/

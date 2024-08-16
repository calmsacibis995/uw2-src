/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unicat.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicat.c,v 1.1 1994/09/26 17:21:28 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNICAT.C												*
 *																			*
 * Date Created:	October 8, 1991											*
 *																			*
 * Version:			1.00													*
 *																			*
 * Programmers:		Lloyd Honomichl											*
 *																			*
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.			*
 *																			*
 * No part of this file may be duplicated, revised, translated, localized	*
 * or modified in any manner or compiled, linked or uploaded or downloaded	*
 * to or from any computer system without the prior written consent of 		*
 * Novell, Inc.																*
 *																			*
 ****************************************************************************/

/*
	Unicode version of ANSI strcat function

	Example:
		
		unicat(s1, s2);

	Description:

		Appends the unicode string pointed to by s2 (including the
		terminating unicode null character) to the string in the array
		pointed to by s1.  The terminating zero of s1 is replaced by the
		first character of s2.  The result is null terminated.

	Returns:
		
		Returns the value of s1.
*/

/****************************************************************************/

#ifdef WIN32
#define  NOATOM                  // Atom Manager routines
#define  NOCLIPBOARD             // Clipboard routines
#define  NOCOMM                  // to eliminate lint warnings
#define  NODEFERWINDOWPOS        // DeferWindowPos routines
#define  NOGDICAPMASKS           // CC_*, LC_*, PC_*, CP_*, etc
#define  NOKANJI                 // Kanji support stuff
#define  NOMETAFILE              // typedef METAFILEPICT
#define  NOMINMAX                // NO min(a,b) or max(a,b)
#define  NOOPENFILE              // OpenFile(), OemToAnsi(), AnsiToOem()
#define  NOPROFILER              // Profiler interface
#define  NOSOUND                 // Sound Driver routines
#undef   OEMRESOURCE             // OEM Resource values
#undef   NOLSTRING               // using lstrlen()
#include <windows.h>
#include "ntypes.h"
#include "unicode.h"

unicode N_FAR * N_API unicat
(
	unicode N_FAR *s1,			/* String to be appended to					*/
	unicode N_FAR *s2				/* String to be appended					*/
)
{
   return( wcscat( s1, s2 ) );
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

unicode N_FAR * N_API unicat
(
	unicode N_FAR *s1,			/* String to be appended to					*/
	unicode N_FAR *s2				/* String to be appended					*/
)
{

	int		 i;					/* Loop variable							*/

	/* Find the end of s1 */
	for ( i = 0 ; s1[i] ; i++ )
		;

	/* Append the second string */
	do
	{
		s1[i++] = *s2;
	} while (*s2++);

	/* Return pointer to s1 */
	return s1;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicat.c,v 1.1 1994/09/26 17:21:28 rebekah Exp $
*/

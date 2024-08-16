/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unistr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unistr.c,v 1.1 1994/09/26 17:21:51 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNISTR.C												*
 *																			*
 * Date Created:	October 15, 1991										*
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
	Unicode version of ANSI strstr function

	Example:
		
		p = unistr(s1, s2);

	Description:

		The unistr function locates the first occurance in the string
		pointed to by s1 of the sequence of characters (excluding the
		terminating null character) in the string pointed to by s2.

	Returns:
		
		The unistr function returns a pointer to the located string, or
		a null pointer if the string is not found.  If s2 points to a string
		with zero length, the function returns s1.
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

unicode N_FAR * N_API unistr
(
	unicode N_FAR *s1,			/* String to be searched					*/
	unicode N_FAR *s2				/* String to be found						*/
)
{
	return(wcsstr(s1, s2));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

/****************************************************************************/

unicode N_FAR * N_API unistr
(
	unicode N_FAR *s1,			/* String to be searched					*/
	unicode N_FAR *s2				/* String to be found						*/
)
{
	int		 i;					/* Loop variable							*/

	/*
		Check at each position in s1
	*/
	for ( ; *s1 ; s1++ )
	{
		/*
			Does s2 occur here?
		*/
		for ( i = 0 ; s2[i] ; i++ )
			if (s1[i] != s2[i])
				break;

		if (s2[i] == 0)
			return (s1);
	}

	/*
		Never found s2 in s1	
	*/
	return NULL;
}

#endif /* Macintosh */
#endif /* All but WIN 32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unistr.c,v 1.1 1994/09/26 17:21:51 rebekah Exp $
*/

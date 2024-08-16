/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unincat.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unincat.c,v 1.1 1994/09/26 17:21:39 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNINCAT.C												*
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
	Unicode version of ANSI strncat function

	Example:

		unincat(s1, s2);

	Description:

		The unincat function appends not more the n unicode characters (a
		null character and characters that follow it are not appended) from
		the array pointed to by s2 to the end of the string pointed to by s1.
		The initial character of s2 overwrites the null character at the end
		of s1.  A terminating null character is always appended to the result.

	Returns:
		
		Returns the a pointer to s1.
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

unicode N_FAR * N_API unincat(

	unicode N_FAR *s1, 			/* String to be appended to					*/
	unicode N_FAR *s2, 			/* String to be appended					*/
	size_t	n)						/* Maximum characters to append				*/
{
   return( wcsncat( s1, s2, n ) );
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

unicode N_FAR * N_API unincat(

	unicode N_FAR *s1, 			/* String to be appended to					*/
	unicode N_FAR *s2, 			/* String to be appended					*/
	size_t	n)						/* Maximum characters to append				*/

{

	int	i,					      /* Loop variable							*/
			j;					      /* Loop variable							*/

	/*
		Find the end of s1
	*/
	for ( i = 0 ; s1[i] ; i++ );

	/*
		Append stuff from s2 until we find a null, or reach the length limit
	*/
	for ( j = 0 ; s2[j] && j < (int)n ; i++, j++ )
		s1[i] = s2[j];

	/*
		Null terminate the result
	*/
	s1[i] = 0;

	/*
		Return pointer to s1
	*/
	return s1;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unincat.c,v 1.1 1994/09/26 17:21:39 rebekah Exp $
*/

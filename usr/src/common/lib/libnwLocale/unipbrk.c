/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unipbrk.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unipbrk.c,v 1.1 1994/09/26 17:21:43 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNIPBRK.C												*
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
	Unicode version of ANSI strpbrk function

	Example:

		p = unipbrk(s1, s2);

	Description:

		The unipbrk function locates the first occurance in the string
		pointed to by s1 of any unicode character from the string pointed
		to by s2.

	Returns:
		
		The unipbrk returns a pointer to the unicode character, or a null
		pointer if no character from s2 occurs in s1.
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

unicode N_FAR * N_API unipbrk(

	unicode N_FAR *s1, 			/* String to be searched					*/
	unicode N_FAR *s2) 			/* Unicode character set to be located		*/
{
   return( wcspbrk( s1, s2 ));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

/****************************************************************************/

unicode N_FAR * N_API unipbrk(

	unicode N_FAR *s1, 			/* String to be searched					*/
	unicode N_FAR *s2) 			/* Unicode character set to be located		*/

{

	int		 i;					/* Loop variable							*/

	/*
		See if we can find a character in string that occurs in set
	*/
	for ( ; *s1 ; s1++ )
		for ( i = 0 ; s2[i] ; i++ )
			if ( s2[i] == *s1 )
				return (s1);

	/*
		Didn't find any characters from s2 in s1
	*/
	return NULL;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unipbrk.c,v 1.1 1994/09/26 17:21:43 rebekah Exp $
*/

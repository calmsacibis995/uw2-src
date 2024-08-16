/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unirchr.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unirchr.c,v 1.1 1994/09/26 17:21:45 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNIRCHR.C												*
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
	Unicode version of ANSI strrchr function

	Example:

		p = unirchr(s, c);

	Description:

	  	The unirchr function locates the last occurance of the unicode
		character c in the string pointed to by s.  The terminating null
		character is considered to be part of the string.

	Returns:
		
		Returns a pointer to the located character, or a null pointer if the
		character does not occur in the string.
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

unicode N_FAR * N_API unirchr
(
	unicode N_FAR *s,				/* String to be searched					*/
	unicode	c						/* Unicode character to be located			*/
)
{
	return(wcsrchr(s, c));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

unicode N_FAR * N_API unirchr
(
	unicode N_FAR *s,				/* String to be searched					*/
	unicode		c						/* Unicode character to be located			*/
)
{

	unicode N_FAR *loc = NULL;	/* Location where we found c				*/

	/*
		See if we can find the character
	*/
	do
	{
		if (*s == c)
			loc = s;
	} while (*s++);

	/*
		Return pointer to the character, or null if we didn't find it
	*/
	return loc;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unirchr.c,v 1.1 1994/09/26 17:21:45 rebekah Exp $
*/

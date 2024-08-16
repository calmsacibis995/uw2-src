/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unispn.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unispn.c,v 1.1 1994/09/26 17:21:50 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNISPN.C												*
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
	Unicode version of ANSI strspn function

	Example:
		
		i = unispn(s1, s2);

	Description:

		The unispn function computes the length of the maximum initial
		segment of the string pointed to by s1 which consists entirely of
		unicode characters from the string pointed to by s2.

	Returns:
		
		The unispn function returns the length of the segment.
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

size_t N_API unispn
(
	unicode N_FAR *search,	  	/* String to be searched					*/
	unicode N_FAR *set		  	/* Unicode character we are looking for		*/
)
{
	return(wcsspn(search, set));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

size_t N_API unispn
(
	unicode N_FAR *search,	  	/* String to be searched					*/
	unicode N_FAR *set		  	/* Unicode character we are looking for		*/
)
{
	int i,					      /* Loop variable							*/
		 j;					      /* Loop variable							*/

	/*
		Scan the characters in search string
	*/
	for ( i = 0 ; search[i] ; i++ )
	{
		/*
			See if this character occurs in the set
		*/
		for ( j = 0 ; set[j] ; j++ )
			if ( set[j] == search[i] )
				break;

		/*
			If we can't find this character in the set, we're done
		*/
		if (set[j] == 0)
			break;
	}

	/*
		Return the length of the segment
	*/
	return i;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unispn.c,v 1.1 1994/09/26 17:21:50 rebekah Exp $
*/

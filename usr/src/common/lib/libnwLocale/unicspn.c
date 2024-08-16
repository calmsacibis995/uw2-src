/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unicspn.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicspn.c,v 1.1 1994/09/26 17:21:33 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNICSPN.C												*
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
	Unicode version of ANSI strcspn function

	Example:

		i = unicspn(s1, s2);

	Description:

		The unicspn function computes the length of the maximum initial
		segment of the string pointed to by s1 which consists entirely of
		unicode characters NOT found in the string pointed to by s2.

	Returns:
		
		The unicspn function returns the length of the segment.
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

size_t N_API unicspn(

	unicode N_FAR *string,		/* String to be scanned						*/
	unicode N_FAR *set)			/* Character set to search for				*/
{
	return(wcscspn(string, set));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

/****************************************************************************/

size_t N_API unicspn(

	unicode N_FAR *string,		/* String to be scanned						*/
	unicode N_FAR *set)			/* Character set to search for				*/

{

	int 	 i,					   /* Loop variable							*/
			 j;					   /* Loop variable							*/

	/*
		See if we can find any character from set in string
	*/
	for ( i = 0 ; string[i] ; i++ )
		for ( j = 0 ; set[j] ; j++ )
			if (string[i] == set[j])
				goto Done;

Done:
	return (i);
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicspn.c,v 1.1 1994/09/26 17:21:33 rebekah Exp $
*/

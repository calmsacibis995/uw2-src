/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unincpy.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unincpy.c,v 1.1 1994/09/26 17:21:40 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNINCPY.C												*
 *																			*
 * Date Created:	October 7, 1991											*
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
	Unicode version of ANSI strncpy function

	Example:

		unincpy(s1, s2, n);

	Description:

		Copies not more than n unicode characters (characters that follow
		a null character are not copied) from the array pointed to by s2 to
		the array pointed to by s1.  The result will not be null terminated
		is the length limit is reached before the terminating null of s2
		is found.

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

unicode N_FAR * N_API unincpy(

	unicode N_FAR *s1,			/* Buffer characters are copied to			*/
	unicode N_FAR *s2,			/* Buffer characters are copied from		*/
	size_t	n)					   /* Maximum number of characters to copy		*/
{
   return( wcsncpy( s1, s2, n ) );
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

unicode N_FAR * N_API unincpy(

	unicode N_FAR *s1,			/* Buffer characters are copied to			*/
	unicode N_FAR *s2,			/* Buffer characters are copied from		*/
	size_t	n)					   /* Maximum number of characters to copy		*/

{

	int i;					/* Loop variable							*/

	/*
		Copy until the null, or until the length limit
	*/
	for ( i = 0 ; s2[i] && i < (int)n ; i++ )
		s1[i] = s2[i];

	/*
		Copy the null, if there is room for it
	*/
	if (i < (int)n)
		s1[i] = s2[i];

	/*
		Return pointer to s1
	*/
	return s1;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unincpy.c,v 1.1 1994/09/26 17:21:40 rebekah Exp $
*/

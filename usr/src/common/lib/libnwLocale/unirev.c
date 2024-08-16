/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unirev.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unirev.c,v 1.1 1994/09/26 17:21:47 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNIREV.C												*
 *																			*
 * Date Created:	October 14, 1991										*
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
	Unicode version of strrev function

	Example:
		
		unirev(*s);

	Description:

	  	The unirev function reverses the string pointed to by s.

	Returns:
		
		Returns a pointer to the reversed string.
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

unicode N_FAR * N_API unirev
(
	unicode N_FAR *s		/* String to be reversed					*/
)
{
	return(_wcsrev(s));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

unicode N_FAR * N_API unirev
(
	unicode N_FAR *s		/* String to be reversed					*/
)
{

	int len,					/* Length of the string						*/
		 i;					/* Loop variable							*/
	unicode	 c;			/* Unicode character						*/

	/*
		Find out how long the string is
	*/
	for ( len = 0 ; s[len] ; len++ );

	/*
		Reverse the string
	*/
	for ( i = 0 ; i < len / 2 ; i++ )
	{
		c = s[i];
		s[i] = s[len - i - 1];
		s[len - i - 1] = c;
	}

	/*
		Return a pointer to the string
	*/
	return s;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unirev.c,v 1.1 1994/09/26 17:21:47 rebekah Exp $
*/

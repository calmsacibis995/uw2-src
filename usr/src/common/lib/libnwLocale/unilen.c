/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unilen.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unilen.c,v 1.1 1994/09/26 17:21:35 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNILEN.C												*
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
	Unicode version of ANSI strlen function

	Example:
		
		l = unilen(s);

	Description:

		Calculates the length (in unicode characters) of the unicode string
		pointed to by s.

	Returns:
		
		Returns the length of the unicode string.
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

size_t N_API unilen(

	unicode N_FAR *s)				/* String whose length should be calculated	*/
{
   return( wcslen( s ) );
}

size_t N_API unisize(

   unicode N_FAR *s)
{
   return( (unilen(s) + 1 ) * sizeof( *s ) );
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

size_t N_API unilen(

	unicode N_FAR *s)				/* String whose length should be calculated	*/

{

	int		 i;					/* Loop variable							*/

	/*
		Find the null
	*/
	for ( i = 0 ; s[i] ; i++ );

	/*
		Return the length
	*/
	return (i);
}

size_t N_API unisize(

   unicode N_FAR *s)

{
	return (unilen(s) + 1) * sizeof(*s);
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unilen.c,v 1.1 1994/09/26 17:21:35 rebekah Exp $
*/

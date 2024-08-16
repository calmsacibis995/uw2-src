/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unicpy.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicpy.c,v 1.1 1994/09/26 17:21:31 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNICPY.C												*
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
	Unicode version of ANSI strcpy function

	Example:
		
		unicpy(s1, s2);

	Description:

		Copies the unicode string pointed to by s2 (including the terminating
		null character) into the array pointed to by s1.

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

unicode N_FAR * N_API unicpy(

	unicode	N_FAR *s1,			/* Buffer characters are copied to			*/
	unicode	N_FAR *s2)			/* Buffer characters are copied from		*/
{
   return( wcscpy( s1, s2 ) );
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

/****************************************************************************/

unicode N_FAR * N_API unicpy(

	unicode	N_FAR *s1,			/* Buffer characters are copied to			*/
	unicode	N_FAR *s2)			/* Buffer characters are copied from		*/

{

	int		 i = 0;				/* Loop variable							*/

	/*
		Copy the string
	*/
	do
	{
		s1[i++] = *s2;
	} while (*s2++);

	/*
		Return pointer to s1
	*/
	return (s1);
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicpy.c,v 1.1 1994/09/26 17:21:31 rebekah Exp $
*/

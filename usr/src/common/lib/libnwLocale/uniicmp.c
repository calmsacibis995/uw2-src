/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:uniicmp.c	1.1"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNIICMP.C												*
 *																			*
 * Date Created:	August 1992										*
 *																			*
 * Version:			1.00													*
 *																			*
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.			*
 *																			*
 * No part of this file may be duplicated, revised, translated, localized	*
 * or modified in any manner or compiled, linked or uploaded or downloaded	*
 * to or from any computer system without the prior written consent of 		*
 * Novell, Inc.																*
 *																			*
 ****************************************************************************/

#if (defined N_PLAT_WNT3 || defined N_PLAT_WNT4) && defined N_ARCH_32
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

int N_API uniicmp(unicode N_FAR *s1, unicode N_FAR *s2)
{
   return( wcsicmp( s1, s2 ) );
}

int N_API uninicmp(unicode N_FAR *s1, unicode N_FAR *s2, unsigned len)
{
   return( _wcsnicmp( s1, s2, len ) );
}

#else
#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

extern void N_FAR *monoHandle;


static unicode MonoCase(unicode c)
{
	unicode dest[2];
	size_t	len = 0;

	NWUnicodeToMonocase(monoHandle, dest, 2, &c, (size_t N_FAR *) &len);

	return *dest;
}

int N_API uniicmp(unicode N_FAR *s1, unicode N_FAR *s2)
{
	while (MonoCase(*s1) == MonoCase(*s2) && *s1)
	{
		s1++;
		s2++;
	}
	return MonoCase(*s1) - MonoCase(*s2);
}

int N_API uninicmp(unicode N_FAR *s1, unicode N_FAR *s2, unsigned len)
{
 	if (!len) return 0;
	while (MonoCase(*s1) == MonoCase(*s2) && *s1 && --len)
	{
		s1++;
		s2++;
	}
	return MonoCase(*s1) - MonoCase(*s2);
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniicmp.c,v 1.1 1994/09/26 17:21:34 rebekah Exp $
*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unicomp.c	1.1"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNICOMP.C												*
 *																			*
 * Date Created:	October 18, 1991										*
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

Compares two unicode characters.

*/

/****************************************************************************/

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

int N_API NWUnicodeCompare(		/* Compare two unicode characters			*/
	void N_FAR *ruleHandle,		   /* Rule table handle						*/
	unicode	chr1,						/* 1st character							*/	
	unicode	chr2)						/* 2nd character							*/
{
	return(CompareStringW(GetThreadLocale(), 0, &chr1, 1, &chr2, 1)-2);
}

#else

#ifndef MACINTOSH

#include "ntypes.h"
#include "unicode.h"

int N_API NWUnicodeCompare(		/* Compare two unicode characters			*/
	void N_FAR *ruleHandle,		   /* Rule table handle						*/
	unicode	chr1,						/* 1st character							*/	
	unicode	chr2)						/* 2nd character							*/
{

	int		 i;					/* Loop variable							*/
	unicode	 input1[3],			/* Buffer for unicode input					*/
	          input2[3],			/* Buffer for unicode input					*/
			    output1[3],		/* Buffer for unicode weights				*/
			    output2[3];		/* Buffer for unicode weights				*/
	size_t    len1,
             len2;

	/*
		Copy the unicode characters into the inputers
	*/
	input1[0] = chr1;
	input1[1] = 0;
	input2[0] = chr2;
	input2[1] = 0;

#if defined( N_PLAT_NLM )
	/* Convert the inputers to collation weights */
	NWUnicodeToCollation((nuint32)ruleHandle, output1, 3, input1, 0,(pnuint32)&len1);
	NWUnicodeToCollation((nuint32)ruleHandle, output2, 3, input2, 0,(pnuint32)&len2);

#else
	/* Convert the inputers to collation weights */
	NWUnicodeToCollation(ruleHandle, output1, 3, input1, 0,
                       (size_t N_FAR *) &len1);
	NWUnicodeToCollation(ruleHandle, output2, 3, input2, 0,
                       (size_t N_FAR *) &len2);

#endif

	/* Now compare the strings */
	/* We can't just do a simple subtraction because the unicode collation */
	/* Value has the potential to be multibyte */

	for ( i = 0 ; ; i++ )
	{
		if (output1[i] < output2[i])
			return (-1);
		if (output1[i] > output2[i])
			return (1);
		if (output1[i] == 0 && output2[i] == 0)
			return (0);
	}
}

#else	/* MACINTOSH */
# include "uniintrn.h"
# include "MacUnico.h"

int NWUnicodeCompare
(
	char	*ruleHandle,
	unicode	ch1,
	unicode	ch2	
)
{
	return ch1 - ch2;
}

#endif /* Macintosh */
#endif /* all but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unicomp.c,v 1.1 1994/09/26 17:21:30 rebekah Exp $
*/

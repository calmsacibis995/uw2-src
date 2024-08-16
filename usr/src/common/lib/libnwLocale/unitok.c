/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unitok.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unitok.c,v 1.1 1994/09/26 17:21:52 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNITOK.C												*
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
	Unicode version of ANSI strtok function

	Example:

		p = "a,bc.def.g,hi";
		do
		{
			t = unitok(p, ",.");
			.
			.
			.
			p = NULL;
		} while (t != NULL);

	Description:

		A sequence of calls to the strtok function breaks the string pointed
		to by s1 into a sequence of tokens, each of which is delimited by a
		character from the string pointed to by s2.  The first call in the
		sequence has s1 as its first argument, and is followed by calls with
		a null pointer as the first argument.  The separator string pointed
		to by s2 may different from call to call.

		The first call in the sequence searches the string pointed to by s1
		for the first character that is NOT contained in the current
		separator string pointed to by s2.  If no such character is found,
		then there are no tokens in the string pointed to by s1 and the
		unitok function returns a null pointer.  If such a character is
		found, it is the start of the first token.

		The function then searches beginning at the token found for a
		character that IS contained in the current separator string.  If no
		such character is found, the current token extends to the end of
		the string pointed to by s1, and subsequent searches for a token will
		return a null pointer.  If such a character	is found, it is
		overwritten by a null character, which terminates the current token.
		The function save a pointer to the following character, from which
		the next search for a token will start.

		Each subsequent call, with a null pointer as the value of the first
		argument, starts searching from the saved pointer and behaves as
		described above.

	Returns:
		
		The unitok function returns a pointer to the first character of a
		token, or a null pointer is there is no token.
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

unicode N_FAR * N_API unitok
(
	unicode N_FAR *parse,  		   /* String to be parsed						*/
	unicode N_FAR *delim	  		   /* Set of delimiters						*/
)
{
	return(wcstok(parse, delim));
}

#else
#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

#if defined(N_PLAT_UNIX)
#include "libnwlocale_mt.h"
#endif

unicode N_FAR * N_API unitok
(
	unicode N_FAR *parse,  		   /* String to be parsed						*/
	unicode N_FAR *delim	  		   /* Set of delimiters						*/
)
{
	int		i;							/* Loop variable							*/
	unicode N_FAR *token = NULL;	/* Pointer to a token we found				*/

	/*
	**  make this into thread specific storage
	*/
	static 	unicode
			  N_FAR *last;				/* End of last token we found plus two		*/

	/*
		New parse, or continuing from last call?
	*/

#if defined(N_PLAT_UNIX) && defined(_REENTRANT)
	struct _libnwlocale_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD)
	{
		if (parse == NULL)
			parse = last;

	} else {

		key_tbl = (struct _libnwlocale_tsd *)_mt_get_thr_specific_storage(
			_libnwlocale_key, _LIBNWLOCALE_KEYTBL_SIZE);

		if (key_tbl)
			parse = key_tbl->last;
		else
			return(NULL);
	}
#else
	if (parse == NULL)
		parse = last;
#endif

	/*
		Find the first character in parse that is not contained in delim
	*/
	for ( ; *parse ; parse++ )
	{
		/*
			Is this character in delim?
		*/
		for ( i = 0 ; delim[i] && delim[i] != *parse ; i++ );
		if (delim[i] == 0)
		{
			/*
				Found a character from parse that is NOT in delim.  So this
				is the start of a token
			*/
			token = parse;
			break;
		}
	}

	/*
		If we found a token, find the end of it
	*/
	if (token != NULL)
	{
		for ( parse++ ; *parse ; parse++ )
		{
			/*
				Is this character in delim?
			*/
			for ( i = 0 ; delim[i] && delim[i] != *parse ; i++ );
			if (delim[i] != 0)
			{
				/*
					Found a character from parse that is in delim.  So this
					is the end of the token
				*/
				*parse = 0;

#if defined (N_PLAT_UNIX) && defined(_REENTRANT)

				if (FIRST_OR_NO_THREAD)
					last = parse + 1;
				else
					key_tbl->last = parse + 1;
#else
				last = parse + 1;
#endif

				return (token);
			}
		}

		/*
			This token ends the string
		*/

#if defined (N_PLAT_UNIX) && defined(_REENTRANT)

		if (FIRST_OR_NO_THREAD)
			last = parse;
		else
			key_tbl->last = parse;
#else
		last = parse;
#endif
		return (token);
	}

	/*
		Return whatever we found
	*/
	return token;
}

#endif /* Macintosh */
#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unitok.c,v 1.1 1994/09/26 17:21:52 rebekah Exp $
*/

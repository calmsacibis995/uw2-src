/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/strip.c	1.4.5.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

/**
 ** strip() - STRIP LEADING AND TRAILING BLANKS
 **/

char *
#if	defined(__STDC__)
strip (
	char *			str
)
#else
strip (str)
	register char		*str;
#endif
{
	register char		*p;

	if (!str || !*str)
		return (0);

	str += strspn(str, " ");
	for (p = str; *p; p++)
		;
	p--;
	for (; p >= str && *p == ' '; p--)
		;
	*++p = 0;
	return (str);
}

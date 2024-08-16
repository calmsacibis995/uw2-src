/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/sprintlist.c	1.7.6.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** sprintlist() - FLATTEN (char **) LIST INTO (char *) LIST
 **/

char *
#if	defined(__STDC__)
sprintlist (
	char **			list
)
#else
sprintlist (list)
	char			**list;
#endif
{
	register char		**plist,
				*p,
				*q;

	char			*ret;

	int			len	= 0;


	if (!list || !*list)
		return (0);

	for (plist = list; *plist; plist++)
		len += strlen(*plist) + 1;

	if (!(ret = Malloc(len))) {
		errno = ENOMEM;
		return (0);
	}

	q = ret;
	for (plist = list; *plist; plist++) {
		p = *plist;
		while (*q++ = *p++)
			;
		q[-1] = ' ';
	}
	q[-1] = 0;

	return (ret);
}

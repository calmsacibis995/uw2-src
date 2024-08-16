/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/dellist.c	1.8.6.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** dellist() - REMOVE ITEM FROM (char **) LIST
 **/

int
#if	defined(__STDC__)
dellist (
	char ***		plist,
	char *			item
)
#else
dellist (plist, item)
	register char		***plist,
				*item;
#endif
{
	register char		**pl;

	register int		n;

	if (*plist) {

		n = lenlist(*plist);

		for (pl = *plist; *pl; pl++)
			if (STREQU(*pl, item))
				break;

		if (*pl) {
			Free (*pl);
			for (; *pl; pl++)
				*pl = *(pl+1);
			if (--n == 0) {
				Free ((char *)*plist);
				*plist = 0;
			} else {
				*plist = (char **)Realloc(
					(char *)*plist,
					(n + 1) * sizeof(char *)
				);
				if (!*plist) {
					errno = ENOMEM;
					return (-1);
				}
/*				(*plist)[n] = 0; /* done in "for" loop */
			}
		}

	}

	return (0);
}

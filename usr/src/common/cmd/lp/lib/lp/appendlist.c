/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/appendlist.c	1.8.6.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"
#include "errno.h"
#include "sys/types.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** appendlist() - ADD ITEM TO (char **) LIST
 **/

int
#if	defined(__STDC__)
appendlist (
	char ***		plist,
	char *			item
)
#else
appendlist (plist, item)
	register char		***plist,
				*item;
#endif
{
	register int		n;

	if (*plist) {

		n = lenlist(*plist);

		n++;
		*plist = (char **)Realloc(
			(char *)*plist,
			(n + 1) * sizeof(char *)
		);
		if (!*plist) {
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[n - 1] = Strdup(item);
		(*plist)[n] = 0;

	} else {

		*plist = (char **)Malloc(2 * sizeof(char *));
		if (!*plist) {
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[0] = Strdup(item);
		(*plist)[1] = 0;

	}

	return (0);
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/searchlist.c	1.5.5.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "lp.h"

/**
 ** searchlist() - SEARCH (char **) LIST FOR ITEM
 **/

int
#if	defined(__STDC__)
searchlist (
	char *			item,
	char **			list
)
#else
searchlist (item, list)
	register char		*item;
	register char		**list;
#endif
{
	if (!list || !*list)
		return (0);

	else if (STREQU(item, NAME_ANY) || STREQU(item, NAME_ALL))
		return (1);

	/*
	 * This is a linear search--we believe that the lists
	 * will be short.
	 */
	while (*list) {
		if (
			STREQU(*list, item)
		     || STREQU(*list, NAME_ANY)
		     || STREQU(*list, NAME_ALL)
		)
			return (1);
		list++;
	}
	return (0);
}

/**
 ** searchlist_with_terminfo() - SEARCH (char **) LIST FOR ITEM
 **/

int
#if	defined(__STDC__)
searchlist_with_terminfo (
	char *			item,
	char **			list
)
#else
searchlist_with_terminfo (item, list)
	register char		*item;
	register char		**list;
#endif
{
	if (!list || !*list)
		return (0);

	else if (STREQU(item, NAME_ANY) || STREQU(item, NAME_ALL))
		return (1);

	/*
	 * This is a linear search--we believe that the lists
	 * will be short.
	 */
	while (*list) {
		if (
			STREQU(*list, item)
		     || STREQU(*list, NAME_ANY)
		     || STREQU(*list, NAME_ALL)
		     || (
				STREQU(*list, NAME_TERMINFO)
			     && isterminfo(item)
			)
		)
			return (1);
		list++;
	}
	return (0);
}

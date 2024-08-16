/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/bsd/findfld.c	1.1.5.4"
#ident	"$Header: $"

#include <string.h>
#include "lp.h"

/*
 * Return pointer to field in 'buf' beginning with 'ptn'
 * (Input string unmolested)
 */
char *
#if defined (__STDC__)
find_strfld(register char *ptn, register char *buf)
#else
find_strfld(ptn, buf)
register char	*ptn;
register char	*buf;
#endif
{
	char	**list;
	char	**pp;
	char	 *p = NULL;
	int 	  n = strlen(ptn);

	if (!(list = dashos(buf)))
		return(NULL);
	for (pp = list; *pp; pp++)
		if (STRNEQU(ptn, *pp, n)) {
			p = strdup(*pp);
			break;
		}
	freelist(list);
	return(p);
}

/*
 * Return pointer to list entry beginning with 'ptn'
 */
char *
#if defined (__STDC__)
find_listfld(register char *ptn, register char **list)
#else
find_listfld(ptn, list)
register char	 *ptn;
register char	**list;
#endif
{
	int	n = strlen(ptn);

	if (!list || !*list)
		return(NULL);
	for (; *list; ++list)
                if (STRNEQU(ptn, *list, n))
                        return (*list);
        return (NULL);
}

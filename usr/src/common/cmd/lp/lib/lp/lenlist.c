/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/lenlist.c	1.4.5.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/**
 ** lenlist() - COMPUTE LENGTH OF LIST
 **/

int
#if	defined(__STDC__)
lenlist (
	char **			list
)
#else
lenlist (list)
	char			**list;
#endif
{
	register char **	pl;

	if (!list)
		return (0);
	for (pl = list; *pl; pl++)
		;
	return (pl - list);
}

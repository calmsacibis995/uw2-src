/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/xlink.c	6.3.1.1"
/*
	Interface to link(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

# include	"errno.h"
int
xlink(f1,f2)
char	*f1, *f2;
{
	extern errno;
	int	link(), sprintf(), fatal(), xmsg();

	if (link(f1,f2)) {
		if (errno == EEXIST || errno == EXDEV) {
			return(fatal(":241:can't link `%s' to `%s' (%d)",
				f2,f1,errno == EEXIST ? 111 : 112));
		}
		if (errno == EACCES)
			f1 = f2;
		return(xmsg(f1,"xlink"));
	}
	return(0);
}

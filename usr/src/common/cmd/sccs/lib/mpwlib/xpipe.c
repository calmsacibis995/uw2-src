/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/xpipe.c	6.3"
/*
	Interface to pipe(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

int
xpipe(t)
int *t;
{
	static char p[]="pipe";
	int	pipe(), xmsg();

	if (pipe(t) == 0)
		return(0);
	return(xmsg(p,p));
}

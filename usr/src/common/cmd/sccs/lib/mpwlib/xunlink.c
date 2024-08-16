/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/xunlink.c	6.3"
/*
	Interface to unlink(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

int
xunlink(f)
char	*f;
{
	int	unlink(), xmsg();
	if (unlink(f))
		return(xmsg(f,"xunlink"));
	return(0);
}

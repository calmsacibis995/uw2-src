/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_getname.c	1.1.6.2"
#ident	"$Header: $"

/*
 *
 * This routine was never documented in 4.0, but we leave it here
 * for backward compatibility.  New programs should use t_getprotaddr().
 *
 */


#include <stdio.h>
#include <errno.h>
#include <sys/xti.h>


int
t_getname(int fd, struct netbuf *name, int type)
{
	struct t_bind bind;

	if (!name || ((type != LOCALNAME) && (type != REMOTENAME))) {
		errno = EINVAL;
		return(-1);
	}

	bind.addr.maxlen = name->maxlen;
	bind.addr.buf = name->buf;

	return((type == LOCALNAME)?
		t_getprotaddr(fd, &bind, NULL) :
		t_getprotaddr(fd, NULL, &bind));
}

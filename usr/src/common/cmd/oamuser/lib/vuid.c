/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/vuid.c	1.2.11.2"
#ident  "$Header: vuid.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<pwd.h>
#include	<userdefs.h>
#include	<users.h>

#include	<sys/param.h>

#ifndef MAXUID
#include	<limits.h>
#define	MAXUID	UID_MAX
#endif

struct passwd *getpwuid();

int
valid_uid(uid, pptr)
	uid_t uid;
	struct passwd **pptr;
{
	register struct passwd *t_pptr;

	if (uid < 0)
		return INVALID;

	if (uid > MAXUID)
		return TOOBIG;

	if (t_pptr = getpwuid(uid)) {
		if (pptr)
			*pptr = t_pptr;
		return NOTUNIQUE;
	}

	if (uid <= DEFRID) {
		if (pptr)
			*pptr = getpwuid(uid);
		return RESERVED;
	}

	return UNIQUE;
}

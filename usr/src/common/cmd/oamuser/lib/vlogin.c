/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/vlogin.c	1.3.13.6"
#ident  "$Header: vlogin.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<pwd.h>
#include	<shadow.h>
#include	<userdefs.h>
#include	<users.h>
#include    <limits.h>

extern	struct	passwd	*getpwnam(), *nis_getpwnam();
extern	struct	spwd	*getspnam();

/*
 * validate string given as login name.
 */
int
valid_login(login, pptr)
	char	*login;
	struct passwd	**pptr;
{
	register struct	passwd	*t_pptr;
	register struct	spwd	*s_pptr;
	register char *ptr = login;

	if (!login || !*login) {
		return INVALID;
	}

	if (!strcmp(login, ".") || !strcmp(login, "..")) {
		return INVALID;
	}
		
	if (strlen(login) >= (size_t) LOGNAME_MAX) {
		return INVALID;
	}

	for ( ; *ptr != NULL; ptr++) {
		if (!isprint(*ptr) || (*ptr == ':'))
			return INVALID;
	}
	if (t_pptr = nis_getpwnam(login)) {
		if (pptr) {
			*pptr = t_pptr;
		}
		return NOTUNIQUE;
	}
	if (s_pptr = getspnam(login)) {
		return NOTUNIQUE;
	}
	return UNIQUE;
}


int
all_numeric(strp)
	char	*strp;
{
	register char *ptr = strp;

	for (; *ptr != NULL; ptr++) {
		if (!isdigit(*ptr)) {
			return 0;
		}
	}
	return 1;
}

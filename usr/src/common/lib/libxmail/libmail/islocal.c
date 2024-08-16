/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/islocal.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)islocal.c	1.10 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	islocal - see if user exists on this system

    SYNOPSIS
	int islocal(const char *user, uid_t *puid)

    DESCRIPTION
	Return an indication if the given name has or can have a
	mailbox on the system. If so, also return their uid.
*/

int islocal(user, puid)
const char *user;
uid_t *puid;
{
	string	*fname;
	struct stat statb;
	struct passwd *pwd_ptr;

	/* Check for existing mailfile first */
	fname = s_copy(maildir);
	fname = s_append(fname, user);
	if (stat(s_to_c(fname),&statb) == 0) {
		if (puid) *puid = statb.st_uid;
		s_free(fname);
		return 1;
	}

	/* Check for existing forwarding file next */
	s_restart(fname);
	fname = s_xappend(fname, mailfwrd, user, (char*)0);
	if (stat(s_to_c(fname),&statb) == 0) {
		if (puid) *puid = statb.st_uid;
		s_free(fname);
		return 1;
	}

	/* If no existing mailfile, check passwd file */
	/*setpwent();*/
	_abi_setpwent();
	if ((pwd_ptr = getpwnam(user)) == NULL) {
		s_free(fname);
		return 0;
	}
	if (puid) *puid = pwd_ptr->pw_uid;
	s_free(fname);
	return 1;
}

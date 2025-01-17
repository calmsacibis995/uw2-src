/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/uid.c	1.5.12.2"
#ident  "$Header: uid.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<pwd.h>
#include	<mac.h>
#include	"uidage.h"

#include	<sys/param.h>

#ifndef	MAXUID
#include	<limits.h>
#ifdef UID_MAX
#define	MAXUID	UID_MAX
#else 
#define	MAXUID	60000
#endif
#endif

extern	int	uid_age(),
		add_uid();

extern	void	free();

uid_t
findnextuid()
{
	register int	end_of_file = 0;

	uid_t	uid = 0;

	struct	passwd	*pent;

	extern	int	errno;

	struct	uid_blk	*uid_sp,
			*uid_sp2;

	errno = 0;
	/*
	 * create the head of the uid number list
	*/
	if ((uid_sp = (struct uid_blk *) malloc((size_t) sizeof(struct uid_blk))) == NULL) {
		exit(EX_FAILURE);
	}
	uid_sp->link = NULL;
	uid_sp->low = (UID_MIN - 1);
	uid_sp->high = (UID_MIN - 1);

	while (!end_of_file) {
		if ((pent = (struct passwd *) getpwent()) != NULL) {
			if (add_uid(pent->pw_uid, &uid_sp) == -1) {
				endpwent();
				return -1;
			}
		}
		else {
			if (errno == 0)
				end_of_file = 1;
			else {
				if (errno == EINVAL)
					errno = 0;

				else end_of_file = 1;
			}
		}
	}
	endpwent();

	uid = uid_sp->high + 1;	
	uid_sp2 = uid_sp;
	while (uid_age(CHECK, uid, 0) == 0) {
		if (++uid == uid_sp2->link->low) {
			uid_sp2=uid_sp2->link;
			uid = uid_sp2->high + 1;	 
		}
	}
	return uid;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/xopen.c	6.4.1.1"
/*
	Interface to open(II) which differentiates among the various
	open errors.
	Returns file descriptor on success,
	fatal() on failure.
*/

# include "errno.h"
# include <ccstypes.h>

xopen(name,mode)
char name[];
mode_t mode;
{
	register int fd;
	extern int errno;
	int	open(), sprintf(), fatal(), xmsg();

	if ((fd = open(name,mode)) < 0) {
		if(errno == EACCES) {
			if(mode == 0)
				fd=fatal(":248:`%s' unreadable (ut5)",name);

			else if(mode == 1)
				fd=fatal(":249:`%s' unwritable (ut6)",name);

			else
				fd=fatal(":250:`%s' unreadable or unwritable (ut7)",name);

		}
		else
			fd = xmsg(name,"xopen");
	}
	return(fd);
}

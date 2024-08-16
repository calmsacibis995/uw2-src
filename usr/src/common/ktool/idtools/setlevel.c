/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/setlevel.c	1.5"
#ident	"$Header: $"

#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>

#ifndef CROSS
#include <sys/types.h>
#include <mac.h>
#define MACDEFAULT	2	/* SYS_PRIVATE */
#else
typedef unsigned long level_t;
#endif

/*
 * Set MAC-Level
 */
int
#ifdef __STDC__
setlevel(char *path, level_t level)
#else
setlevel(path, level)
char *path;
level_t level;
#endif
{
#ifndef CROSS
	if (level == 0)
		level = MACDEFAULT;

	if (lvlfile(path, MAC_SET, &level) < 0) {
		/*
		 * If the lvlfile failed due to a non-MAC file system,
		 * ignore it.
		 */
		if (errno == ENOSYS)
			return(0);

		pfmt(stderr, MM_WARNING,
			":229:can't change MAC-Level of \"%s\" to %d, %s\n",
			path, level, strerror(errno));

		return(-1);
	}
#endif

	return(0);
}

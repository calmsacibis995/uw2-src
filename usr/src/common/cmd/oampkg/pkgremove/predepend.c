/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgremove/predepend.c	1.2.6.3"
#ident  "$Header: predepend.c 1.2 91/06/27 $"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pkglocs.h>

extern int	warnflag;

extern int	unlink();
extern void	progerr();

#define ERR_UNLINK	":331:unable to unlink <%s>"

void
predepend(oldpkg)
char	*oldpkg;
{
	struct stat status;
	char	spath[PATH_MAX];

	oldpkg = strtok(oldpkg, " \t\n");
	if(oldpkg == NULL)
		return;

	do {
		(void) sprintf(spath, "%s/%s.name", PKGOLD, oldpkg);
		if(lstat(spath, &status) == 0) {
			if(status.st_mode & S_IFLNK) {
				if(unlink(spath)) {
					progerr(ERR_UNLINK, spath);
					warnflag++;
				}
				return;
			}
		}
	} while(oldpkg = strtok(NULL, " \t\n"));
}

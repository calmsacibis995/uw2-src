/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/pkgvolume.c	1.5.7.6"
#ident  "$Header: pkgvolume.c 1.2 91/06/27 $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <pkgdev.h>
#include <pkgstrct.h>

extern char	instdir[],
		pkgbin[];

extern void	progerr(),
		quit();
extern int	chdir(),
		pkgumount(),
		pkgmount(),
		ckvolseq();

void
pkgvolume(devp, pkg, part, nparts)
struct pkgdev *devp;
char	*pkg;
int	part;
int	nparts;
{
	static int	cpart = 0;
	char	path[PATH_MAX];
	int	n;

	if(devp->cdevice)
		return;
	if(cpart == part)
		return;
	cpart = part;

	if(part == 1) {
		if(ckvolseq(instdir, 1, nparts, LOG)) {
			progerr(":419:corrupt directory structure");
			quit(99);
		}
		cpart = 1;
		return;
	}

	if(devp->mount == NULL) {
		if(ckvolseq(instdir, part, nparts, LOG)) {
			progerr(":419:corrupt directory structure");
			quit(99);
		}
		return;
	}

	if(ckvolseq(instdir, part, nparts, NOLOG)) {
		for(;;) {
			(void) chdir("/");
			if(n = pkgumount(devp)) {
				progerr(":420:attempt to unmount <%s> failed (%d)", 
					devp->bdevice, n);
				quit(99);
			}
			if (n = pkgmount(devp, pkg, part, nparts, 1)) {
				/*
				 * pkgmount returns 3 if the user types 'q'
				 * to quit at the prompt to insert a new
				 * volume.
				 */
				if (n == 3) {
					/*
					 * A new return code to communicate
					 * with the invoking program that the
					 * file system is already umounted.
					 */
					n += 30;
				}
				quit(n);
			}
			(void) sprintf(path, "%s/%s", devp->dirname, pkg);
			if(ckvolseq(path, part, nparts, LOG) == 0)
				break;
		}
	}
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/pkgmount.c	1.10.14.1"
#ident "$Header: $"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pkgdev.h>
#include <pkginfo.h>
#include <sys/types.h>
#include <devmgmt.h>
#include <sys/mount.h>

#include <pfmt.h>

extern int	errno;
extern FILE	*epopen();
extern void	quit(),
		progerr(),
		logerr();
extern int	getvol(),
		pkgexecl();

#define CMDSIZ	4096
#define ERR_MOUNTFS "uxpkgtools:646:processing of media inserted in %s failed."
#define ERR_FSTYP	"uxpkgtools:647:Please check if the correct media is inserted."
#define MOUNT		"/sbin/mount"
#define UMOUNT		"/sbin/umount"
#define FSTYP		"/sbin/fstyp"

#define LABEL0	gettxt("uxpkgtools:648", "Insert %%v %d of %d for <%s> package into %%p.")
#define LABEL1	gettxt("uxpkgtools:649", "Insert %%v for <%s> package into %%p.")
#define LABEL2	gettxt("uxpkgtools:650", "Insert %%v %d of %d into %%p.")
#define LABEL3	gettxt("uxpkgtools:651", "Insert %%v into %%p.")

int	Mntflg = 0;

int
pkgmount(devp, pkg, part, nparts, getvolflg)
struct pkgdev *devp;
char	*pkg;
int	part, nparts, getvolflg;
{
	int	n, flags, cdflg = 0;
	char	*pt, prompt[64], cmd[CMDSIZ];
	FILE	*pp;

	if (devp->mntflg) {
		return(0);
	}
	if(part && nparts) {
		if(pkg)
			(void) sprintf(prompt, LABEL0, part, nparts, pkg);
		else
			(void) sprintf(prompt, LABEL2, part, nparts);
	} else if(pkg)
		(void) sprintf(prompt, LABEL1, pkg);
	else
		(void) sprintf(prompt, LABEL3);

	n = 0;
	for(;;) {
		if(!getvolflg && n)
			/*
			 * Return to caller if not prompting
			 * and error was encountered.
			 */
			return -1;
		if (!devp->fstyp) {
			devp->fstyp = devattr(devp->name, "fstype");
		}
		if (devp->fstyp && (strcmp(devp->fstyp, "cdfs") == 0)) {
			cdflg = 1;
		}
		if(getvolflg && (cdflg == 0) &&
		   (n = getvol(devp->bdevice, NULL, 
		      (devp->rdonly ? 0 : DM_FORMFS|DM_WLABEL), prompt))) {
			if(n == 3)
				return(3);
			if(n == 2) {
				progerr(ERR_MOUNTFS, devp->name);
				progerr("uxpkgtools:138:unknown device <%s>", devp->bdevice);
			} else {
				progerr(ERR_MOUNTFS, devp->name);
				progerr("uxpkgtools:139:unable to obtain package volume");
			}
			return(99);
		}

		if(devp->fstyp == NULL) {
			(void) sprintf(cmd, "%s %s", FSTYP, devp->bdevice);
			if((pp = epopen(cmd, "r")) == NULL) {
				rpterr();
				progerr(ERR_MOUNTFS, devp->name);
				logerr(ERR_FSTYP);
				n = -1;
				continue;
			}
			cmd[0] = '\0';
			if(fgets(cmd, CMDSIZ, pp) == NULL) {
				progerr(ERR_MOUNTFS, devp->name);
				logerr(ERR_FSTYP);
				(void) pclose(pp);
				n = -1;
				continue;
			}
			if(epclose(pp)) {
				rpterr();
				progerr(ERR_MOUNTFS, devp->name);
				logerr(ERR_FSTYP);
				n = -1;
				continue;
			}
			if(pt = strpbrk(cmd, " \t\n"))
				*pt = '\0';
			if(cmd[0] == '\0') {
				progerr(ERR_MOUNTFS, devp->name);
				logerr(ERR_FSTYP);
				n = -1;
				continue;
			}	
			devp->fstyp = strdup(cmd);
			if (strcmp(devp->fstyp, "cdfs") == 0) {
				cdflg = 1;
			}
		}

		if (Mntflg) {
			flags = MS_FSS;
			if(devp->rdonly)
				flags |= MS_RDONLY;
			n = mount(devp->bdevice, devp->mount,
				flags, devp->fstyp);
		} else if(devp->rdonly) {
			n = pkgexecl(NULL, NULL, NULL, MOUNT, "-r", "-F", devp->fstyp,
				devp->bdevice, devp->mount, NULL);
		} else {
			n = pkgexecl(NULL, NULL, NULL, MOUNT, "-F", devp->fstyp,
				devp->bdevice, devp->mount, NULL);
		}
		if(n) {
			progerr(ERR_MOUNTFS, devp->name);
			progerr("uxpkgtools:652:mount of %s failed", devp->bdevice);
			continue;
		}
		devp->mntflg++; 
		break;
	}
	return(0);
}

int
pkgumount(devp)
struct pkgdev *devp;
{
	int	n;

	if(!devp->mntflg)
		return(0);

	if (Mntflg) {
		n = umount(devp->bdevice);
	} else {
		n = pkgexecl(NULL, NULL, NULL, UMOUNT, devp->bdevice, NULL);
	}
	if (n == 0)
		devp->mntflg = 0;
	return(n);
}

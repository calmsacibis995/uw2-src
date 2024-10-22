/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:getvfsent.c	1.9.1.1"

/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getvfsspec = _getvfsspec
	#pragma weak getvfsfile = _getvfsfile
	#pragma weak getvfsany	= _getvfsany
	#pragma weak getvfsent	= _getvfsent
#endif
#include	"synonyms.h"
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/vfstab.h>
#include	<mac.h>
#include	<errno.h>
#include	<string.h>

extern char * _lgen_strtok_r(char *, const char *, char **);

#define	GETTOK(xx, ll, yy)\
	if ((vp->xx = _lgen_strtok_r(ll, sepstr, &yy)) == NULL)\
		return	VFS_TOOFEW;\
	if (strcmp(vp->xx, dash) == 0)\
		vp->xx = NULL
#define	DIFF(xx)\
	(vrefp->xx != NULL && (vgetp->xx == NULL ||\
	 strcmp(vrefp->xx, vgetp->xx) != 0))
#define	SDIFF(xx, typem, typer)\
	(vgetp->xx == NULL || stat(vgetp->xx, &statb) == -1 ||\
	(statb.st_mode & S_IFMT) != typem ||\
	 statb.st_rdev != typer)

static char	line[VFS_LINE_MAX];
static const char	sepstr[] = " \t\n";
static const char	dash[] = "-";

static	int getline();

int
getvfsspec(fd, vp, special)
	register FILE	*fd;
	register struct vfstab	*vp;
	register const char	*special;
{
	struct vfstab	vv;

	vfsnull(&vv);
	vv.vfs_special = (char *)special;

	return	getvfsany(fd, vp, &vv);
}

int
getvfsfile(fd, vp, mountp)
	register FILE	*fd;
	register struct vfstab	*vp;
	register const char	*mountp;
{
	struct vfstab	vv;

	vfsnull(&vv);
	vv.vfs_mountp = (char *)mountp;

	return	getvfsany(fd, vp, &vv);
}



int
getvfsany(fd, vgetp, vrefp)
	register FILE	*fd;
	register struct vfstab	*vgetp;
	register const struct vfstab	*vrefp;
{
	register int	ret, bstat, cstat;
	register mode_t	bmode, cmode;
	register dev_t	brdev, crdev;
	struct stat	statb;



	if (vrefp->vfs_special && stat(vrefp->vfs_special, &statb) == 0 &&
	  ((bmode = (statb.st_mode & S_IFMT)) == S_IFBLK || bmode == S_IFCHR)) {
		bstat = 1;
		brdev = statb.st_rdev;
	} else
		bstat = 0;

	if (vrefp->vfs_fsckdev && stat(vrefp->vfs_fsckdev, &statb) == 0 &&
	  ((cmode = (statb.st_mode & S_IFMT)) == S_IFBLK || cmode == S_IFCHR)) {
		cstat = 1;
		crdev = statb.st_rdev;
	} else
		cstat = 0;

	while ((ret = getvfsent(fd, vgetp)) == 0 &&
	      ((bstat == 0 && DIFF(vfs_special)) ||
	       (bstat == 1 && SDIFF(vfs_special, bmode, brdev)) ||
	       (cstat == 0 && DIFF(vfs_fsckdev)) ||
	       (cstat == 1 && SDIFF(vfs_fsckdev, cmode, crdev)) ||
	       DIFF(vfs_mountp) ||
	       DIFF(vfs_fstype) ||
	       DIFF(vfs_fsckpass) ||
	       DIFF(vfs_automnt) ||
	       DIFF(vfs_mntopts)))
		;
	return	ret;
}

int
getvfsent(fd, vp)
	FILE	*fd;
	struct vfstab	*vp;
{

	int	ret;
	char 	*last;

	/* skip leading spaces and comments */
	if ((ret = getline(line, fd)) != 0)
		return	ret;

	/* split up each field */
	GETTOK(vfs_special, line, last);
	GETTOK(vfs_fsckdev, NULL, last);
	GETTOK(vfs_mountp, NULL, last);
	GETTOK(vfs_fstype, NULL, last);
	GETTOK(vfs_fsckpass, NULL, last);
	GETTOK(vfs_automnt, NULL, last);
	GETTOK(vfs_mntopts, NULL, last);
	
	/* check if there is a value for vfs_macceiling */
	/* did not use GETTOK to be compatible with 4.0 /etc/vfstab */

	vp->vfs_macceiling = _lgen_strtok_r(NULL, sepstr, &last);
	if (vp->vfs_macceiling && (strcmp(vp->vfs_macceiling, dash) == 0))
		vp->vfs_macceiling = NULL;

	if (_lgen_strtok_r(NULL, sepstr, &last) != NULL)
		return	VFS_TOOMANY;

	return	0;
}

static int
getline(lp, fd)
	char	*lp;
	FILE	*fd;
{
	char	*cp;

	while ((lp = fgets(lp, VFS_LINE_MAX, fd)) != NULL) {
		if (strlen(lp) == VFS_LINE_MAX-1 && lp[VFS_LINE_MAX-2] != '\n')
			return	VFS_TOOLONG;

		for (cp = lp; *cp == ' ' || *cp == '\t'; cp++)
			;

		if (*cp != '#' && *cp != '\n')
			return	0;
	}
	return	-1;
}

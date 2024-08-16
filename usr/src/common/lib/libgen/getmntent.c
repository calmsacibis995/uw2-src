/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:getmntent.c	1.5.1.1"

/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getmntany = _getmntany
	#pragma weak getmntent = _getmntent
#endif
#include	"synonyms.h"
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mnttab.h>
#include	<string.h>
#include 	<stddef.h>

/*
 * library routines cant call strtok directly since it destroys
 * any user supplied information. Use an version internal libgen 
 */

char *
_lgen_strtok_r(string, sepset, lasts)
register char *string;
const char *sepset;
char **lasts;
{
	register char	*q, *r;

	q = string;

	/*first or subsequent call*/
	if (q == NULL)
		q = *lasts;

	if(q == 0)		/* return if no tokens remaining */
		return(NULL);

	q = q + strspn(q, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		*lasts = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		*lasts = r+1;
	}
	return(q);
}

#define	GETTOK(xx, ll, yy)\
	if ((mp->xx = _lgen_strtok_r(ll, sepstr, &yy)) == NULL)\
		return	MNT_TOOFEW;\
	if (strcmp(mp->xx, dash) == 0)\
		mp->xx = NULL
#define	DIFF(xx)\
	(mrefp->xx != NULL && (mgetp->xx == NULL ||\
	 strcmp(mrefp->xx, mgetp->xx) != 0))
#define	SDIFF(xx, typem, typer)\
	(mgetp->xx == NULL || stat(mgetp->xx, &statb) == -1 ||\
	(statb.st_mode & S_IFMT) != typem ||\
	 statb.st_rdev != typer)

static char	line[MNT_LINE_MAX];
static const char	sepstr[] = " \t\n";
static const char	dash[] = "-";

static int	getline();

int
getmntany(fd, mgetp, mrefp)
	register FILE	*fd;
	register struct	mnttab	*mgetp;
	register struct mnttab	*mrefp;
{
	register int	ret, bstat;
	register mode_t	bmode;
	register dev_t	brdev;
	struct stat	statb;

	if (mrefp->mnt_special && stat(mrefp->mnt_special, &statb) == 0 &&
	  ((bmode = (statb.st_mode & S_IFMT)) == S_IFBLK || bmode == S_IFCHR)) {
		bstat = 1;
		brdev = statb.st_rdev;
	} else
		bstat = 0;

	while ((ret = getmntent(fd, mgetp)) == 0 &&
	      ((bstat == 0 && DIFF(mnt_special)) ||
	       (bstat == 1 && SDIFF(mnt_special, bmode, brdev)) ||
	       DIFF(mnt_mountp) ||
	       DIFF(mnt_fstype) ||
	       DIFF(mnt_mntopts) ||
	       DIFF(mnt_time)))
		;
	return	ret;
}

int
getmntent(fd, mp)
	register FILE	*fd;
	register struct mnttab	*mp;
{
	register int	ret;
	char *last;

	/* skip leading spaces and comments */
	if ((ret = getline(line, fd)) != 0)
		return	ret;

	/* split up each field */
	GETTOK(mnt_special, line, last);
	GETTOK(mnt_mountp, NULL, last);
	GETTOK(mnt_fstype, NULL, last);
	GETTOK(mnt_mntopts, NULL, last);
	GETTOK(mnt_time, NULL, last);

	/* check for too many fields */
	if (_lgen_strtok_r(NULL, sepstr, &last) != NULL)
		return	MNT_TOOMANY;

	return	0;
}

static int
getline(lp, fd)
	register char	*lp;
	register FILE	*fd;
{
	register char	*cp;

	while ((lp = fgets(lp, MNT_LINE_MAX, fd)) != NULL) {
		if (strlen(lp) == MNT_LINE_MAX-1 && lp[MNT_LINE_MAX-2] != '\n')
			return	MNT_TOOLONG;

		for (cp = lp; *cp == ' ' || *cp == '\t'; cp++)
			;

		if (*cp != '#' && *cp != '\n')
			return	0;
	}
	return	-1;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/pkgxpand.c	1.3.5.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libpkg/pkgxpand.c,v 1.1 91/02/28 20:55:28 ccs Exp $"

#include <stdio.h>
#include <limits.h>
#include <string.h>

#include <pfmt.h>

extern char	*fpkginst();
extern void	*calloc(), 
		*realloc();
extern char	*pkgdir;

#define LSIZE	512
#define MALSIZ	16

char **
pkgalias(pkg)
char *pkg;
{
	FILE	*fp;
	char	path[PATH_MAX], *pkginst;
	char	*mypkg, *myarch, *myvers, **pkglist;
	char	line[LSIZE];
	int	n, errflg;

	pkglist = (char **) calloc(MALSIZ, sizeof(char *));
	if(pkglist == NULL)
		return((char **) 0);

	(void) sprintf(path, "%s/%s/pkgmap", pkgdir, pkg);
	if((fp = fopen(path, "r")) == NULL)
		return((char **) 0);

	n = errflg = 0;
	while(fgets(line, LSIZE, fp)) {
		mypkg = strtok(line, " \t\n");
		myarch = strtok(NULL, "( \t\n)");
		myvers = strtok(NULL, "\n");

		(void) fpkginst(NULL);
		pkginst = fpkginst(mypkg, myarch, myvers); 
		if(pkginst == NULL) {
			logerr("uxpkgtools:684:no package instance for [%s]", mypkg);
			errflg++;
			continue;
		}
		if(errflg)
			continue;

		pkglist[n] = strdup(pkginst);
		if((++n % MALSIZ) == 0) {
			pkglist = (char **) realloc(pkglist, 
				(n+MALSIZ)*sizeof(char *));
			if(pkglist == NULL)
				return((char **) 0);
		}
	}
	pkglist[n] = NULL;

	(void) fclose(fp);
	if(errflg) {
		while(n-- >= 0)
			free(pkglist[n]);
		free(pkglist);
		return((char **) 0);
	}
	return(pkglist);
}

#define ispkgalias(p)	(*p == '+')

char **
pkgxpand(pkg)
char *pkg[];
{
	static int level = 0;
	char	**pkglist;
	int	i;

	if(++level >= 0)
		pfmt(stdout, MM_NOSTD, "uxpkgtools:685:too deep");
	for(i=0; pkg[i]; i++) {
		if(ispkgalias(pkg[i])) {
			pkglist = pkgxpand(pkg[i]);
			pkgexpand(pkglist);
		}
	}
}

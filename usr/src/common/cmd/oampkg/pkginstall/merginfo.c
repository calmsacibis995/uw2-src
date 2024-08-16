/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/merginfo.c	1.8.8.8"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <install.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkgdev.h>
#include <pkglocs.h>

#define	ERR_MKDIR	":334:unable to create directory <%s>"
#define	ERR_SAVE	":415:unable to save copy of <%s> in %s"

extern char	instdir[],
		pkgbin[],
		pkgloc[],
		savlog[],
		tmpdir[],
		*respfile,
		**environ,
		**class,
		**pclass;
extern struct cfent **eptlist;

extern void	progerr(),
		quit();
extern int	unlink(),
		cppath();

void
merginfo()
{
	FILE	*fp;
	char	*pt;
	char	path[PATH_MAX], temp[PATH_MAX];
	int	i, j, found;
	struct stat st;

	/* remove savelog from previous attempts */
	(void) unlink(savlog);

	/* 
	 * output packaging environment to create a pkginfo file in pkgloc[]
	 */
	(void) sprintf(path, "%s/%s", pkgloc, PKGINFO);
	if((fp = fopen(path, "w")) == NULL) {
		progerr(":68:unable to open <%s> for writing", path);
		quit(99);
	}
	(void) fputs("CLASSES=", fp);
	if(pclass && pclass[0]) {
		(void) fprintf(fp, "%s", pclass[0]);
		for(i=1; pclass[i]; i++)
			(void) fprintf(fp, " %s", pclass[i]);
	}
	if ( class ) {
		for(i=0; class[i]; i++) {
			found = 0;
			if(pclass) {
				for(j=0; pclass[j]; ++j) {
					if(!strcmp(pclass[j], class[i])) {
						found++;
						break;
					}
				}
			}
			if(!found)
				(void) fprintf(fp, " %s", class[i]);
		}
	}
	(void) fputc('\n', fp);

		
	/*
	 * Output all other environment parameters except CLASSES,
	 * SERIALID, SERIALKEY, and SERIALNUM.
	 */
	for(i=0; environ[i]; i++) {
		if (!strncmp(environ[i], "CLASSES=", 8) ||
		    (environ[i][0] == 'S' &&
		     (!strncmp(environ[i], "SERIALID=", 9) ||
		      !strncmp(environ[i], "SERIALKEY=", 10) ||
		      !strncmp(environ[i], "SERIALNUM=", 10)))) {
			continue;
		}
		(void) fputs(environ[i], fp);
		(void) fputc('\n', fp);
	}
	(void) fclose(fp);

	/*
	 * If this is a set installation package, then a setinfo file
	 * should exist in pkgloc.  Copy it to disk.
	 */
	(void) sprintf(path, "%s/%s", instdir, SETINFO);
	(void) sprintf(temp, "%s/%s", pkgloc, SETINFO);
	(void) cppath(0, path, temp, (long) 0);

	/* 
	 * copy all packaging scripts to appropriate directory,
	 * ignoring pkginfo and setinfo files.
	 */
	j = strlen(pkgloc);
	for (i = 0; eptlist[i]; i++) {
		if (eptlist[i]->ftype != 'i' ||
		    (!strcmp(eptlist[i]->path, "pkginfo") ||
		     !strcmp(eptlist[i]->path, "setinfo"))) {
			continue;
		}
		(void)sprintf(path, "%s/install/%s", instdir, eptlist[i]->path);
		(void)sprintf(temp, "%s/%s", pkgbin, eptlist[i]->path);
		/*
		 * Create any intermediate directories, starting at
		 * PKGLOC directory of package.
		 */
		for (pt = &temp[j + 1]; *pt; pt++) {
			if (*pt == '/') {
				*pt = '\0';
				/*
				 * Due to an old bug whereby directories
				 * were treated as files, remove components
				 * of the pathname that are not directories.
				 * Just in case this is an overlay or upgrade.
				 */
				if (stat(temp, &st) ||
				    (!S_ISDIR(st.st_mode) &&
				     unlink(temp) == 0)) {
					if (mkdir(temp, 0755)) {
						progerr(ERR_MKDIR, temp);
						progerr(ERR_SAVE,
							eptlist[i]->path,
							pkgbin);
						quit(99);
					}
				}
				*pt = '/';
			}
		}
		if (cppath(0, path, temp, (long)0)) {
			progerr(ERR_SAVE, eptlist[i]->path, pkgbin);
			quit(99);
		}
	}
}

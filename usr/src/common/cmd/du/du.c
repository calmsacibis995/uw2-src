/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)du:du.c	1.22.1.9"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/du/du.c,v 1.1 91/02/28 16:55:59 ccs Exp $"
/***************************************************************************
 * Command: du
 * Inheritable Privileges: P_MACREAD,P_DACREAD
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

/*
 * du -- summarize disk usage
 *	du [-arskx] [name ...]
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>

char	path[PATH_MAX+1], name[PATH_MAX+1];
int	aflg;
int	rflg;
int	sflg;
int	kflg;
int	xflg;
char	*dot = ".";

#define ML	1000
struct {
	dev_t	dev;
	ino_t	ino;
} ml[ML];
int	mlx = 0;

dev_t	cur_dev;	/* current device used for each filename specified */
long	descend();
static const char errmsg[] = ":37:%s: %s\n";


/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: None
                 pfmt: None
                 strerror: None
                 chdir(2): None
*/

main(argc, argv)
	int argc;
	char **argv;
{
	long blocks = 0;
	register c;
	extern int optind;
	register char *np;
	register pid_t pid, wpid;
	int status, retcode = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:du");

	setbuf(stderr, NULL);
	while ((c = getopt(argc, argv, "arskx")) != EOF)
		switch (c) {

		case 'a':
			aflg++;
			continue;

		case 'r':
			rflg++;
			continue;

		case 's':
			sflg++;
			continue;

		case 'k':
			kflg++;
			continue;

		case 'x':
			xflg++;
			continue;

		default:
			pfmt(stderr, MM_ACTION,
				":910:Usage: du [-arskx] [name ...]\n");
			exit(2);
		}
	if (optind == argc) {
		argv = &dot;
		argc = 1;
		optind = 0;
	}
	do {
		if (optind < argc - 1) {
			pid = fork();
			if (pid == (pid_t)-1) {
				pfmt(stderr, MM_ERROR, ":43:fork() failed: %s\n",
					strerror(errno));
				exit(1);
			}
			if (pid != 0) {
				while ((wpid = wait(&status)) != pid
				    && wpid != (pid_t)-1)
					;
				if (pid != (pid_t)-1 && status != 0)
					retcode = 1;
			}
		}
		if (optind == argc - 1 || pid == 0) {
			(void) strcpy(path, argv[optind]);
			(void) strcpy(name, argv[optind]);
			if (np = strrchr(name, '/')) {
				*np++ = '\0';
				if (chdir(*name ? name : "/") < 0) {
					if (rflg) {
						pfmt(stderr, MM_ERROR, errmsg,
							(*name ? name : "/"),
							strerror(errno));
					}
					exit(1);
				}
			} else
				np = path;
			if (xflg) {
				struct stat stb;
				if (lstat(path, &stb) < 0) {
					if (rflg)
						pfmt(stderr, MM_ERROR, errmsg,
							path, strerror(errno));
					exit(1);
				}
				cur_dev = stb.st_dev;
			}
			blocks = descend(path, *np ? np : ".", &retcode);
			if (sflg)
				printf("%ld\t%s\n", kflg ? (blocks / 2) : 
					blocks, path);
			if (optind < argc - 1)
				exit(0);
		}
		optind++;
	} while (optind < argc);
	exit(retcode);
}





/*
 * Procedure:     descend
 *
 * Restrictions:
                 lstat(2): none
                 pfmt: none
                 strerror: none
                 opendir: none
                 chdir(2): none
*/

DIR	*dirp = NULL;

long
descend(base, name, retcode)
	char *base, *name;
	int  *retcode;
{
	char *ebase0, *ebase;
	struct stat stb;
	int i;
	long blocks = 0;
	long curoff = 0;
	register struct dirent *dp;

	ebase0 = ebase = strchr(base, 0);
	if (ebase > base && ebase[-1] == '/')
		ebase--;
	if (lstat(name, &stb) < 0) {
		if (rflg)
			pfmt(stderr, MM_ERROR, errmsg, name, strerror(errno));
		*retcode = 1; 
		*ebase0 = 0;
		return 0;
	}
	if (xflg) {
		if (stb.st_dev != cur_dev) {
			*retcode = 0; 
			*ebase0 = 0;
			return 0;
		}
	}
	if (stb.st_nlink > 1 && (stb.st_mode & S_IFMT) != S_IFDIR) {
		for (i = 0; i <= mlx; i++)
			if (ml[i].ino == stb.st_ino && ml[i].dev == stb.st_dev)
				return 0;
		if (mlx < ML) {
			ml[mlx].dev = stb.st_dev;
			ml[mlx].ino = stb.st_ino;
			mlx++;
		}
	}
	blocks = stb.st_size/512;
	if ((stb.st_mode & S_IFMT) != S_IFDIR) {
		if (aflg)
			printf("%ld\t%s\n", kflg ? (blocks / 2) : blocks, base);
		return blocks;
	}
	if (dirp != NULL)
		(void) closedir(dirp);
	if ((dirp = opendir(name)) == NULL) {
		if (rflg)
			pfmt(stderr, MM_ERROR, errmsg, name, strerror(errno));
		*retcode = 1;
		*ebase0 = 0;
		return 0;
	}
	if (chdir(name) < 0) {
		if (rflg)
			pfmt(stderr, MM_ERROR, errmsg, name, strerror(errno));
		*retcode = 1;
		*ebase0 = 0;
		(void) closedir(dirp);
		dirp = NULL;
		return 0;
	}
	while (dp = readdir(dirp)) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		(void) sprintf(ebase, "/%s", dp->d_name);
		curoff = telldir(dirp);
		blocks += descend(base, ebase+1, retcode);
		*ebase = 0;
		if (dirp == NULL) {
			if ((dirp = opendir(".")) == NULL) {
				if (rflg) {
					pfmt(stderr, MM_ERROR,
						":182:Cannot reopen '.' in %s: %s\n",
						base, strerror(errno));
				}
				*retcode = 1;
				return 0;
			}
			seekdir(dirp, curoff);
		}
	}
	(void) closedir(dirp);
	dirp = NULL;
	if (sflg == 0)
		printf("%ld\t%s\n", kflg ? (blocks / 2) : blocks, base);
	if (chdir("..") < 0) {
		if (rflg) {
			(void) sprintf(strchr(base, '\0'), "/..");
			pfmt(stderr, MM_ERROR,
				":183:Cannot change back to '..' in %s: %s\n",
				base, strerror(errno));
		}
		exit(1);
	}
	*ebase0 = 0;
	return blocks;
}

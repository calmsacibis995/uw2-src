/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)idmap:uidadmin.c	1.19.3.4"
#ident  "$Header: uidadmin.c 2.0 91/07/12 $"

/*
 * uidadmin is a user interface to the name mapping database.
 *
 * Synopsis
 *
 *	uidadmin [ -S scheme [ -l logname ] ]
 *	uidadmin -S scheme -a -l logname [ -r g_name ]
 *	uidadmin -S scheme -d -l logname [ -r g_name ]
 *	uidadmin -S scheme [ -cf ]
 *
 */


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <pfmt.h>
#include <locale.h>
#include <limits.h>
#include <mac.h>
#include "idmap.h"

#define	OPTIONS		"S:l:r:adcf"

#define	ACT_LIST	0
#define	ACT_ADD		1
#define	ACT_DELETE	2
#define	ACT_CHECK	7
#define	ACT_FIX		8


static	FILE	*logfile;

extern	int	setlabel();
extern	int	fsync();
extern	int	gettimeofday();
extern	void	*malloc();
extern	void	free();
extern	void	qsort();
extern	struct passwd	*getpwnam();
extern	struct passwd	*getpwuid();
extern	struct group	*getgrnam();
extern	uid_t	getuid(), geteuid();
extern	gid_t	getgid(), getegid();
extern	mode_t	umask();
extern	int	lvlin();
extern	int	lvlfile();
extern	int	breakname();


static void
log_cmd(argc, argv)
int	argc;
char	*argv[];
{
	struct timeval	t;
	struct passwd	*pwd;
	struct group	*grp;

	if (access(LOGFILE, F_OK) != 0) {
		(void) close(creat(LOGFILE, LOGFILE_MODE));
		pwd = getpwnam(IDMAP_LOGIN);
		grp = getgrnam(IDMAP_GROUP);
		(void) chown(LOGFILE, pwd->pw_uid, grp->gr_gid);
	}

	if ((logfile = fopen(LOGFILE, "a")) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			    ":58:%s: Cannot open logfile\n", LOGFILE);
		exit(1);
	} else {
		(void) gettimeofday(&t, NULL);
		fprintf(logfile, "%s", ctime(&t.tv_sec));
		while (argc--)
			fprintf(logfile, "%s ", *argv++);
		fprintf(logfile, "\nUID = %d GID = %d SUCC = ",
			(int) getuid(), (int) getgid());
	}
}


static void
log_succ()
{
	fprintf(logfile, "+\n\n");
	(void) fclose(logfile);
}


static void
log_fail()
{
	fprintf(logfile, "-\n\n");
	(void) fclose(logfile);
}


static void
list_scheme(scheme)
char	*scheme;
{
	char	filename[MAXFILE];
	char	descr[MAXLINE];
	char	remote[MAXLINE];
	char	local[MAXLINE];
	FILE	*fp;

	if (strcmp(scheme, ATTRMAP) == 0)
		return;

	/* open uidata file */
	sprintf(filename, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if ((fp = fopen(filename, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		exit(1);
	}

	/* print the record descriptor (skip the !) */
	(void) fgets(descr, MAXLINE, fp);
	printf("%s", descr+1);

	/* print all records */
	while (fscanf(fp, "%s %s\n", remote, local) == 2)
		printf("%s %s\n", remote, local);

	(void) fclose(fp);
}


static void
list_all()
{
	DIR *dirp;
	struct dirent *dp;
	char	filename[MAXFILE];
	struct stat	stat_buf;
	u_short	mode;

	if ((dirp = opendir(MAPDIR)) == NULL)
		return;

	/* skip over . and .. */
	(void) readdir(dirp);
	(void) readdir(dirp);
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ATTRMAP) == 0)
			continue;
		sprintf(filename, "%s/%s/%s", MAPDIR, dp->d_name, UIDATA);
		if (stat(filename, &stat_buf) == 0) {
			mode = stat_buf.st_mode & (~S_IFMT);
			printf("%-16.16s ", dp->d_name);
			if (mode != 0)
				pfmt(stdout, MM_NOSTD, ":112:USER mode\n");
			else
				pfmt(stdout, MM_NOSTD, ":113:SECURE mode\n");
		}
	}
	(void) closedir(dirp);
}


static void
list_logname(scheme, logname)
char	*scheme;
char	*logname;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	descr[MAXLINE];		/* field descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	FILE	*fp;			/* map file stream pointer */

	if (getpwnam(logname) == (struct passwd *) NULL) {
		(void) pfmt(stderr, MM_WARNING, ":76:%s: No such user\n",
			    logname);
	}

	/* open uidata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		exit(1);
	}

	/* get and skip record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		exit(1);
	}

	/* print appropriate records */
	while (fscanf(fp, "%s %s\n", remote, local) == 2) {
		if (strcmp(local, logname) == 0)
			printf("%s %s\n", remote, local);
	}

	(void) fclose(fp);
}


static void
add(scheme, g_name, logname)
char	*scheme;
char	*g_name;
char	*logname;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* temp map file name */
	char	descr[MAXLINE];		/* field descriptor */
	char	descr2[MAXLINE];	/* copy of field descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	remote2[MAXLINE];	/* copy of remote name */
	char	local[MAXLINE];		/* local name */
	char	g_name2[MAXLINE];	/* copy of g_name */
	FIELD	d_fields[MAXFIELDS];	/* descriptor field info */
	FIELD	g_fields[MAXFIELDS];	/* global name field info */
	FIELD	r_fields[MAXFIELDS];	/* remote name field info */
	FILE	*fp, *fp2;		/* map file stream pointers */
	int	passed = 0;		/* insert location flag */
	int	eof = 0;		/* end of file flag */
	int	sr;			/* scanf return code */
	struct passwd	*pwd;
	struct group	*grp;
	int	i;			/* general counter */
	level_t	lvlno;			/* file level number */
	int	tmpfd;			/* temporary fd */

	if (getpwnam(logname) == (struct passwd *) NULL) {
		(void) pfmt(stderr, MM_WARNING, ":76:%s: No such user\n",
			    logname);
	}

	/* open uidata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if ((fp = fopen(mapfile, "r+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* lock it */
	if (lockf(fileno(fp), F_TLOCK, (off_t) 0) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":82:User map file for scheme %s is locked\n",
			    scheme);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}

	/* create temp file */
	(void) strcpy(tmpfile, mapfile);
	(void) strcat(tmpfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, UIDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(UIDATA_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((fp2 = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	/* get record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		(void) fclose(fp2);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	/* copy it */
	(void) fputs(descr, fp2);

	/* find place to insert */
	(void) strcpy(g_name2, g_name);
	if (breakname(g_name2, descr, g_fields) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			   ":63:mandatory field(s) missing\n");
		(void) fclose(fp);
		(void) fclose(fp2);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	(void) strcpy(descr2, descr+1);
	(void) breakname(descr2, descr, d_fields);

	while (!passed && !eof) {
		sr = fscanf(fp, "%s %s\n", remote, local);

		/* check for duplicates */
		if (strcmp(g_name, remote) == 0) {
			(void) fclose(fp);
			(void) fclose(fp2);
			(void) unlink(tmpfile);
			if (strcmp(logname, local) == 0) {

			/* exact duplicate (local and remote names ) */
				(void) pfmt(stderr, MM_WARNING,
					    ":65:entry already in file\n");
				log_succ();
				exit(0);

			} else {

			/* local name was not the same */
				(void) pfmt(stderr, MM_ERROR,
					    ":66:remote name already in file\n");
				log_fail();
				exit(1);
			}
		}

		(void) strcpy(remote2, remote);
		switch (sr) {
		case EOF:
			eof++;
			break;
		case 2:
			(void) breakname(remote2, descr, r_fields);
			if (namecmp(r_fields, g_fields) > 0)
				passed++;
			else
				fprintf(fp2, "%s %s\n", remote, local);
			break;
		default:
			(void) pfmt(stderr, MM_ERROR,
				    ":67:%s: format error in file\n", mapfile);
			(void) fclose(fp);
			(void) fclose(fp2);
			(void) unlink(tmpfile);
			log_fail();
			exit(1);
		}
	}

	/* write new record */
	fprintf(fp2, "%s %s\n", g_name, logname);

	/* copy after insert */
	if (!eof) {
		fprintf(fp2, "%s %s\n", remote, local);
		while (fscanf(fp, "%s %s\n", remote, local) != EOF)
			fprintf(fp2, "%s %s\n", remote, local);
	}

	(void) fclose(fp);
	(void) fflush(fp2);
	(void) fsync((int) fileno(fp2));
	(void) fclose(fp2);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
delete(scheme, g_name, logname)
char	*scheme;
char	*g_name;
char	*logname;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* temp file name */
	char	descr[MAXLINE];		/* field descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	FILE	*fp, *fp2;		/* map file stream pointers */
	struct passwd	*pwd;
	struct group	*grp;
	level_t	lvlno;			/* file level number */
	int	tmpfd;			/* temporary fd */
	int	delete_happened = 0;	/* has a delete happened? */

	/* open data file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if ((fp = fopen(mapfile, "r+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* lock it */
	if (lockf(fileno(fp), F_TLOCK, (off_t) 0) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":82:User map file for scheme %s is locked\n",
			    scheme);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}

	/* create temp file */
	(void) strcpy(tmpfile, mapfile);
	(void) strcat(tmpfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, UIDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(UIDATA_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((fp2 = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	/* get record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		(void) fclose(fp2);
		log_fail();
		exit(1);
	}

	/* copy it */
	(void) fputs(descr, fp2);

	/* copy file while deleting entries */
	while (fscanf(fp, "%s %s\n", remote, local) == 2) {
		if ((strcmp(local, logname) != 0) ||
		    ((g_name != NULL) && (strcmp(remote, g_name) != 0)))
			fprintf(fp2, "%s %s\n", remote, local);
		else {
			printf("%s %s\n", remote, local);
			delete_happened++;
		}
	}

	if (!delete_happened) {
		(void) pfmt(stderr, MM_WARNING, ":69:Entry not found\n");
	}
	(void) fclose(fp);
	(void) fflush(fp2);
	(void) fsync((int) fileno(fp2));
	(void) fclose(fp2);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
check(scheme)
char	*scheme;
{
	char mapfile[MAXFILE];	/* map file name */
	char descr[MAXLINE];	/* field descriptor */
	char mapline[MAXLINE];	/* a line from the map file */
	char prevmapline[MAXLINE];/* previous line */
	FILE *fp;		/* map file stream pointer */
	int numerrors = 0;	/* number of errors found */
	int linenum = 1;	/* line number */
	int sr;			/* scanf return code */
	char remote[MAXLINE];	/* remote name */
	char local[MAXLINE];	/* local name */
	int cr;			/* check return code */

	/* open uidata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		exit(1);
	}

	/* get descriptor and check it */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		exit(1);
	}

	descr[strlen(descr)-1] = '\0';

	if ((*descr != '!') || (check_descr(&descr[1]) < 0)) {
		pfmt(stdout, MM_NOSTD, ":123:Bad descriptor in user map file\n");
		(void) fclose(fp);
		exit(1);
	}

	descr[strlen(descr)] = '\n';

	/* read and check the file */
	while (fgets(mapline, MAXLINE, fp) != NULL) {

		linenum++;

		/*
		 * check syntax errors, missing fields,
		 * duplicate entries, and entries out of order
		 */

		cr = check_entry(descr, (linenum == 2)? NULL:prevmapline, mapline);
		if (cr == 0)
			cr = check_user(mapline, 0);

		if (cr != 0) {

			pfmt(stdout, MM_NOSTD, ":87:Error on line number %d: ", linenum);

			switch(cr) {
			case IE_SYNTAX:
				pfmt(stdout, MM_NOSTD, ":88:Syntax error\n");
				break;
			case IE_MANDATORY:
				pfmt(stdout, MM_NOSTD, ":89:Mandatory field missing\n");
				break;
			case IE_DUPLICATE:
				pfmt(stdout, MM_NOSTD, ":90:Duplicate entry\n");
				break;
			case IE_ORDER:
				pfmt(stdout, MM_NOSTD, ":91:Line out of order\n");
				break;
			case IE_NOUSER:
			case IE_NOFIELD: /* no transparent mappings in uidata */
				pfmt(stdout, MM_NOSTD, ":115:Unknown mapped user\n");
				break;
			}
			numerrors++;
		}

		(void) strncpy(prevmapline, mapline, MAXLINE);
	}
	if (numerrors) {
		if (numerrors == 1)
			pfmt(stdout, MM_NOSTD, ":124:1 error found in user map\n");
		else
			pfmt(stdout, MM_NOSTD, ":125:%d errors found in user map\n", numerrors);
	}
	else
		pfmt(stdout, MM_NOSTD, ":126:No errors in user map\n");
	(void) fclose(fp);
}


typedef struct {
	char	line[MAXLINE];
	char	rem[MAXLINE];
	FIELD	flds[MAXFIELDS];
} MAPTABLE;


static int
mapcompar(name1, name2)
MAPTABLE **name1, **name2;
{
	MAPTABLE *n1 = *name1, *n2 = *name2;

	return(namecmp(n1->flds, n2->flds));
}


static void
fix(scheme)
char	*scheme;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* tmp file name */
	char	descr[MAXLINE];		/* field descriptor */
	char	mapline[MAXLINE];	/* a line from the map file */
	char	prevmapline[MAXLINE];	/* previous line */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	char	response[MAXLINE];	/* a line from the user */
	char	*def;			/* default response */
	char	n1[MAXLINE],		/* name pieces */
		n2[MAXLINE],
		n3[MAXLINE];		/* extra piece */
	FILE	*fp;			/* map file stream pointer */
	FILE	*tmpfp;			/* tmp map file stream pointer */
	int	numerrors = 0;		/* number of errors found */
	int	numfixes = 0;		/* number of fixes made */
	int	linenum = 1;		/* line number */
	int	sr;			/* scanf return code */
	MAPTABLE *maptable;		/* mapping file table */
	MAPTABLE **mapptrs;		/* pointers into mapping table */
	int	entry;			/* entry number */
	int	entries = 0;		/* number of entries in mapping file */
	int	i;
	struct passwd	*pwd;
	struct group	*grp;
	level_t	lvlno;			/* file level number */
	int	cr;			/* check() return code */
	int	tmpfd;			/* temporary fd */

	/* open uidata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if ((fp = fopen(mapfile, "r+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* lock it */
	if (lockf(fileno(fp), F_TLOCK, (off_t) 0) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":82:User map file for scheme %s is locked\n",
			    scheme);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}

	/* create temp file */
	sprintf(tmpfile, "%s%s", mapfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, UIDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(UIDATA_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((tmpfp = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	/* get descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		(void) fclose(tmpfp);
		(void) unlink(tmpfile);
		exit(1);
	}

	descr[strlen(descr)-1] = '\0';

	while ((*descr != '!') || (check_descr(&descr[1]) < 0)) {
		pfmt(stdout, MM_NOSTD, ":127:Bad descriptor in user map file:\n%s\n", descr);
		pfmt(stdout, MM_NOSTD, ":97:Type c to change, a to abort (default c) ");
		(void) fgets(response, sizeof(response), stdin);
		response[strlen(response)-1] = '\0';
		if ((*response == '\0') || (*response == 'c')) {
			pfmt(stdout, MM_NOSTD, ":98:New descriptor (include leading !): ");
			(void) fgets(descr, sizeof(descr), stdin);
			descr[strlen(descr)-1] = '\0';
		} else if (*response == 'a') {
			pfmt(stdout, MM_NOSTD, ":99:Aborting fix...\n");
			(void) fclose(fp);
			(void) fclose(tmpfp);
			(void) unlink(tmpfile);
			exit(1);
		}
	}

	descr[strlen(descr)] = '\n';

	/* copy descriptor */
	(void) fputs(descr, tmpfp);

	/* read and check the file */
	while (fgets(mapline, MAXLINE, fp) != NULL) {

		linenum++;
		cr = check_entry(descr, (linenum == 2)? NULL:prevmapline, mapline);
		if (cr == 0)
			cr = check_user(mapline, 0);

		if (cr != 0) {

			pfmt(stdout, MM_NOSTD, ":87:Error on line number %d: ", linenum);

			switch(cr) {
			case IE_SYNTAX:
				pfmt(stdout, MM_NOSTD, ":100:Syntax error:\n");
				break;
			case IE_MANDATORY:
				pfmt(stdout, MM_NOSTD, ":101:Mandatory field missing:\n");
				break;
			case IE_DUPLICATE:
				pfmt(stdout, MM_NOSTD, ":102:Duplicate entry:\n");
				break;
			case IE_ORDER:
				pfmt(stdout, MM_NOSTD, ":103:Line out of order:\n");
				break;
			case IE_NOUSER:
			case IE_NOFIELD: /* no transparent mappings in uidata */
				pfmt(stdout, MM_NOSTD, ":120:Unknown mapped user:\n");
				break;
			}
			pfmt(stdout, MM_NOSTD|MM_NOGET, "%s", mapline);
			numerrors++;

			pfmt(stdout, MM_NOSTD, ":105:Type c to change, s to skip, d to delete (default c) ");
			(void) fgets(response,sizeof(response),stdin);
			response[strlen(response)-1] = '\0';
			if ((*response == '\0') || (*response == 'c')) {
				pfmt(stdout, MM_NOSTD, ":121:New remote name: ");
				(void) fgets(remote,sizeof(remote),stdin);
				remote[strlen(remote)-1] = '\0';
				pfmt(stdout, MM_NOSTD, ":122:New local name: ");
				(void) fgets(local,sizeof(local),stdin);
				local[strlen(local)-1] = '\0';
				fprintf(tmpfp, "%s %s\n", remote, local);
				entries++;
				pfmt(stdout, MM_NOSTD, ":108:new entry inserted\n");
				numfixes++;
			} else if (*response == 'd') {
				pfmt(stdout, MM_NOSTD, ":109:entry deleted\n");
				numfixes++;
			} else {
				(void) fputs(mapline, tmpfp);
				entries++;
				pfmt(stdout, MM_NOSTD, ":110:entry skipped\n");
			}
		} else {
			(void) fputs(mapline, tmpfp);
			entries++;
		}
		(void) strncpy(prevmapline, mapline, MAXLINE);
	}
	if (numerrors) {
		if (numerrors == 1)
			pfmt(stdout, MM_NOSTD, ":124:1 error found in user map\n", numerrors);
		else
			pfmt(stdout, MM_NOSTD, ":125:%d errors found in user map\n", numerrors);
		pfmt(stdout, MM_NOSTD, ":111:%d of them fixed\n", numfixes);
	} else
		pfmt(stdout, MM_NOSTD, ":126:No errors in user map\n");
	(void) fclose(fp);
	(void) fflush(tmpfp);
	(void) fsync((int) fileno(tmpfp));
	(void) fclose(tmpfp);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	/* check and fix the order of entries in the file */

	/* open uidata file */
	if ((fp = fopen(mapfile, "r+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    mapfile);
		log_fail();
		exit(1);
	}

	/* lock it */
	if (lockf(fileno(fp), F_TLOCK, (off_t) 0) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":82:User map file for scheme %s is locked\n",
			    scheme);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}

	/* get record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}

	/* allocate required space for the mapping file */
	maptable = (MAPTABLE *) malloc(sizeof(MAPTABLE) * entries);
	mapptrs = (MAPTABLE **) malloc(sizeof(MAPTABLE *) * entries);

	/* read map file into table */
	entry = 0;
	while(fgets(maptable[entry].line, MAXLINE, fp) != NULL) {
		(void) sscanf(maptable[entry].line, "%s %s", remote, local);
		(void) strcpy(maptable[entry].rem, remote);
		(void) breakname(maptable[entry].rem, descr, maptable[entry].flds);
		mapptrs[entry] = &maptable[entry];
		entry++;
	}
	(void) fclose(fp);

	/* sort the map table in memory */
	qsort((void *) mapptrs, (unsigned int) entry,
	      sizeof(char *), mapcompar);

	/* write the sorted map back out */
	/* create temp file */
	if ((tmpfd = creat(tmpfile, UIDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(UIDATA_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((tmpfp = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		log_fail();
		exit(1);
	}
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	(void) fputs(descr, tmpfp);

	for (i = 0; i < entry; i++)
		(void) fputs(mapptrs[i]->line, tmpfp);

	free(mapptrs);
	free(maptable);
	(void) fflush(tmpfp);
	(void) fsync((int) fileno(tmpfp));
	(void) fclose(tmpfp);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
usage(prog, file)
char	*prog;
FILE	*file;
{
	pfmt(file, MM_ERROR, ":73:Syntax\n");
	pfmt(file, MM_ACTION, ":83:%s: Usage:\n\t%s [ -S scheme [ -l logname ] ]\n\t%s -S scheme -a [ -l logname ] -r g_name\n\t%s -S scheme -d -l logname [ -r g_name ]\n\t%s -S scheme [ -cf ]\n", prog, prog, prog, prog, prog);
}


main(argc, argv)
int	argc;
char	*argv[];
{
	extern int	getopt();
	int		c;
	extern char	*optarg;
	int		opterror = 0;	/* options/usage error */
	int		autherror = 0;	/* unauthorized user error */
	int		action = ACT_LIST;
	char		*scheme, *logname, *g_name;
	int		sysadm = 0;	/* user == administrator flag */
	int		theuser = 0;	/* user is same as local name */
	struct passwd	*pwd;
	char		username[LOGNAME_MAX+1];
	int		effinlist = 0;	/* egid in list of supp groups flag */
	gid_t		grouplist[NGROUPS_MAX];
	gid_t		effgr;		/* effective group number */
	int		ng;		/* number of supplementary groups */
	char		mapfile[MAXFILE]; /* file name buffer */
	struct stat	statbuf;	/* buffer for stat() */
	char		elogname[LOGNAME_MAX+1]; /* logname of euid */

	(void) umask(0000);

/* set up error message handling */

	(void) setlocale(LC_MESSAGES, "");
	(void) setlabel("UX:uidadmin");
	(void) setcat("uxnsu");

/* parse command line */

	scheme = NULL;
	logname = NULL;
	g_name = NULL;

	while ((c = getopt(argc, argv, OPTIONS)) != EOF)
		switch(c) {
		case 'S':
			scheme = optarg;
			/* check if the scheme is SECURE */
			(void) sprintf(mapfile, "%s/%s/%s",
				       MAPDIR, scheme, UIDATA);
			if (stat(mapfile, &statbuf) >= 0)
			    if ((statbuf.st_mode & 0777) == 0) {
				(void) pfmt(stderr, MM_ERROR,
					    ":84:scheme %s is in SECURE mode\n",
					    scheme);
				exit(1);
			}
			break;
		case 'l':
			logname = optarg;
			if ((pwd = getpwnam(logname)) != NULL) {
			  theuser = (pwd->pw_uid == geteuid());
			}
			break;
		case 'r':
			g_name = optarg;
			break;
		case 'a':
			if (action)
				opterror++;
			else {
				action = ACT_ADD;
			}
			break;
		case 'd':
			if (action)
				opterror++;
			else {
				action = ACT_DELETE;
			}
			break;
		case 'c':
			if (action)
				opterror++;
			else {
				action = ACT_CHECK;
			}
			break;
		case 'f':
			if (action)
				opterror++;
			else {
				action = ACT_FIX;
			}
			break;
		case '?':
			opterror++;
			break;
		}

	if (opterror) {
		usage(argv[0], stderr);
		exit(1);
	}

	/* log command line (initial) */
	if ((action != ACT_LIST) && (action != ACT_CHECK))
		log_cmd(argc, argv);

	/* is the user the administrator */
	effgr = getegid();
	ng = getgroups(NGROUPS_MAX, grouplist);
	while(ng > 0) {
		if (effgr == grouplist[ng-1])
			effinlist++;
		ng--;
	}
	sysadm = ((getgid() == getegid()) || (effinlist));

	switch(action) {
	case ACT_LIST:
		if (g_name != NULL)
			opterror++;
		else {
			if (scheme != NULL) {
				if (logname != NULL)
					if (theuser || sysadm)
						list_logname(scheme, logname);
					else
						autherror++;
				else
					if (sysadm)
						list_scheme(scheme);
					else {
						if ((pwd = getpwuid(geteuid()))
							!= (struct passwd *) NULL) {
							strcpy(elogname, pwd->pw_name);
							list_logname(scheme, elogname);
						}
					}
			} else {
				list_all();
			}
		}
		break;

	case ACT_ADD:
		if ((scheme == NULL) || (g_name == NULL))
			opterror++;
		else
			if ((logname == NULL) || theuser || sysadm) {
				if (logname == NULL) {
					pwd = getpwuid(geteuid());
					strncpy(username, pwd->pw_name, LOGNAME_MAX+1);
					logname = username;
				}
				add(scheme, g_name, logname);
			} else
				autherror++;
		break;

	case ACT_DELETE:
		if ((scheme == NULL) || (logname == NULL))
			opterror++;
		else
			if (theuser || sysadm)
				delete(scheme, g_name, logname);
			else
				autherror++;
		break;

	case ACT_CHECK:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else
			if (sysadm)
				check(scheme);
			else
				autherror++;
		break;

	case ACT_FIX:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else
			if (sysadm)
				fix(scheme);
			else
				autherror++;
		break;
	}

	if (opterror) {
		usage(argv[0], stderr);
		log_fail();
		exit(1);
	}

	if (autherror) {
		(void) pfmt(stderr, MM_ERROR,
			    ":85:%s: Not allowed to perform this operation\n",
			    argv[0]);
		log_fail();
		exit(1);
	}

	return(0);
}

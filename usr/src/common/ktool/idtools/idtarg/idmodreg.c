/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idmodreg.c	1.16"
#ident	"$Header:"

#include "inst.h"
#include "defines.h"
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mod.h>
#include <sys/modexec.h>
#include <unistd.h>
#include <errno.h>
#include <varargs.h>
#include <string.h>
#include <locale.h>
#include <pfmt.h>

#define USAGE	":117:Usage:\tidmodreg [-r root] [-f mreglist] [[-M module] ...] [-c conf_dir] [-#]\n"
#define	SYNTAX	"%d:%d:%[^:]:%s"
#define ENTLEN 128

int mflag = 0;
int rflag = 0;
int fflag = 0;
int cflag = 0;
int debug = 0;

char *gfile = "mod_reg";
char *rfile = "mod_register";
char regfile[LINESZ] = "/etc/conf/mod_register";

char current[LINESZ];
char root[LINESZ];

int errcnt = 0;

struct modlist *modlist;
void runcmd();
void clean_up();

main(argc, argv)
int argc;
char **argv;
{
	int m;
	struct modlist *mod;
	FILE *fp;
	char confdir[LINESZ];

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idmodreg");

	while ((m = getopt(argc, argv, "?M:c:r:f:#")) != EOF)
		switch(m) {

		case 'M':
			mflag++;
			mod = (struct modlist *)malloc(sizeof(struct modlist));
			strcpy(mod->name, optarg);
			mod->next = modlist;
			modlist = mod;
			break;

		case 'r':
			rflag++;
			strcpy(root, optarg);
			break;

		case 'f':
			fflag++;
			strcpy(regfile, optarg);
			break;
			
		case 'c':
			cflag++;
			strcpy(confdir, optarg);
			break;
			
		case '#':
			debug++;
			break;

		case '?':
			pfmt(stderr, MM_ERROR, USAGE);
			exit(1);
		}

	if (!fflag && cflag)
		sprintf(regfile, "%s/%s", confdir, rfile);
	if (debug)
		printf("regfile=%s\n", regfile);

	if (mflag)
		mod_reg();
	else {
		if (!fflag && access(regfile, F_OK) < 0)
			exit(0);
		if ((fp = fopen(regfile, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, FOPEN, regfile, "r");
			exit(1);
		}
		proc_regfile(fp);
		fclose(fp);
	}
	if (errcnt)
		exit(1);
	exit(0);
}

getpath(flag, buf, def)
int flag;
char *buf, *def;
{
	if (flag) {
		if (chdir(buf) != 0) {
			pfmt(stderr, MM_ERROR, EXIST, buf);
			exit(1);
		}
		getcwd(buf, LINESZ);
		chdir(current);
	} else
		strcpy(buf, def);
}


struct		mod_exec_tdata	metd;
ushort_t	magic_buf[MAX_MAGIC];

void *
set_exec_typedata(typed)
char *typed;
{
	int	i;
	char	*cp;
	ushort_t	*mp;

	if ((cp = strtok(typed, ",")) == NULL) {
		return (NULL);
	}
	metd.met_order = atoi(cp);

	if ((cp = strtok(NULL, ",")) == NULL) {
		return (NULL);
	}
	if (*cp != 'Y' && *cp != 'N' || *(cp+1) != '\0') {
		return (NULL);
	}
	metd.met_wildcard = (*cp == 'Y' ? 1 : 0);

	i = 0;
	mp = magic_buf;

	while((cp = strtok(NULL, ",\n")) != NULL) {
		if (mp >= &magic_buf[MAX_MAGIC]) {
			pfmt(stderr, MM_ERROR, ":119:Too many magic numbers.\n");
			return (NULL);
		}
		*mp++ = (short)atoi(cp);
		i++;
	}
	if (i == 0 && !metd.met_wildcard) {
		return (NULL);
	}
	metd.met_nmagic = i;
	metd.met_magic = magic_buf;

	return (&metd);
}

proc_regfile(fp)
FILE *fp;
{
	int nfield;
	int major;
	unsigned int type;
	unsigned int cmd;
	char buf[ENTLEN], typed[512];
	struct mod_mreg mreg;
	char errmsg[80];

	while (fgets(buf, sizeof(buf), fp) != NULL) {

		if (buf[0] == '#' || buf[0] == '*')
			continue;
		nfield = sscanf(buf, SYNTAX, &type, &cmd, mreg.md_modname, 
			typed);
		if (nfield != 4) {
			fprintf(stderr, "%s\n", buf);
			pfmt(stderr, MM_ERROR, ":120:number of fields is incorrect\n");
			errcnt++;
			continue;
		}

		if (type == MOD_TY_CDEV || type == MOD_TY_BDEV ||
		    type == MOD_TY_SDEV) {
			major = atoi(typed);
			mreg.md_typedata = (void *)major;
		}
		else if (type == MOD_TY_EXEC) {
			if ((mreg.md_typedata = set_exec_typedata(typed)) == NULL) {
				fprintf(stderr, "%s\n", buf);
				pfmt(stderr, MM_ERROR, ":121:exec type data is incorrect\n");
				errcnt++;
				continue;
			}
		}
		else
			mreg.md_typedata = (void *)typed;

		if (modadm(type, cmd, (void *)&mreg) < 0) {
			pfmt(stderr, MM_ERROR,
				":268:cannot register module '%s': %s\n",
				mreg.md_modname,
				strerror(errno));
			errcnt++;
		}
	}
}

mod_reg()
{
	char moddir[LINESZ], outfile[LINESZ];
	struct modlist *mod;
	FILE *fp;

	getcwd(current, sizeof(current));
	getpath(rflag, root, ROOT);

	if (debug)
		fprintf(stderr, "Root: %s\n", root);

	sprintf(outfile, "%s/%s/%s", root, CFDIR, gfile);

	if (access(regfile, F_OK) == 0)
		clean_up(regfile, outfile);

	for (mod = modlist; mod != NULL; mod = mod->next) {
		struct stat statb;

		sprintf(moddir, "%s/%s/%s", root, PKDIR, mod->name);
		if (chdir(moddir) != 0) {
			pfmt(stderr, MM_ERROR, EXIST, moddir);
			exit(1);
		}

		if (stat(gfile, &statb) < 0) {
			pfmt(stderr, MM_ERROR, ":118:cannot find %s for %s, need to be preconfiged by idbuild\n",
				gfile, mod->name);
			exit(1);
		}

		if ((fp = fopen(gfile, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, FOPEN, gfile, "r");
			exit(1);
		}

		proc_regfile(fp);

		fclose(fp);
		runcmd("cat %s >> %s", gfile, outfile);
	}
	chdir(current);
	if (access(outfile, F_OK) == 0)
		runcmd("mv %s %s", outfile, regfile);
}

/*
 * This routine takes a variable number of arguments to pass them to 
 * the "system" library routine.
 */

void 
runcmd(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	char buf[LINESZ];

	va_start(args);
	fmt = va_arg(args, char *);
	va_end(args);
	vsprintf(buf, fmt, args);
	system(buf);
	return;
}

void
clean_up(oldf, newf)
char *oldf, *newf;
{
	FILE *ofp, *nfp;
	struct modlist *mod;
	char buf[ENTLEN], typed[20];
	char modname[NAMESZ];
	unsigned int type, cmd;
	int nfield;

	if ((ofp = fopen(oldf, "r")) == NULL) {
		perror(oldf);
		exit(1);
	}
	if ((nfp = fopen(newf, "w")) == NULL) {
		perror(newf);
	}

	while (fgets(buf, 40, ofp) != NULL) {
		if (buf[0] == '#') {
			fputs(buf, nfp);
			continue;
		}
		nfield = sscanf(buf, SYNTAX, &type, &cmd, &modname, &typed);
		if (nfield != 4) {
			fprintf(stderr, "%s\n", buf);
			pfmt(stderr, MM_ERROR, ":122:number of fields is incorrect, ignored.\n");
			continue;
		}

		for (mod = modlist; mod != NULL; mod = mod->next)
			if (strcmp(mod->name, modname) == 0)
				break;

		if (mod == NULL)
			fputs(buf, nfp);
	}

	fclose(ofp);
	fclose(nfp);
}

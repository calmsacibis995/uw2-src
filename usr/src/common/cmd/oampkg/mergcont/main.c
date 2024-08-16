/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/mergcont/main.c	1.1"
#ident  "$Header: $"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>
#include <pkglocs.h>
#include <pkgstrct.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * The USER_PUBLIC level.
 */
#define USER_PUB 4

static char *instdir;
static char buf[PATH_MAX];
static char contents[PATH_MAX];
static char t_contents[PATH_MAX];
char *prog;

void usage();
void trap(int);
int cont_merg(FILE *, FILE *, FILE *);
void pinfo_merg(struct cfent *, struct cfent *);

/*
 * This program merges an incoming contents file with an already
 * existing one.
 *
 * It's meant to be executed at boot install of an OVERLAY or
 * UPGRADE.
 */
int
main(int argc, char **argv)
{
	int c;
	int n, k;
	FILE *inpfp;
	FILE *mapfp;
	FILE *tmpfp;
	time_t clock;
	level_t lvl;
	struct stat st;
	struct cfent entry;
	struct cfent entry1;
	void (*func)();

	prog = strrchr(argv[0], '/');
	if (prog++ == NULL) {
		prog = argv[0];
	}
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:mergcont");

	instdir = PKGADM;
	while ((c = getopt(argc, argv, "d:")) != EOF) {
		switch (c) {
			case 'd':
				instdir = optarg;
				break;
			case '?':
				usage();
		}
	}

	if (argc - optind != 1) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		usage();
	}

	if ((inpfp = fopen(argv[optind], "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
			argv[1], strerror(errno));
		exit(1);
	}
	/* 
	 * We open the file for update because we don't want any other
	 * process either reading or writing the contents file while we're
	 * working with it.
	 */
	(void)sprintf(contents, "%s/contents", instdir);

	if ((mapfp = fopen(contents, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
			contents, strerror(errno));
		exit(1);
	}

	/*
	 * Retain the current attributes for the contents file to be used for
	 * resettting after we're done changing it.  This is done so the pkgchk
	 * command won't complain about the owner and uid associated with the
	 * contents files changing.
	 */
	if (stat(contents, &st)) {
		pfmt(stderr, MM_ERROR, ":602:%s failed: %s\n",
			"stat()", strerror(errno));
		exit(1);
	}

	(void)sprintf(t_contents, "%s/t.contents", instdir);
	if ((tmpfp = fopen(t_contents, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
			t_contents, strerror(errno));
		exit(1);
	}
	func = signal(SIGINT, trap);
	if (func != SIG_DFL) {
		(void)signal(SIGINT, func);
	}

	if (cont_merg(mapfp, tmpfp, inpfp) != 0) {
		(void)fclose(tmpfp);
		(void)unlink(t_contents);
		exit(1);
	}
	/*
	 * Need to modify file.
	 */
	(void)time(&clock);
	(void)pfmt(tmpfp, MM_NOSTD|MM_NOGET,
		   "# Last modified by %s as part of OVERLAY/UPGRADE operation\n# %s",
		    prog, ctime(&clock));
	if (rename(t_contents, contents)) {
		pfmt(stderr, MM_ERROR, ":339:Cannot rename %s to %s: %s\n",
			t_contents, contents, strerror(errno));
		exit(1);
	}
	/*
	 * Set level on contents.
	 */
	lvl = USER_PUB;
	(void)setlvl(contents, &lvl);

	/*
	 * Reset contents file attributes to original attributes.
	 */ 
	(void)chmod(contents, st.st_mode);
	(void)chown(contents, st.st_uid, st.st_gid);
	exit(0);
}

void
usage()
{
	(void)sprintf(buf, "%s [-d dirname] pathname", prog);
	pfmt(stderr, MM_ACTION, ":527:Usage: %s\n", buf);
	exit(2);
}

void
trap(int signo)
{
	(void)unlink(t_contents);
	exit(3);
}

int
cont_merg(FILE *mapfp, FILE *tmpfp, FILE *inpfp)
{
	int k;
	int n;
	struct cfent *inp;
	struct cfent entry;
	struct cfent entry1;
	char inpfp_eof = 0;

	inp = &entry1;
	entry.path = NULL;
	entry.pinfo = NULL;
	inp->path = NULL;
	inp->pinfo = NULL;
	for(;;) {
		/*
		 * Get the next incoming contents file entry.
		 */
		if (!inpfp_eof && (k = srchcfile(inp, "*", inpfp, NULL))) {
			switch (k) {
				case 0:
					inpfp_eof++;
					inp = NULL;
					break;
				case -1:
					/*
					 * This entry is invalid.
					 */
					return -1;
			 	default:
					/*
					 * srchcfile() uses a static local
					 * buffer to store both inp->path
					 * and inp->ainfo.local.  So let's
					 * make a copy to avoid overwritting
					 * during the next call to it below
					 * for a match entry.
					 */
					inp->path = strdup(inp->path);
					inp->ainfo.local = strdup(inp->ainfo.local);
			}
		}

		/*
		 * Get the next entry from the installed contents file.
		 */
		n = srchcfile(&entry, inp ? inp->path : NULL, mapfp, tmpfp);
		if (n < 0) {
			return(-1);
		} else if (n == 0) {
			break; /* EOF */
		} else if (k == 1) {
			/*
			 * The pathname matches an entry in the old
			 * (installed) contents file.  Merge the
			 * referenced packages with the new contents
			 * file entry.
			 */
			pinfo_merg(inp, &entry);
		}
		if (!strchr("sl", inp->ftype)) {
			(void)free(inp->ainfo.local);
			inp->ainfo.local = NULL;
		}
		if (putcfile(inp, tmpfp)) {
			return -1;
		}
		if (inp) {
			(void)free(inp->path);
			if (inp->ainfo.local) {
				(void)free(inp->ainfo.local);
			}
		}
	}

	while (!inpfp_eof && (k = srchcfile(inp, "*", inpfp, NULL)) != 0) {
		if (k < 0) {
			return -1;
		}
		if (!strchr("sl", inp->ftype)) {
			inp->ainfo.local = NULL;
		}
		if (putcfile(inp, tmpfp)) {
			return -1;
		}
	}

	return 0;
}

/*
 * void pinfo_merg(new content file entry, old content file entry)
 *
 * This routine merges the package information structure of the
 * old (installed) contents file entry with the incoming content
 * file entry.
 */
void
pinfo_merg(struct cfent *ninp, struct cfent *oinp)
{
	int found = 0;
	struct pinfo *last, *prev_op;
	register struct pinfo *op, *np;

	if (strcmp(ninp->class, oinp->class)) {
		/*
		 * The incoming entry class is different.  Let's
		 * attach the default class to the old package list.
		 */
		for (op = oinp->pinfo; op; op = op->next) {
			if (op->aclass[0] == '\0') {
				strcpy(op->aclass, oinp->class);
			}
		}
	}
	for (op = oinp->pinfo; op;) {
		prev_op = op;
		for (np = ninp->pinfo; np; np = np->next) {
			last = np;
			if (!strcmp(op->pkg, np->pkg)) {
				found++;
				break;
			}
		}
		op = prev_op->next;
		if (found) {
			found = 0;
			(void)free(prev_op);
			continue;
		}
		prev_op->next = NULL;
		last->next = prev_op;
	}
	oinp->pinfo = NULL;
}

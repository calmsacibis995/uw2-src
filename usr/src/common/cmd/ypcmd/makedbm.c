/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ypcmd:makedbm.c	1.4.11.5"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#undef NULL

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <signal.h>

#include "ypdefs.h"
#include "ypsym.h"
USE_YP_MASTER_NAME
USE_YP_LAST_MODIFIED
USE_YP_INPUT_FILE
USE_YP_OUTPUT_NAME
USE_YP_DOMAIN_NAME
USE_YP_SECURE
USE_DBM

#define MAXLINE 4096		/* max length of input line */
static char *get_date();
static char *any();

static void addpair();
static void unmake();
static void usage();

extern char *gettxt();

main(argc, argv)
	char **argv;
{
	FILE *infp;
	datum key, content, tmp;
	char buf[MAXLINE];
	char pagbuf[MAXPATHLEN];
	char tmppagbuf[MAXPATHLEN];
	char dirbuf[MAXPATHLEN];
	char tmpdirbuf[MAXPATHLEN];
	char *p,ic;
	char *infile, *outfile;
	char outalias[MAXPATHLEN];
	char outaliasmap[MAXNAMLEN];
	char outaliasdomain[MAXNAMLEN];
	char *last_slash, *next_to_last_slash;
	char *infilename, *outfilename, *mastername, *domainname,
	    *security, *lower_case_keys;
	char local_host[MAX_MASTER_NAME];
	int cnt,i;
	static int sendmail = 0;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxmakedbm");
        (void)setlabel("UX:makedbm");
 
	infile = outfile = NULL; /* where to get files */
	/* name to imbed in database */
	infilename = outfilename = mastername = domainname = 
	    security = lower_case_keys = NULL; 
	argv++;
	argc--;
	while (argc > 0) {
		if (argv[0][0] == '-' && argv[0][1]) {
			switch(argv[0][1]) {
				case 'i':
					infilename = argv[1];
					argv++;
					argc--;
					break;
				case 'o':
					outfilename = argv[1];
					argv++;
					argc--;
					break;
				case 'm':
					mastername = argv[1];
					argv++;
					argc--;
					break;
				case 'd':
					domainname = argv[1];
					argv++;
					argc--;
					break;
				case 'l':
					lower_case_keys = argv[0];
					break;
				case 's':
					security = argv[0];
					break;
				case 'u':
					unmake(argv[1]);
					argv++;
					argc--;
					exit(0);
				case 'M':
					sendmail++;
					break;
				default:
					usage();
			}
		}
		else if (infile == NULL)
			infile = argv[0];
		else if (outfile == NULL)
			outfile = argv[0];
		else
			usage();
		argv++;
		argc--;
	}
	if (infile == NULL || outfile == NULL)
		usage();
	if (strcmp(infile, "-") != 0)
		infp = fopen(infile, "r");
	else
		infp = stdin;
	if (infp == NULL) {
		pfmt(stderr, MM_STD, ":1:can't open %s\n", infile);
		exit(1);
	}

	/*
	 *  do alias mapping if necessary
	 */
	last_slash = strrchr(outfile,'/');
	if (last_slash){
		 *last_slash='\0';
		 next_to_last_slash= strrchr(outfile,'/');
		if (next_to_last_slash) *next_to_last_slash='\0';
	}
	else next_to_last_slash = NULL;

#ifdef DEBUG
	if (last_slash) pfmt(stdout, MM_NOSTD, ":2:last_slash=%s\n",
			last_slash+1);
	if (next_to_last_slash) pfmt(stdout, MM_NOSTD,
		":3:next_to_last_slash=%s\n", next_to_last_slash+1);
#endif DEBUG

	/* reads in alias file for system v filename translation */
	sysvconfig();

	if (last_slash && next_to_last_slash) {
		if (yp_getalias(last_slash+1, outaliasmap, MAXALIASLEN) < 0) {
			if ((int)strlen(last_slash+1) <= MAXALIASLEN)
				strcpy(outaliasmap, last_slash+1);
			else
				pfmt(stderr, MM_STD | MM_WARNING,
				     ":4:no alias for %s\n",
				    	last_slash+1);
		}
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":5:%s\n",last_slash+1);
		pfmt(stdout, MM_NOSTD, ":5:%s\n",outaliasmap);
#endif DEBUG
		if (yp_getalias(next_to_last_slash+1, outaliasdomain, 
		    MAXALIASLEN) < 0) {
			if ((int)strlen(last_slash+1) <= MAXALIASLEN)
				strcpy(outaliasdomain, next_to_last_slash+1);
			else
				pfmt(stderr, MM_STD | MM_WARNING,
				    ":4:no alias for %s\n",
				    next_to_last_slash+1);
		}
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":5:%s\n",next_to_last_slash+1);
		pfmt(stdout, MM_NOSTD, ":5:%s\n",outaliasdomain);
#endif DEBUG
		sprintf(outalias, gettxt(":6", "%s/%s/%s"), outfile, outaliasdomain,
			outaliasmap);
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":7:outalias=%s\n",outalias);
#endif DEBUG

	} else if (last_slash) {
		if (yp_getalias(last_slash+1, outaliasmap, MAXALIASLEN) < 0){
			if ((int)strlen(last_slash+1) <= MAXALIASLEN)
				strcpy(outaliasmap, last_slash+1);
			else {
				pfmt(stderr, MM_STD | MM_WARNING, ":4:no alias for %s\n",
				    last_slash+1);
			}
		}
		if (yp_getalias(outfile, outaliasdomain, MAXALIASLEN) < 0){
			if ((int)strlen(outfile) <= MAXALIASLEN)
				strcpy(outaliasmap, outfile);
			else
				pfmt(stderr, MM_STD | MM_WARNING, ":4:no alias for %s\n",
				    last_slash+1);
		}	
		sprintf(outalias,gettxt(":8", "%s/%s"), outaliasdomain, outaliasmap);
	} else {
		if (yp_getalias(outfile, outalias, MAXALIASLEN) < 0){
			if ((int)strlen(last_slash+1) <= MAXALIASLEN)
				strcpy(outalias, outfile);
			else 
				pfmt(stderr, MM_STD | MM_WARNING, ":4:no alias for %s\n",
				    outfile);
			}
	}
#ifdef DEBUG
	pfmt(stderr, MM_NOSTD, ":9:outalias=%s\n",outalias);
	pfmt(stderr, MM_NOSTD, ":10:outfile=%s\n",outfile);
#endif DEBUG

	strcpy(tmppagbuf, outalias);
	strcat(tmppagbuf, ".t");
	strcpy(tmpdirbuf, tmppagbuf);
	strcat(tmpdirbuf, dbm_dir);
	strcat(tmppagbuf, dbm_pag);
	if (fopen(tmpdirbuf, "w") == NULL) {
	    	pfmt(stderr, MM_STD, ":11:can't create %s\n", tmpdirbuf);
		exit(1);
	}
	if (fopen(tmppagbuf, "w") == NULL) {
	    	pfmt(stderr, MM_STD, ":11:can't create %s\n", tmppagbuf);
		exit(1);
	}
	strcpy(dirbuf, outalias);
	strcat(dirbuf, ".t");
	if (dbminit(dirbuf) != 0) {
		pfmt(stderr, MM_STD, ":12:can't init %s\n", dirbuf);
		exit(1);
	}
	strcpy(dirbuf, outalias);
	strcpy(pagbuf, outalias);
	strcat(dirbuf, dbm_dir);
	strcat(pagbuf, dbm_pag);

	/*
	 * there seems to be bug with the way
	 * the shell sets up pipes. The SIGCLD is received
	 * when the process in front of the pipe dies. This
	 * in turn cause the fgets to fail.
	 */
	sigset(SIGCLD, SIG_IGN);

	while (fgets(buf, sizeof(buf), infp) != NULL) {
		p = buf;
		cnt = strlen(buf) - 1; /* erase trailing newline */
		while (p[cnt-1] == '\\') {
			p+=cnt-1;
			if (fgets(p, sizeof(buf)-(p-buf), infp) == NULL)
				goto breakout;
			cnt = strlen(p) - 1;
		}
		if (cnt == 0) { /* empty line */
		    continue;
		}
		p = any(buf, " \t\n");
		key.dptr = buf;
		key.dsize = p - buf;
		if (sendmail) {
		    *p = '\0'; p++;
		    key.dsize++;
		}
		for (;;) {
			if (p == NULL || *p == NULL) {
				pfmt(stderr, MM_STD, ":13:yikes (invalid input line)!\n");
				exit(1);
			}
			if (*p != ' ' && *p != '\t')
				break;
			p++;
		}
		content.dptr = p;
		content.dsize = strlen(p) - 1; /* erase trailing newline */
		if (sendmail) {
		    content.dptr[content.dsize] = '\0';
		    content.dsize++;
		}
		if (lower_case_keys) 
			for (i=0; i<key.dsize; i++) {
				ic = *(key.dptr+i);
				if (isascii(ic) && isupper(ic)) 
					*(key.dptr+i) = tolower(ic);
			} 
		tmp = fetch(key);
		if (tmp.dptr == NULL) {
			if (store(key, content) != 0) {
				pfmt(stdout, MM_NOSTD, ":14:problem storing %.*s %.*s\n",
				    key.dsize, key.dptr,
				    content.dsize, content.dptr);
				exit(1);
			}
		}
#ifdef DEBUG
		else {
			pfmt(stdout, MM_NOSTD, ":15:duplicate: %.*s %.*s\n",
			    key.dsize, key.dptr, content.dsize, content.dptr);
		}
#endif
	}
   breakout:
	addpair(yp_last_modified, get_date(infile));
	if (infilename)
		addpair(yp_input_file, infilename);
	if (outfilename)
		addpair(yp_output_file, outfilename);
	if (domainname)
		addpair(yp_domain_name, domainname);
	if (security)
		addpair(yp_secure, "");
	if (!mastername) {
		gethostname(local_host, sizeof (local_host) - 1);
		mastername = local_host;
	}
	addpair(yp_master_name, mastername);

	if (rename(tmppagbuf, pagbuf) < 0) {
		(void)unlink(tmppagbuf);
		pfmt(stderr, MM_STD, ":16:rename failed: %s\n",
			strerror(errno));
	}
	if (rename(tmpdirbuf, dirbuf) < 0) {
		(void)unlink(tmpdirbuf);
		pfmt(stderr, MM_STD, ":16:rename failed: %s\n",
			strerror(errno));
	}
	return(0);
}


/* 
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

static char *
get_date(name)
	char *name;
{
	struct stat filestat;
	static char ans[MAX_ASCII_ORDER_NUMBER_LENGTH];/* ASCII numeric string*/

	if (strcmp(name, "-") == 0)
		sprintf(ans, gettxt(":17","%010d"), (long) time(0));
	else {
		if (stat(name, &filestat) < 0) {
			pfmt(stderr, MM_STD, ":18:can't stat %s\n", name);
			exit(1);
		}
		sprintf(ans, gettxt(":17","%010d"), (long) filestat.st_mtime);
	}
	return ans;
}

void
usage()
{
	pfmt(stderr, MM_STD,
":19:usage: makedbm -u file\n       makedbm [-s] [-i YP_INPUT_FILE] [-o YP_OUTPUT_FILE] [-d YP_DOMAIN_NAME] [-m YP_MASTER_NAME] infile outfile\n");
	exit(1);
}

void
addpair(str1, str2)
	char *str1, *str2;
{
	datum key;
	datum content;
	
	key.dptr = str1;
	key.dsize = strlen(str1);
	content.dptr  = str2;
	content.dsize = strlen(str2);
	if (store(key, content) != 0){
		pfmt(stdout, MM_NOSTD, ":14:problem storing %.*s %.*s\n",
		    key.dsize, key.dptr, content.dsize, content.dptr);
		exit(1);
	}
}

void
unmake(file)
	char *file;
{
	datum key, content;

	if (file == NULL)
		usage();
	
	if (dbminit(file) != 0) {
		pfmt(stderr, MM_STD, ":20:couldn't init %s\n", file);
		exit(1);
	}
	for (key = firstkey(); key.dptr != NULL; key = nextkey(key)) {
		content = fetch(key);
		pfmt(stdout, MM_NOSTD, ":21:%.*s %.*s\n", key.dsize, key.dptr,
		    content.dsize, content.dptr);
	}
}

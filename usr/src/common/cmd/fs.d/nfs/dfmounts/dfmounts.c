/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:dfmounts/dfmounts.c	1.5"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
 
*/
/*
 * nfs dfmounts
 */
#include <stdio.h>
#include <varargs.h>
#include <string.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <locale.h>
#include <pfmt.h>

/*
 * exit values
 */
#define RET_OK		0
#define RET_ERR		33	/* usage error */
#define RET_RPCERR	34	/* rpc error */
#define RET_HNAME	36	/* gethostname (uname) failed */

struct timeval TIMEOUT = { 25, 0 };

int hflg;
void pr_mounts();
void freemntlist();
int sortpath();
void usage();

main(argc, argv)
	int argc;
	char **argv;
{
	
	char hostbuf[256];
	extern int optind;
	int i, c;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfs dfmounts");

	while ((c = getopt(argc, argv, "h")) != EOF) {
		switch (c) {
		case 'h':
			hflg++;
			break;
		default:
			usage();
			exit(RET_ERR);
		}
	}

	if (optind < argc) {
		for (i = optind ; i < argc ; i++)
			pr_mounts(argv[i]);
	} else {
		if (gethostname(hostbuf, sizeof(hostbuf)) < 0) {
			pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
			     "gethostname", strerror(errno));
			exit(RET_HNAME);
		}
		pr_mounts(hostbuf);
	}

	exit(RET_OK);
}

#define	NTABLEENTRIES	2048
struct mountlist *table[NTABLEENTRIES];

void
pr_mounts(host)
	char *host;
{
	CLIENT *cl;
	struct mountlist *ml = NULL;
	struct mountlist **tb, **endtb;
	enum clnt_stat err;
	char *last;
	int tail = 0;

	/*
	 * First try circuit, then drop back to datagram if
	 * circuit is unavailable (an old version of mountd perhaps)
	 * Using circuit is preferred because it can handle
	 * arbitrarily long export lists.
	 */
	cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "circuit_n");
	if (cl == NULL) {
		cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "datagram_n");
		if (cl == NULL) {
			pfmt(stderr, MM_ERROR,
			     ":47:%s: server not responding for %s\n",
			     host, clnt_spcreateerror("clnt_create"));
			exit(RET_RPCERR);
		}
	}

	if (err = clnt_call(cl, MOUNTPROC_DUMP, xdr_void, 0, 
			    xdr_mountlist, (caddr_t) &ml, TIMEOUT)) {
		pfmt(stderr, MM_ERROR,
		     ":47:%s: server not responding for %s\n",
		     host, clnt_sperror(cl, "clnt_call"));
		clnt_destroy(cl);
		exit(RET_RPCERR);
	}

	if (ml == NULL)
		return;	/* no mounts */

	if (!hflg) {
		/* printf("%-8s %10s %-24s  %s",
			"RESOURCE", "SERVER", "PATHNAME", "CLIENTS"); 
		   The following string is not in the nfscmds.str message
		   file yet (9/26/94) */
		pfmt(stdout, MM_NOSTD,
		     ":48:RESOURCE     SERVER PATHNAME                  CLIENTS");
		hflg++;
	}

	tb = table;
	for (; ml != NULL && tb < &table[NTABLEENTRIES]; ml = ml->ml_nxt)
		*tb++ = ml;
	if (ml != NULL && tb == &table[NTABLEENTRIES])
		pfmt(stderr, MM_ERROR,
		     ":49:table overflow: only %d entries shown\n",
		     NTABLEENTRIES);
	endtb = tb;
	qsort(table, endtb - table, sizeof(struct mountlist *), sortpath);

	last = "";
	for (tb = table; tb < endtb; tb++) {
		if (*((*tb)->ml_path) == '\0' || *((*tb)->ml_name) == '\0')
			continue;
		if (strcmp(last, (*tb)->ml_path) != 0) {
			printf("\n%-8s %10s %-24s ",
				"  -", host, (*tb)->ml_path);
			last = (*tb)->ml_path;
			tail = 0;
		}
		if (tail++)
			printf(",");
		printf("%s", (*tb)->ml_name);
	}
	printf("\n");

	freemntlist(ml);
	clnt_destroy(cl);
}

void
freemntlist(ml)
	struct mountlist *ml;
{
	register struct mountlist *old;

	while (ml) {
		if (ml->ml_name)
			free(ml->ml_name);
		if (ml->ml_path)
			free(ml->ml_path);
		old = ml;
		ml = ml->ml_nxt;
		free(old);
	}
}

int
sortpath(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_path, (*b)->ml_path);
}

void
usage()
{
	pfmt(stderr, MM_ACTION, ":46:Usage: dfmounts [-h] [host ...]\n");
}
